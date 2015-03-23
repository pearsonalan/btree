#include "stdinc.h"
#include "os.h"
#include "dbg.h"
#include "bt.h"

namespace bt {

    BTree::Header::Header() {
	assert( sizeof(Data) == BLOCK_SIZE );
	_data = boost::shared_ptr<Data>( new Data() );
    }
    
    BLOCKNO BTree::Header::allocateBlockNumber() {
	return _data->_blocks++;
    }

    void BTree::Header::write( os::File f ) throw(os::IoException) {
	DBG( dout("bt",2) << "Writing header to disk" << std::endl );
	os::File::POS fp = 0;
	f.seek( fp, os::File::SeekAbsolute );
	f.write( _data.get(), BLOCK_SIZE );
    }
    
    BTree::Header::Data::Data() {
	_magic = HEADER_MAGIC_VALUE;
	_blocks = 0;
	_free_block = INVALID_BLOCK_NUMBER;
	for( int i = 0; i < MAX_FRAGS_PER_BLOCK; i++ )
	    _frag_list[i] = INVALID_BLOCK_NUMBER;
	os::mem::clear( _padding, sizeof(_padding) );
    }
}


