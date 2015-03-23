#include "stdinc.h"
#include "os.h"
#include "dbg.h"
#include "bt.h"

namespace bt {

    void BTree::insert( int key, const char* data, int len ) throw(os::IoException,FileCorruptedException) {
	DBG( dout("bt.insert",1) << "Inserting key " << key << " data length = " << len << std::endl );
	
	boost::shared_ptr<Node> r = _root;

	// determine if data can be inserted within a node, or if
	// it needs to overflow;
	Node::Entry e;

	if( len <= NODE_DATA_LEN ) {
	    e.set( key, etComplete, data, len ) ;
	} else {
	    writeOverflowEntry( e, key, data, len );
	}

	( key, data );
	
	if( r->isFull() ) {

	    boost::shared_ptr<InternalNode> s = boost::shared_dynamic_cast<InternalNode>( allocateNode( ntInternalNode, _header.allocateBlockNumber() ) );
	    _root = s;
	    s->setChild(0,r);

	    splitChild( s, 0, r );
	    insertNonFull( s, e );

	    // don't need to write header here since splitChild writes header changes
	} else {
	    insertNonFull( r, e );
	}
    }

    void BTree::splitChild( boost::shared_ptr<InternalNode> x, int i, boost::shared_ptr<Node> y ) throw(os::IoException) {
	DBG( dout("bt.insert",1) << "Splitting y=" << *y << ", child of x=" << *x << std::endl );

	boost::shared_ptr<Node> z( allocateNode( y->getNodeType(), _header.allocateBlockNumber() ) );
	int nz, ny, j;

	ny = y->getKeyCount() / 2;
	nz = y->getKeyCount() - ny - 1;

	assert( nz + ny + 1 == y->getKeyCount() );

	bool isLeaf = (y->getNodeType() == ntLeafNode );
	for( j = 0; j < nz; j++ ) 
	    z->setEntry( j, y->getEntry( ny+j+1 ) ) ;
	
	if( !isLeaf ) 
	    for( j = 0; j < nz+1; j++ ) 
		z->setChild( j, y->getChild( ny+j+1 ) );

	z->setKeyCount( nz );
	y->setKeyCount( ny );

	for( j = x->getKeyCount(); j > i; j-- )
	    x->setChild( j+1, x->getChild( j ) );
	x->setChild(i+1, z );
	
	for( j = x->getKeyCount()-1; j >= i; j-- )
	    x->setEntry( j+1, x->getEntry(j) );
	x->setEntry(i, y->getEntry(ny) );

	x->setKeyCount( x->getKeyCount() + 1 );

	x->write(_file);
	z->write(_file);
	y->write(_file);
	_header.write(_file);
    }

    void BTree::insertNonFull( boost::shared_ptr<Node> x, Node::Entry& e ) throw(os::IoException,FileCorruptedException) {
	int i = x->getKeyCount()-1;
	bool isLeaf = (x->getNodeType() == ntLeafNode );

	if( isLeaf ) {
	    while( i >= 0 && e._key < x->getEntry(i)._key ) {
		x->setEntry(i+1,x->getEntry(i));
		i--;
	    }
	    x->setEntry(i+1,e);
	    x->setKeyCount( x->getKeyCount() + 1 );
	    x->write( _file );
	} else {
	    while( i >= 0 && e._key < x->getEntry(i)._key ) {
		i--;
	    }
	    i++;
	    boost::shared_ptr<Node> n = readNode( x->getChild(i) );
	    if( n->isFull() ) {
		boost::shared_ptr<InternalNode> xi = boost::shared_dynamic_cast<InternalNode>(x);
		splitChild( xi, i, n );
		if( e._key > xi->getEntry(i)._key ) {
		    i++;
		    n = readNode( xi->getChild(i) );
		}
	    }
	    insertNonFull( n, e );
	}
    }


    void BTree::writeOverflowEntry( Node::Entry& e, int key, const char* data, int len ) {
	assert( len > NODE_DATA_LEN );
	assert( NODE_DATA_LEN == sizeof(Node::OverflowEntryData) );

	// set up the Entry to describe an overflow entry
	e._key = key;		// key
	e._et  = etOverflow;	// entry type indicates an overflow entry
	e._len = len;		// len is total length of data

	// the entry data is an overflow entry data
	Node::OverflowEntryData& oed = *reinterpret_cast<Node::OverflowEntryData*>(e._data);
	oed._block = INVALID_BLOCK_NUMBER;
	oed._frag  = INVALID_FRAG_NUMBER;
	
	// this sets up the block number and frag number to be saved into the overflow entry data
	BLOCKNO&  bn = oed._block;
	FRAGNO&   fn = oed._frag;

	// fill the remainder of the entry with some actual data, up to the maximum
	// count of bytes that can be put into an OverflowEntryData
	memcpy( oed._data, data, OVERFLOW_ENTRY_DATA_LEN );

	// adjust pointer and length of data remaining to be saved
	data += OVERFLOW_ENTRY_DATA_LEN;
	len  -= OVERFLOW_ENTRY_DATA_LEN;

	assert( len > 0 ) ;
	
	// save the remaining data into an overflow block.  the blocknumber and
	// frag number where the data is saved is returned in bn and fn, which
	// are then set into the oed structure through the bn and fn references
	int written = writeOverflowData( bn, fn, data, len );

	assert( len == written );
    }


    int BTree::writeOverflowData( BLOCKNO& bn, FRAGNO& fn, const char* data, int len ) {
	
	// Compute the number of fragments we need to store the remaining data.
	// The amount is the size of a Header plus the length of the data.
	// Then round up by (FRAG_SIZE-1), and divide by FRAG_SIZE to get the
	// whole number of fragments needed to store the header+data.
	int frags = (len + (sizeof(OverflowDataHeader)) + (FRAG_SIZE-1)) / FRAG_SIZE;

	// get a fragment chain from the free list, or create a new fragment block.
	//   block number is returned in bn,
	//   fragment number is return in fn,
	//   frags is adjusted to the total count of frags allocated
	boost::shared_ptr<FragmentBlock> fb = allocateFragments( frags, bn, fn );

	// compute the total space that was allocated
	int totallen = frags * FRAG_SIZE;

	// compute the data space allocated (total space less header size)
	int datalen = totallen - sizeof(OverflowDataHeader);

	// don't write more data than we were asked to...
	if( datalen > len )
	    datalen = len;
		
	// fill in the overflow data header
	OverflowDataHeader& odh	= fb->getFragment(fn);
	odh._len		= datalen;
	odh._next_block		= INVALID_BLOCK_NUMBER;
	odh._next_frag		= INVALID_FRAG_NUMBER;

	// copy the actual data into the fragments
	void* fragdata		= fb->getFragment(fn).getData();
	os::mem::copy( fragdata, data, datalen );

	// figure out how much data is left to write
	int dataleft = len - datalen;

	// if more data to write, then we must chain to another overflow block
	if( dataleft > 0 ) {
	    dataleft = writeOverflowData( odh._next_block, odh._next_frag, data+datalen, dataleft );
	}
	
	// save the fragment block to the file
	fb->write(_file);

	return datalen + dataleft;
    }
}
