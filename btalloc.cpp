#include "stdinc.h"
#include "os.h"
#include "dbg.h"
#include "bt.h"

namespace bt {
    boost::shared_ptr<Node> BTree::allocateNode( NODETYPE nt, BLOCKNO bn ) {
	if( nt == ntInternalNode ) {
	    return boost::shared_ptr<Node>( new InternalNode(bn) );
	} else {
	    return boost::shared_ptr<Node>( new LeafNode(bn) );
	}
    }


    boost::shared_ptr<FragmentBlock> BTree::allocateFragments( int& frags, BLOCKNO& bn, FRAGNO& fn ) throw(os::IoException) {
	boost::shared_ptr<FragmentBlock> fb;
	
	// we cannot allocate more than 1 block of fragments at a time
	if( frags > MAX_FRAGS_PER_BLOCK )
	    frags = MAX_FRAGS_PER_BLOCK;

	// set the FRAGNO to invalid first, it will be set later once we find a
	// fragment cluster
	fn = INVALID_FRAG_NUMBER;
	bn = INVALID_BLOCK_NUMBER;
	
	// if the request is for a whole block of fragments, just
	// skip to where a new block is allocated, becase the request
	// cannot be accomodated by a partially full block.

	if( frags < MAX_FRAGS_PER_BLOCK ) {
	    // see if there is an existing partially full fragment
	    // block with enough consecutive fragments to fit the
	    // request

	    // try to get a perfect fit first, the progressively try to fit the data
	    // into a bucket with more empty space (best fit first algorithm)
	    for( int b = frags; fn == INVALID_FRAG_NUMBER && b < MAX_FRAGS_PER_BLOCK; b++ ) {

		// see if there are any blocks with b free fragments
		if( _header.getFragListHead(b) != INVALID_BLOCK_NUMBER ) {
		    // read in the block that has the free fragments
		    fb = readFragmentBlock( _header.getFragListHead(b) );
		    assert(fb);

		    bn = fb->getBlockNumber();
		    
		    // reserve that group of fragments
		    try {
			fn = fb->reserveFragments( frags );
		    } catch( FragmentReservationException& ) {
			// it is possible that it failed to reserve a large enough block, which
			// would happen if the block somehow ended up on the wrong freelist.
			// in this case, the logic below will take care of moving the block
			// to the right free list, and we just contine the search.
		    }
		    
		    // take this block off of the head of the list we found it on, since
		    // it will likely now have fewer blocks (though it could have the same,
		    // in which case it will be re-added below).
		    _header.setFragListHead(b, fb->getNextBlock() );
		}
	    }
	}


	//
	// allocate a brand new block if the search for a properly
	// sized partially full block was not fruitful.
	//

	if( fn == INVALID_FRAG_NUMBER ) {
	    bn = _header.allocateBlockNumber();
	    fb = boost::shared_ptr<FragmentBlock>( new FragmentBlock(bn) );
	    fn = fb->reserveFragments( frags );
	}


	// compute X = size of the largest contiguous cluster of
	// fragments left in the block, and add the block to the head
	// of the list with X free fragments
	int cluster_len = fb->getMaxFragmentClusterLength();
	if( cluster_len == 0 ) {
	    // no free fragments left.
	    fb->setNextBlock( INVALID_BLOCK_NUMBER );
	} else {
	    // block has some free fragments available.  put it in the
	    // head of the appropriate list...
			
	    // ... but, if it is already the head of the chain, don't
	    // set the next pointer, or you end up with a circular
	    // reference here
	    if( _header.getFragListHead(cluster_len) != fb->getBlockNumber() )
		fb->setNextBlock( _header.getFragListHead(cluster_len) );
			
	    // put this block on the front of the header
	    _header.setFragListHead(cluster_len, fb->getBlockNumber() );
	}
		    
	_header.write( _file );

	assert( bn != INVALID_BLOCK_NUMBER );
	assert( fn != INVALID_FRAG_NUMBER );
	assert( fb );
	
	return fb;
    }
    

}
