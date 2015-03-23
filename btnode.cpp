#include "stdinc.h"
#include "os.h"
#include "dbg.h"
#include "bt.h"

namespace bt {

    
    //-----------------------------------------------------------------------------
    // Node class
    //
    
    Node::Node() {
    }

    Node::Data::Data() {
	assert( _magic == NODE_MAGIC_VALUE );
    }
    
    Node::Data::Data( BLOCKNO bn ) {
	_magic = NODE_MAGIC_VALUE;
	_n = 0;
	_blockno = bn;
    }
    
    void* Node::Data::operator new( unsigned int len ) {
	assert( len == BLOCK_SIZE );
	return new char[len];
    }
    
    void* Node::Data::operator new( unsigned int len, char* buf ) {
	// len is ignored.
	return buf;
    }

    void Node::Data::operator delete( void* pv ) {
	delete[] reinterpret_cast<char*>(pv);
    }
    
    void Node::Data::operator delete( void* pv, char* buf ) {
	assert( pv == (void*) buf );
	delete[] reinterpret_cast<char*>(pv);
    }
	
    Node::Entry::Entry() {
	_key = 0;
	_et = etComplete;
	_len = 0;
    }

    Node::Entry::Entry( const Node::Entry& e ) {
	_key = e._key;
	_et  = e._et;
	_len = e._len;
	os::mem::copy( _data, e._data, sizeof(NODE_DATA_LEN) );
    }
	
    Node::Entry::Entry( int key, ENTRYTYPE et, const char* data, int len ) {
	set(key,et,data,len);
    }

    void Node::Entry::set( int key, ENTRYTYPE et, const char* data, int len ) {
	assert( len <= NODE_DATA_LEN );
	if( data == NULL )
	    len = 0;
	_key = key;
	_et = et;
	_len = len;
	os::mem::copy( _data, data, len );
	if( len < NODE_DATA_LEN )
	    os::mem::clear( _data + len, NODE_DATA_LEN - len );
    }

    Node::Entry& Node::Entry::operator = ( const Node::Entry& e ) {
	_key = e._key;
	_et  = e._et;
	_len = e._len;
	os::mem::copy( _data, e._data, NODE_DATA_LEN );
	return *this;
    }
    
    void Node::write( os::File f ) throw(os::IoException) {
	DBG( dout("bt",2) << "Writing " << *this << " to disk" << std::endl );
	os::File::POS fp = _data->getBlockNumber() * (os::File::POS) BLOCK_SIZE;
	f.seek( fp, os::File::SeekAbsolute );
	f.write( _data.get(), BLOCK_SIZE );
    }
    
    void Node::setChild( int n, boost::shared_ptr<Node> c ) {
	setChild( n, c->getBlockNumber() );
    }

    void Node::setKeyCount( int n ) {
	_data->setKeyCount(n);
    }

    std::ostream& operator << ( std::ostream& os, const Node& n ) {
	return os << n.getNodeTypeName() << "@" << n.getBlockNumber() ;
    }
    
    //-----------------------------------------------------------------------------
    // InternalNode class
    //
    
    InternalNode::InternalNode( BLOCKNO bn ) {
	assert( sizeof(Data) == BLOCK_SIZE );
	_data = boost::shared_ptr<Data>( new Data( bn ) );
    }

    InternalNode::InternalNode( BLOCKNO bn, boost::shared_ptr<Node::Data> pData ) {
	assert( sizeof(Data) == BLOCK_SIZE );
	_data = boost::shared_static_cast<Data>( pData );
    }
    
    InternalNode::Data::Data( BLOCKNO bn ) : Node::Data(bn) {
	_type = ntInternalNode;
	os::mem::clear( _entries, sizeof(_entries) );
	for( int i = 0; i < INTERNAL_ENTRIES+1; i++ )
	    _children[i] = INVALID_BLOCK_NUMBER;
#if INTERNAL_NODE_PADDING > 0	
	os::mem::clear( _padding, sizeof(_padding) );
#endif
    }

    std::pair<Node::Entry,BLOCKNO> InternalNode::Data::removeEntryAndRightChild( int n ) {
	std::pair<Entry,BLOCKNO> peb = std::make_pair( _entries[n], _children[n+1] );
	
	if( n < _n-1 ) {
	    // must shift entries down
	    os::mem::move( _entries + (n), _entries+(n+1), (_n - (n+1)) * sizeof(Entry) );
	    os::mem::clear( _entries + (_n-1), sizeof(Entry) );

	    // shift children down as well
	    os::mem::move( _children + (n+1), _children+(n+2), (_n - (n+2)) * sizeof(BLOCKNO) );
	    os::mem::clear( _children + (_n), sizeof(BLOCKNO) );
	}
	_n--;
	
	return peb;
    }
    
    std::string InternalNode::getNodeTypeName() const {
	return std::string("InternalNode");
    }

    void InternalNode::setKeyCount( int n ) {
	assert( n <= INTERNAL_ENTRIES );
	_data->setKeyCount( n );
    }

    BLOCKNO InternalNode::getChild( int n ) {
	assert( n < INTERNAL_ENTRIES + 1 );
	return getData()->getChild(n) ;
    }
    
    void InternalNode::setChild( int n, boost::shared_ptr<Node> c ) {
	assert( n < INTERNAL_ENTRIES + 1 );
	getData()->setChild(n,c->getBlockNumber() );
    }
    
    void InternalNode::setChild( int n, BLOCKNO c ) {
	assert( n < INTERNAL_ENTRIES + 1 );
	getData()->setChild(n,c);
    }
    
    void InternalNode::setEntry( int n, const Entry& e ) {
	assert( n < INTERNAL_ENTRIES );
	getData()->setEntry(n,e);
    }
    
    const Node::Entry& InternalNode::getEntry( int n ) {
	assert( n < INTERNAL_ENTRIES );
	return getData()->getEntry(n);
    }
    
    bool InternalNode::isFull() {
	return getData()->getKeyCount() == INTERNAL_ENTRIES;
    }

    std::pair<Node::Entry,BLOCKNO> InternalNode::removeEntryAndRightChild( int n ) {
	assert( n < INTERNAL_ENTRIES );
	return getData()->removeEntryAndRightChild(n);
    }
    
    void InternalNode::merge( const Node::Entry& e, boost::shared_ptr<Node> z ) {
	assert( z->getNodeType() == ntInternalNode );
	assert( (getKeyCount() + z->getKeyCount() + 1) <= INTERNAL_ENTRIES );
	
	boost::shared_ptr<InternalNode> node = boost::shared_dynamic_cast<InternalNode>(z);

	// tack the entry e onto the end of our list of entries, and increment the count
	setEntry( getKeyCount(), e );
	setKeyCount( getKeyCount()+1 );

	// go through all of the other node's entries and children and add them to our own list
	int i;
	for( i = 0; i < node->getKeyCount(); i++ ) {
	    setEntry( getKeyCount(), node->getEntry(i) );
	    setChild( getKeyCount(), node->getChild(i) );
	    setKeyCount( getKeyCount()+1 );
	}
	setChild( getKeyCount()+1, node->getChild(i) );
    }
    
    //-----------------------------------------------------------------------------
    // LeafNode class
    //
    
    LeafNode::LeafNode( BLOCKNO bn ) {
	assert( sizeof(Data) == BLOCK_SIZE );
	_data = boost::shared_ptr<Data>( new Data(bn) );
    }

    LeafNode::LeafNode( BLOCKNO bn, boost::shared_ptr<Node::Data> pData ) {
	assert( sizeof(Data) == BLOCK_SIZE );
	_data = boost::shared_static_cast<Data>( pData );
    }
    
    LeafNode::Data::Data( BLOCKNO bn ) : Node::Data(bn) {
	_type = ntLeafNode;
	os::mem::clear( _entries, sizeof(_entries) );
#if LEAF_NODE_PADDING > 0	
	os::mem::clear( _padding, sizeof(_padding) );
#endif
    }

    Node::Entry LeafNode::Data::removeEntry( int n ) {
	Entry e = _entries[n];
	if( n < _n-1 ) {
	    // must shift entries down
	    os::mem::move( _entries + (n), _entries+(n+1), (_n - (n+1)) * sizeof(Entry) );
	    os::mem::clear( _entries + (_n-1), sizeof(Entry) );
	}
	_n--;
	return e;
    }


    std::string LeafNode::getNodeTypeName() const {
	return std::string("LeafNode");
    }

    void LeafNode::setKeyCount( int n ) {
	assert( n <= LEAF_ENTRIES );
	_data->setKeyCount(n);
    }

    BLOCKNO LeafNode::getChild( int n ) {
	assert( false );
	return INVALID_BLOCK_NUMBER;
    }
    
    void LeafNode::setChild( int n, boost::shared_ptr<Node> c ) {
	assert( false );
    }
    
    void LeafNode::setChild( int n, BLOCKNO c ) {
	assert( false );
    }
    
    void LeafNode::setEntry( int n, const Entry& e ) {
	assert( n < LEAF_ENTRIES );
	getData()->setEntry(n,e);
    }
    
    const Node::Entry& LeafNode::getEntry( int n ) {
	assert( n < LEAF_ENTRIES );
	return getData()->getEntry(n);
    }
    
    Node::Entry LeafNode::removeEntry( int n ) {
	assert( n < LEAF_ENTRIES );
	return getData()->removeEntry(n);
    }
    
    bool LeafNode::isFull() {
	return getData()->getKeyCount() == LEAF_ENTRIES;
    }

    void LeafNode::merge( const Node::Entry& e, boost::shared_ptr<Node> z ) {
	assert( z->getNodeType() == ntLeafNode );
	assert( (getKeyCount() + z->getKeyCount() + 1) <= LEAF_ENTRIES );
	
	boost::shared_ptr<LeafNode> leaf = boost::shared_dynamic_cast<LeafNode>(z);

	// tack the entry e onto the end of our list of entries, and increment the count
	setEntry( getKeyCount(), e );
	setKeyCount( getKeyCount()+1 );

	// go through all of the other leaf node's entries and add them to our own list
	for( int i = 0; i < leaf->getKeyCount(); i++ ) {
	    setEntry( getKeyCount(), leaf->getEntry(i) );
	    setKeyCount( getKeyCount()+1 );
	}
    }
}
