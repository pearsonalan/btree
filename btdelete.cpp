#include "stdinc.h"
#include "os.h"
#include "dbg.h"
#include "bt.h"

/*

  Algorithm for deleting from a B-Tree, from Cormen, Leiserson and Rivest,
  page 395.
  
  1. If the key k is in node x and x is a leaf, remove the key k from x

  2. If the key k is in node x and x is an internal node, do the following:

     a. If the child y that preceeds k in node x has at least t keys, then
        find the predecessor k' of k in the subtree rooted at y. Recursively
	remove k' and replace k by k' in x (Finding k' and deleting it can be
	performed in a single downward pass.)

     b. Symmetrically, if the child z that follows k in node x has at least
        t keys, then find the successor k' of k in the subtree rooted at z.
	Recursively remove k' and replace k by k' in x. (Finding k' and
	deleting it can be performed in a single downward pass.)

     c. Otherwise, if both y and z have only t-1 keys, merge k and all of z
        into y, so that x loses both k and the pointer to z, and y now
	contains 2t-1 keys.  Then free z and recursively remove k from y.

  3. If the key k is not present in internal node x, determine the root c_i[x]
     of the appropriate subtree that must contain k, if k is in the tree at
     all. If c_i[x] has only t-1 keys, execute step 3a or 3b as necessary to
     guarantee that we descend to a node containing at least t keys.  Then,
     Finish by recursing on the appropriate child of x.

     a. If c_i[x] has only t-1 keys, but has a sibling with t keys, give c_i[x]
        an extra key by moving a key from x down into c_i[x], moving a key from
	c_i[x]'s immediate left or right sibling up into x, and moving the
	appropriate child from the sibling into c_i[x].

     b. If c_i[x] and all of c_i[x]'s siblings have t-1 keys, merge c_i with one
        sibling, which involves moving a key from x down into the new merged node
	to become the median key for that node.

*/
    
namespace bt {

    bool BTree::remove( int k ) throw( os::IoException,FileCorruptedException,NotImplementedException ) {
	return remove( _root, k ) ;
    }
  
    bool BTree::remove( boost::shared_ptr<Node> x, int k ) throw( os::IoException,FileCorruptedException,NotImplementedException ) {
	// search linearly for the key within the node's entry
	int i;
	for( i = 0; i < x->getKeyCount() && k > x->getEntry(i)._key; i++ )
	    ;
	
	// see if we found the key, or passed the end
	const Node::Entry& e = x->getEntry(i);
	if( i < x->getKeyCount() && k == e._key ) {
	    // key found

	    if( x->isLeaf() ) {
		// case (1) above: remove key k from leaf node
		
		// cast to a leaf node
		boost::shared_ptr<LeafNode> lx = boost::shared_dynamic_cast<LeafNode>(x);
		
		Node::Entry e = lx->removeEntry(i);
		assert( e._key == k );
		
		// TODO: handle deletion of fragments if entry contains fragments
		
		lx->write(_file);
		return true;
	    } else {
		// cast to internal node
		boost::shared_ptr<InternalNode> ix = boost::shared_dynamic_cast<InternalNode>(x);
		
		// NOTE: this may result in one more disk read that is necessary
		// if we had implemented the check for (2a) first.  We would
		// be able to avoid the read of z if y clearly has too many entries.
		
		// check for case 2c first.  see if predecessor node and successor
		// node of k can be merged into one node that includes k as well.
		boost::shared_ptr<Node> y = readNode( ix->getChild(i) );
		boost::shared_ptr<Node> z = readNode( ix->getChild(i+1) );

		bool mergeable = false;
		switch( y->getNodeType() ) {
		case ntLeafNode:
		    mergeable = y->getKeyCount() + z->getKeyCount() + 1 <= LEAF_ENTRIES;
		    break;
		case ntInternalNode:
		    mergeable = y->getKeyCount() + z->getKeyCount() + 1 <= INTERNAL_ENTRIES;
		    break;
		}

		if( mergeable ) {
		    // implement case (2c): merge y, z and k, then remove k from
		    // that node recursively

		    // remove k and the right child (at location i+1), which points to z,
		    // from x
		    std::pair<Node::Entry,BLOCKNO> peb = ix->removeEntryAndRightChild(i);

		    // merge the y, k, and z into the node at y
		    y->merge( peb.first, z );

		    // TODO: z can now be put on the free block.

		    z.reset();

		    x->write(_file);
		    y->write(_file);
		    
		    // recursively delete from y 
		    return remove( y, k );

		} else {
		    // promote a node from the sub-tree that has the fewest elements
		    Node::Entry replacement;
		    
		    if( y->getKeyCount() > z->getKeyCount() ) {
			// implement case (2a): find and remove the maximum entry from the subtree at y.
			// replace k with that entry.
			replacement = removeMaximumEntry(y);
		    } else {
			// implement case (2b): find and remove the minimum entry from the subtree at z.
			// replace k with that entry.
			replacement = removeMinimumEntry(z);
		    }

		    Node::Entry e = ix->getEntry(i);
		    assert( e._key == k );
		
		    // TODO: handle deletion of fragments if entry contains fragments
			
		    ix->setEntry( i, replacement );

		    ix->write(_file);
		    return true;
		}
	    }
	} else {
	    // key not found
	    if( x->isLeaf() ) {
		return false;
	    } else {
		// case 3: child at i must contain the key, guarantee that
		// c_i[x] has sufficient keys, then recursively remove
		// k from ci
		boost::shared_ptr<Node> ci = readNode( x->getChild(i) );

		// determine if ci has enough keys

		// (case 3a) if ci has the minimum count of keys, we
		// should try to adjust it by stealing a node from a
		// sibling. Look at left sibling first, then the right.
		// if one of the siblings has more than the minimum
		// key count, take an entry from it and move it up to x,
		// an move a key from x down into ci

		// (case 3b) if ci's siblings both have the minimum count,
		// merge ci with a sibling and pull the key down from
		// x into the newly created node
		
		// recurse on ci
		return remove( ci, k );
	    }
	
	}

	// notreached
	assert(false);
    }
    
    Node::Entry BTree::removeMinimumEntry( boost::shared_ptr<Node> x ) throw( os::IoException,FileCorruptedException,NotImplementedException ) {
	if( x->isLeaf() ) {
	    // cast to a leaf node
	    boost::shared_ptr<LeafNode> lx = boost::shared_dynamic_cast<LeafNode>(x);
	    assert( lx->getKeyCount() >= 1 );
	    Node::Entry e = lx->removeEntry(0);
	    lx->write(_file);
	    return e;
	} else {
	    // could get into some trouble here. deletion of the minimum key may cause the
	    // leaf node to have fewer entries than the minimum...  ultimately, it could
	    // get down to 0 entries, which would totally screw us.

	    // TODO:  implement node merging logic as above, where we protect against
	    //        having nodes have too few entries by merging nodes as necessary.
	    
	    // read 1st child of x
	    assert( x->getKeyCount() >= 1 );
	    boost::shared_ptr<Node> z = readNode( x->getChild(0) );
	    
	    return removeMinimumEntry( z );
	}
    }
    
    Node::Entry BTree::removeMaximumEntry( boost::shared_ptr<Node> x ) throw( os::IoException,FileCorruptedException,NotImplementedException ) {
	if( x->isLeaf() ) {
	    // cast to a leaf node
	    boost::shared_ptr<LeafNode> lx = boost::shared_dynamic_cast<LeafNode>(x);
	    assert( lx->getKeyCount() >= 1 );
	    Node::Entry e = lx->removeEntry(lx->getKeyCount()-1);
	    lx->write(_file);
	    return e;
	} else {
	    // could get into some trouble here. deletion of the minimum key may cause the
	    // leaf node to have fewer entries than the minimum...  ultimately, it could
	    // get down to 0 entries, which would totally screw us.

	    // TODO:  implement node merging logic as above, where we protect against
	    //        having nodes have too few entries by merging nodes as necessary.
	    
	    // read 1st child of x
	    assert( x->getKeyCount() >= 1 );
	    boost::shared_ptr<Node> z = readNode( x->getChild(x->getKeyCount()) );
	    
	    return removeMaximumEntry( z );
	}
    }
}

