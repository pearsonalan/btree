#include "stdinc.h"
#include "os.h"
#include "dbg.h"
#include "bt.h"

#if !( (MAX_FRAGS_PER_BLOCK+1)*FRAG_SIZE == BLOCK_SIZE )
#error FRAG_SIZE does not evenly divide BLOCK_SIZE
#endif

namespace bt {
    
    FragmentBlock::FragmentBlock( BLOCKNO bn ) {
	assert( (MAX_FRAGS_PER_BLOCK+1)*FRAG_SIZE == BLOCK_SIZE );
	assert( sizeof(Data) == BLOCK_SIZE );
	_data = boost::shared_ptr<Data>( new Data( bn ) );
    }

    FragmentBlock::FragmentBlock( boost::shared_ptr<Data> pData ) {
	assert( sizeof(Data) == BLOCK_SIZE );
	_data = pData;
    }

    FragmentBlock::Data::Data() {
    }
    
    FragmentBlock::Data::Data( BLOCKNO bn ) {
	_magic = FRAGMENT_MAGIC_VALUE;
	_blockno = bn;
	_next_block = INVALID_BLOCK_NUMBER;
	for( int i = 0; i < MAX_FRAGS_PER_BLOCK; i++ ) {
	    _used[i] = false;
	    os::mem::clear( _frag + i, sizeof(Fragment) );
	}
	
	os::mem::clear( _padding, sizeof(_padding) );
    }

    void* FragmentBlock::Data::operator new( unsigned int len ) {
	assert( len == BLOCK_SIZE );
	return new char[len];
    }
    
    void* FragmentBlock::Data::operator new( unsigned int len, char* buf ) {
	// len is ignored.
	return buf;
    }

    void FragmentBlock::Data::operator delete( void* pv ) {
	delete[] reinterpret_cast<char*>(pv);
    }
    
    void FragmentBlock::Data::operator delete( void* pv, char* buf ) {
	assert( pv == (void*) buf );
	delete[] reinterpret_cast<char*>(pv);
    }
	
    
    void FragmentBlock::write( os::File file ) throw(os::IoException) {
	DBG( dout("bt",2) << "Writing " << *this << " to disk" << std::endl );
	os::File::POS fp = _data->getBlockNumber() * (os::File::POS) BLOCK_SIZE;
	file.seek( fp, os::File::SeekAbsolute );
	file.write( _data.get(), BLOCK_SIZE );
    }


    FRAGNO FragmentBlock::Data::reserveFragments( int count ) {
	std::pair<FRAGNO,int> psl = getMaxFragmentCluster();
	if( count > psl.second ) {
	    // block doesn't actually contain enough room to reserve
	    // requested count
	    throw FragmentReservationException();
	}

	for( int i = 0; i < count; i++ ) {
	    assert( psl.first + i < MAX_FRAGS_PER_BLOCK );
	    _used[ psl.first + i ] = true;
	}

	return psl.first;
    }
    
    FRAGNO FragmentBlock::reserveFragments( int count ) {
	return _data->reserveFragments(count);
    }
    
    std::pair<FRAGNO,int> FragmentBlock::Data::getMaxFragmentCluster() {
	std::pair<FRAGNO,int> psl(INVALID_BLOCK_NUMBER,0);

	for( int start = 0; start + psl.second < MAX_FRAGS_PER_BLOCK; start++ ) {
	    if( _used[start] )
		continue;
	    int end;
	    for( end = start+1; _used[end] == false && end < MAX_FRAGS_PER_BLOCK; end++ )
		;
	    if( end - start > psl.second ) {
		psl.first = start;
		psl.second = end - start;
	    }
	}

	return psl;
    }
    
    std::pair<FRAGNO,int> FragmentBlock::getMaxFragmentCluster() {
	return _data->getMaxFragmentCluster();
    }
    
    FRAGNO FragmentBlock::getMaxFragmentClusterStart() {
	std::pair<FRAGNO,int> psl = _data->getMaxFragmentCluster();
	return psl.first;
    }
    
    int FragmentBlock::getMaxFragmentClusterLength() {
	std::pair<FRAGNO,int> psl = _data->getMaxFragmentCluster();
	return psl.second;
    }
    
    std::ostream& operator << ( std::ostream& os, const FragmentBlock& fb ) {
	return os << "FragBlock@" << fb.getBlockNumber() ;
    }
    
}
