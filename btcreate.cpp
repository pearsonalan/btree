#include "stdinc.h"
#include "os.h"
#include "dbg.h"
#include "bt.h"


namespace bt {

    BTree::BTree() {
	// validate some assumptions about the node sizes

	// MIN entries must be defined such that 2 nodes with t-1 keys can be combined, plus an
	// additional key, to produce a node of 2t-1 keys, which is defined as the maximum
	// entries in a node.

	assert( 2*MIN_LEAF_ENTRIES + 1 <= LEAF_ENTRIES );
	assert( 2*MIN_INTERNAL_ENTRIES + 1 <= INTERNAL_ENTRIES );

	// emit info about compiled-in settings:
	DBG( dout("bt",5) << "blocksize=" << BLOCK_SIZE
			  << ", fragsize=" << FRAG_SIZE
			  << ", entrydatalen=" << NODE_DATA_LEN << std::endl );
	DBG( dout("bt",5) << "oventrydatalen=" << OVERFLOW_ENTRY_DATA_LEN
			  << ", frags/block=" << MAX_FRAGS_PER_BLOCK << std::endl );
	     
	DBG( dout("bt",5) << "InternalNode: max_entries=" << INTERNAL_ENTRIES << ", min_entries=" << MIN_INTERNAL_ENTRIES << std::endl );
	DBG( dout("bt",5) << "LeafNode: max_entries=" << LEAF_ENTRIES << ", min_entries=" << MIN_LEAF_ENTRIES << std::endl );
    }

    void BTree::create( std::string fname ) throw(os::IoException) {
	_file.open( fname,
		    os::File::CreateOrTruncate,
		    os::File::ReadWrite,
		    os::File::ShareNone,
		    os::File::Random );

	// allocate a block for the header
	_header.allocateBlockNumber();

	// allocate a node
	boost::shared_ptr<Node> x = allocateNode( ntLeafNode, _header.allocateBlockNumber() );

	// write the header
	_header.write(_file);

	// write the root node
	x->write(_file);

	// store the root node
	_root = x;
    }

}

