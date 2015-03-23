
//
// bt.h
//

namespace bt {

    static const int BLOCK_SIZE = 256;
    static const int FRAG_SIZE = 32;
    
    typedef long BLOCKNO;
    typedef long FRAGNO;
    
    static const BLOCKNO INVALID_BLOCK_NUMBER = -1;
    static const FRAGNO  INVALID_FRAG_NUMBER  = -1;
    
    static const int NODE_MAGIC_VALUE = 0x76F3D90A;
    static const int HEADER_MAGIC_VALUE = 0x823A9BE4;
    static const int FRAGMENT_MAGIC_VALUE = 0x2301AD98;
    
    static const int NODE_DATA_LEN = 32;
    static const int OVERFLOW_ENTRY_DATA_LEN = NODE_DATA_LEN - (sizeof(BLOCKNO) + sizeof(FRAGNO));
    static const int MAX_FRAGS_PER_BLOCK = (BLOCK_SIZE-sizeof(long)) / FRAG_SIZE;

    class FileCorruptedException : public std::exception {
    public:
	virtual const char* what() const throw() {
	    return "BTree file is corrupted";
	}
    };

    class NotImplementedException : public std::exception {
    public:
	NotImplementedException() {}
	virtual ~NotImplementedException() throw() {}
	virtual const char* what() const throw() {
	    return "method not implemented";
	}
    };

    class FragmentReservationException : public std::exception {
    public:
	virtual const char* what() const throw() {
	    return "Cannot retrieve requested count of fragments.";
	}
    };
    
    
    enum NODETYPE {
	ntInternalNode,
	ntLeafNode
    };

    enum ENTRYTYPE {
	etComplete,
	etOverflow
    };
    
    class Node {
    public:
	class Data {
	protected:
	    // the standard block header
	    long	_magic;
	    BLOCKNO	_blockno;
	    
	    NODETYPE	_type;
	    int		_n;
	    
	public:
	    Data();
	    Data( BLOCKNO bn );

	    int getMagic() const { return _magic; }
	    BLOCKNO getBlockNumber() const { return _blockno; }
	    NODETYPE getType() const { return _type; }
	    int getKeyCount() const { return _n; }
	    void setKeyCount( int n ) { _n = n; }

	    void* operator new( unsigned int len );
	    void* operator new( unsigned int len, char* buf );

	    void operator delete( void* );
	    void operator delete( void*, char* buf );
	};

	
	class Entry {
	public:
	    int		_key;
	    ENTRYTYPE	_et;
	    int		_len;
	    char	_data[NODE_DATA_LEN];

	    Entry();
	    Entry( const Entry& e );
	    Entry( int key, ENTRYTYPE et, const char* data, int len );

	    Entry& operator = ( const Entry& e );
	    
	    void set( int key, ENTRYTYPE et, const char* data, int len );
	};

	class OverflowEntryData {
	public:
	    BLOCKNO	_block;
	    FRAGNO	_frag;
	    char	_data[OVERFLOW_ENTRY_DATA_LEN];
	};
	
    protected:
	boost::shared_ptr<Data>	_data;
	
    public:
	Node();

	BLOCKNO getBlockNumber() const { return _data->getBlockNumber(); }
	
	NODETYPE getNodeType() const { return _data->getType() ; }
	virtual std::string getNodeTypeName() const = 0;
	    
	virtual void write( os::File f ) throw(os::IoException) ;
	    
	bool isLeaf() { return _data->getType() == ntLeafNode; }
	
	virtual BLOCKNO getChild( int n ) = 0;
	void setChild( int n, boost::shared_ptr<Node> c ) ;
	virtual void setChild( int n, BLOCKNO c ) = 0;
	
	virtual void setEntry( int n, const Entry& e ) = 0;
	virtual const Entry& getEntry( int n ) = 0;

	virtual bool isFull() = 0;
	
	int getKeyCount() { return _data->getKeyCount(); }
	virtual void setKeyCount( int n ) ;

	virtual void merge( const Node::Entry& e, boost::shared_ptr<Node> z ) = 0;
    };

    std::ostream& operator << ( std::ostream& os, const Node& n );

    static const int INTERNAL_ENTRIES = (BLOCK_SIZE - sizeof(Node::Data) - sizeof(BLOCKNO)) / (sizeof(Node::Entry) + sizeof(BLOCKNO));
    static const int INTERNAL_NODE_PADDING = BLOCK_SIZE - sizeof(Node::Data) - (sizeof(Node::Entry) * INTERNAL_ENTRIES) - (sizeof(BLOCKNO) * (INTERNAL_ENTRIES+1));
    
    class InternalNode : public Node {
    protected:
	class Data : public Node::Data {
	protected:
	    Entry		_entries[ INTERNAL_ENTRIES ];
	    BLOCKNO		_children[ INTERNAL_ENTRIES + 1 ];
	    char		_padding[INTERNAL_NODE_PADDING];
	    
	public:
	    Data( BLOCKNO bn );

	    void setChild( int n, BLOCKNO c ) { _children[n] = c; }
	    BLOCKNO getChild( int n ) { return _children[n]; }
	    void setEntry( int n, const Entry& e ) { _entries[n] = e; }
	    const Entry& getEntry( int n ) { return _entries[n]; }
	    std::pair<Entry,BLOCKNO> removeEntryAndRightChild( int n ) ;
	};

	Data* getData() { return static_cast<Data*>( _data.get() ); }
	
    public:
	InternalNode( BLOCKNO bn );
	InternalNode( BLOCKNO bn, boost::shared_ptr<Node::Data> pData );

	virtual std::string getNodeTypeName() const ;
	
	virtual BLOCKNO getChild( int n ) ;
	void setChild( int n, boost::shared_ptr<Node> c ) ;
	virtual void setChild( int n, BLOCKNO c ) ;
	
	virtual void setEntry( int n, const Entry& e ) ;
	virtual const Entry& getEntry( int n ) ;
	std::pair<Entry,BLOCKNO> removeEntryAndRightChild( int n ) ;

	virtual bool isFull() ;
	virtual void setKeyCount( int n ) ;
	virtual void merge( const Node::Entry& e, boost::shared_ptr<Node> z ) ;
    };

    static const int LEAF_ENTRIES = (BLOCK_SIZE - sizeof(Node::Data)) / sizeof(Node::Entry);
    static const int LEAF_NODE_PADDING = BLOCK_SIZE - sizeof(Node::Data) - (sizeof(Node::Entry) * LEAF_ENTRIES) ;
    
    class LeafNode : public Node {
    protected:
	class Data : public Node::Data {
	protected:
	    Entry		_entries[LEAF_ENTRIES];
	    char		_padding[LEAF_NODE_PADDING];
	    
	public:
	    Data( BLOCKNO bn );
	    void setEntry( int n, const Entry& e ) { _entries[n] = e; }
	    const Entry& getEntry( int n ) { return _entries[n]; }
	    Entry removeEntry( int n ) ;
	};
	
	Data* getData() { return static_cast<Data*>( _data.get() ); }
	
    public:
	LeafNode( BLOCKNO bn );
	LeafNode( BLOCKNO bn, boost::shared_ptr<Node::Data> pData );

	virtual std::string getNodeTypeName() const ;
	
	virtual BLOCKNO getChild( int n ) ;
	void setChild( int n, boost::shared_ptr<Node> c ) ;
	virtual void setChild( int n, BLOCKNO c ) ;
	
	virtual void setEntry( int n, const Entry& e ) ;
	virtual const Entry& getEntry( int n ) ;
	Entry removeEntry( int n ) ;
	
	virtual bool isFull() ;
	virtual void setKeyCount( int n ) ;
	virtual void merge( const Node::Entry& e, boost::shared_ptr<Node> z ) ;
    };


    //
    // compute minimum number of entries in nodes
    //  CLR defines a btree node as having degree t, maximum key count 
    //  of 2t-1 and maximum child count for an internal node as 2t.
    //
    //  additionally, the minimum count of keys in any node is
    //  t-1 and the minimum count of children in an internal node is t.
    //
    // if we let m be the maximum count of keys in a node, then the
    //  minimum count of keys, t-1 above is computed as:
    //
    //   2t - 1 = m
    //
    //       2t = m + 1
    //
    //        t = (m + 1) / 2
    //
    // :. t - 1 = ((m + 1) / 2 ) - 1
    //

    static const int MIN_LEAF_ENTRIES     = ((LEAF_ENTRIES+1)/2)-1;
    static const int MIN_INTERNAL_ENTRIES = ((INTERNAL_ENTRIES+1)/2)-1;
    
    
    class OverflowDataHeader {
    public:
	int     _len;
	BLOCKNO _next_block;
	FRAGNO  _next_frag;
    };


    static const int FRAGMENT_HEADER_PADDING = FRAG_SIZE - sizeof(long) - (2*sizeof(BLOCKNO)) - (MAX_FRAGS_PER_BLOCK*sizeof(bool));

    class FragmentBlock {
    public:
	struct Fragment {
	protected:
	    char	_data[FRAG_SIZE];
	public:
	    operator OverflowDataHeader& () { return *(reinterpret_cast<OverflowDataHeader*>(_data)); }
	    char* getData() { return _data+sizeof(OverflowDataHeader); }
	};
	
	class Data {
	protected:
	    // the standard block header
	    long	_magic;
	    BLOCKNO	_blockno;
	    BLOCKNO	_next_block;
	    bool	_used[MAX_FRAGS_PER_BLOCK];
	    char	_padding[FRAGMENT_HEADER_PADDING];
	    Fragment	_frag[MAX_FRAGS_PER_BLOCK];

	public:
	    Data();
	    Data( BLOCKNO bn );
	    
	    void* operator new( unsigned int len );
	    void* operator new( unsigned int len, char* buf );

	    void operator delete( void* );
	    void operator delete( void*, char* buf );
	    
	    int getMagic() const { return _magic; }
	    BLOCKNO getBlockNumber() const { return _blockno; }

	    BLOCKNO getNextBlock() const { return _next_block; }
	    void setNextBlock(BLOCKNO bn) { _next_block = bn; }
	
	    Fragment& getFragment( int n ) {
		assert( n >= 0 && n < MAX_FRAGS_PER_BLOCK );
		return _frag[n];
	    }

	    FRAGNO reserveFragments( int count );
	    std::pair<FRAGNO,int> getMaxFragmentCluster();
	};
	
    protected:
	boost::shared_ptr<Data>	_data;

    public:
	FragmentBlock( BLOCKNO bn );
	FragmentBlock( boost::shared_ptr<Data> pData );
	Fragment& getFragment( int n ) {
	    assert( _data );
	    return _data->getFragment(n);
	}
	int getMagic() const { return _data->getMagic(); }
	BLOCKNO getBlockNumber() const { return _data->getBlockNumber(); }

	BLOCKNO getNextBlock() const { return _data->getNextBlock(); }
	void setNextBlock(BLOCKNO bn) { _data->setNextBlock(bn); }

	FRAGNO reserveFragments( int count );
	std::pair<FRAGNO,int> getMaxFragmentCluster();
	FRAGNO getMaxFragmentClusterStart();
	int getMaxFragmentClusterLength();
	
	void write( os::File file ) throw(os::IoException);
    };

    std::ostream& operator << ( std::ostream& os, const FragmentBlock& n );

    
    static const int HEADER_PADDING_LEN = BLOCK_SIZE - sizeof(long) - ((3+MAX_FRAGS_PER_BLOCK)*sizeof(BLOCKNO));

    class BTree {
	
    protected:
	
	class Header {
	protected:
	    class Data {
	    public:
		// the standard block header
		long		_magic;
		BLOCKNO		_blockno;

		// count of blocks allocated in the file
		BLOCKNO		_blocks;

		// location of first free block.  additional free blocks
		// are chained off of it.
		BLOCKNO		_free_block;

		// a list of chains that track fragment blocks with
		// free fragments in them.
		//   _frag_list[0] is unused
		//   _frag_list[1] is the first block with 1 free fragment
		//   ...
		//   _frag_list[MAX_FRAGS_PER_BLOCK-1] is the first block with
		//      (MAX_FRAGS_PER_BLOCK-1) free fragments
		// no block can have MAX_FRAGS_PER_BLOCK free and still be a
		// partially-full fragment block.  If it has no fragments allocated,
		// then it is an entirely free block, and is put on the free
		// block list
		//
		BLOCKNO		_frag_list[MAX_FRAGS_PER_BLOCK];

		// pad the structure out to be a multiple of BLOCK_SIZE
		char		_padding[HEADER_PADDING_LEN];

		Data();
	    };

	    boost::shared_ptr<Data>	_data;

	public:
	    Header();
	    void write( os::File file ) throw(os::IoException);
	    BLOCKNO allocateBlockNumber();
	    BLOCKNO getFragListHead( int n ) const {
		assert( n > 0 && n < MAX_FRAGS_PER_BLOCK );
		return _data->_frag_list[n];
	    }
	    void setFragListHead( int n, BLOCKNO bn ) const {
		assert( n > 0 && n < MAX_FRAGS_PER_BLOCK );
		_data->_frag_list[n] = bn;
	    }
	};

	//
	// BTree data members
	//
	
	os::File			_file;
	boost::shared_ptr<Node>		_root;
	Header				_header;


	// internal methods
	boost::shared_ptr<Node> allocateNode( NODETYPE nt, BLOCKNO bn );
	boost::shared_ptr<Node> readNode( BLOCKNO bn ) throw(os::IoException,FileCorruptedException);

	boost::shared_ptr<FragmentBlock> allocateFragments( int& frags, BLOCKNO& bn, FRAGNO& fn ) throw(os::IoException) ;
	boost::shared_ptr<FragmentBlock> readFragmentBlock( BLOCKNO bn ) throw(os::IoException,FileCorruptedException) ;
	
	void splitChild( boost::shared_ptr<InternalNode> x, int i, boost::shared_ptr<Node> y ) throw(os::IoException) ;
	void insertNonFull( boost::shared_ptr<Node> x, Node::Entry& e ) throw(os::IoException,FileCorruptedException) ;
	bool search( boost::shared_ptr<Node> x, int k, char* d ) throw(os::IoException,FileCorruptedException);

	bool remove( boost::shared_ptr<Node> x, int key ) throw( os::IoException,FileCorruptedException,NotImplementedException );
	
	Node::Entry removeMinimumEntry( boost::shared_ptr<Node> x ) throw( os::IoException,FileCorruptedException,NotImplementedException );
	Node::Entry removeMaximumEntry( boost::shared_ptr<Node> x ) throw( os::IoException,FileCorruptedException,NotImplementedException );
	
	void dump( boost::shared_ptr<Node> x );

	void writeOverflowEntry( Node::Entry& e, int key, const char* data, int len ) ;
	int writeOverflowData( BLOCKNO& bn, FRAGNO& fn, const char* data, int len ) ;
	
	void readOverflowEntry( const Node::Entry& e, char* d ) ;
	
    public:
	BTree();
	
	void create( std::string fname ) throw(os::IoException) ;
	void insert( int key, const char* data, int len ) throw(os::IoException,FileCorruptedException) ;
	bool search( int key, char* data ) throw(os::IoException,FileCorruptedException) ;
	bool remove( int key ) throw( os::IoException,FileCorruptedException,NotImplementedException );
	
	void dump();
    };


}

