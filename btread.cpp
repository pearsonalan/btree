#include "stdinc.h"
#include "os.h"
#include "dbg.h"
#include "bt.h"

namespace bt {
    boost::shared_ptr<Node> BTree::readNode( BLOCKNO bn ) throw(os::IoException,FileCorruptedException) {
	char* buf = new char[BLOCK_SIZE];

	os::File::POS fp = bn * (os::File::POS) BLOCK_SIZE;
	
	_file.seek( fp, os::File::SeekAbsolute );
	_file.read( buf, BLOCK_SIZE );

	boost::shared_ptr<Node::Data> pData( new(buf) Node::Data() );

	if( pData->getMagic() != NODE_MAGIC_VALUE )
	    throw FileCorruptedException();

	if( pData->getBlockNumber() != bn ) 
	    throw FileCorruptedException();

	if( pData->getType() == ntInternalNode ) {
	    return boost::shared_ptr<Node>( new InternalNode(bn,pData) );
	} else {
	    return boost::shared_ptr<Node>( new LeafNode(bn,pData) );
	}
    }
    
    boost::shared_ptr<FragmentBlock> BTree::readFragmentBlock( BLOCKNO bn ) throw(os::IoException,FileCorruptedException) {
	char* buf = new char[BLOCK_SIZE];

	os::File::POS fp = bn * (os::File::POS) BLOCK_SIZE;
	
	_file.seek( fp, os::File::SeekAbsolute );
	_file.read( buf, BLOCK_SIZE );

	boost::shared_ptr<FragmentBlock::Data> pData( new(buf) FragmentBlock::Data() );

	if( pData->getMagic() != FRAGMENT_MAGIC_VALUE )
	    throw FileCorruptedException();

	if( pData->getBlockNumber() != bn ) 
	    throw FileCorruptedException();

	return boost::shared_ptr<FragmentBlock>( new FragmentBlock(pData) );
    }
}
