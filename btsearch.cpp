#include "stdinc.h"
#include "os.h"
#include "dbg.h"
#include "bt.h"

namespace bt {
    bool BTree::search( int key, char* data ) throw(os::IoException,FileCorruptedException) {
	return search( _root, key, data );
    }

    bool BTree::search( boost::shared_ptr<Node> x, int k, char* d ) throw(os::IoException,FileCorruptedException) {
	int i;
	for( i = 0; i < x->getKeyCount() && k > x->getEntry(i)._key; i++ )
	    ;
	const Node::Entry& e = x->getEntry(i);
	if( i < x->getKeyCount() && k == e._key ) {

	    if( e._et == etComplete ) {
		os::mem::copy( d, e._data, e._len );
	    } else {
		readOverflowEntry( e, d );
	    }
	    
	    return true;
	}
	if( x->isLeaf() )
	    return false;
	boost::shared_ptr<Node> n = readNode( x->getChild(i) );
	return search( n, k, d );
    }

    // read data from an overflow entry and the chained fragments into
    // a buffer
    void BTree::readOverflowEntry( const Node::Entry& e, char* d ) {
	assert( e._et == etOverflow );
	assert( e._len > NODE_DATA_LEN );

	int totallen = e._len;

	// the entry data is an overflow entry data
	const Node::OverflowEntryData& oed = *reinterpret_cast<const Node::OverflowEntryData*>(e._data);

	// copy out as much data as we have stored in the overflow entry
	os::mem::copy( d, oed._data, OVERFLOW_ENTRY_DATA_LEN );

	// adjust pointer and length of data remaining to be read
	d        += OVERFLOW_ENTRY_DATA_LEN;
	totallen -= OVERFLOW_ENTRY_DATA_LEN;

	BLOCKNO bn = oed._block;
	FRAGNO  fn = oed._frag;
	
	while( totallen > 0 ) {
	    assert( bn != INVALID_BLOCK_NUMBER );
	    assert( fn != INVALID_FRAG_NUMBER );
	    
	    boost::shared_ptr<FragmentBlock> fb = readFragmentBlock( bn );
	    OverflowDataHeader& odh = fb->getFragment( fn );

	    // copy out as much data as we have stored in the fragment cluster
	    os::mem::copy( d, fb->getFragment(fn).getData(), odh._len );

	    // adjust pointer and length of data remaining to be read
	    d        += odh._len;
	    totallen -= odh._len;

	    // set up pointers to read next fragment cluster in chain
	    bn = odh._next_block;
	    fn = odh._next_frag;
	}
    }
}
