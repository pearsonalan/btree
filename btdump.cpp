#include "stdinc.h"
#include "os.h"
#include "dbg.h"
#include "bt.h"

namespace bt {

    void BTree::dump() {
	dump(_root);
	std::cout << std::endl;
    }
    
    void BTree::dump( boost::shared_ptr<Node> x ) {
	bool isLeaf = x->isLeaf();
	int i;
	for( i = 0; i < x->getKeyCount(); i++ ) {
	    if( !isLeaf ) {
		boost::shared_ptr<Node> c = readNode( x->getChild(i) );
		dump(c);
	    }
	    std::cout << x->getEntry(i)._key << ":" << x->getEntry(i)._data << " ";
	}
	if( !isLeaf ) {
	    boost::shared_ptr<Node> c = readNode( x->getChild(i) );
	    dump(c);
	}
    }
    
}
