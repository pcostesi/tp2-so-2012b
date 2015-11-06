#include <stdint.h>



/*! block size (4k)*/
#define PMM_BLOCK_SIZE 4096

/*1GB Total Memory*/
#define PMM_TOTAL_MEMORY 1073741824

/*262144 Blocks for 4kb blocks and 1 GB total memory*/
#define PMM_TOTAL_BLOCKS (PMM_TOTAL_MEMORY/ PMM_BLOCK_SIZE)
 
/*! size of physical memory*/
static	uint64_t	_pmm_memory_size=0;
 
/*! number of blocks currently in use*/
static	uint64_t	_pmm_used_blocks=0;
 
/*! maximum number of available memory blocks*/
static	uint64_t	_pmm_max_blocks=0;
 
/*! memory map bit array. Each bit represents a memory block*/
static	uint64_t*	_pmm_map= 4096*10;


/*Magic is a LIE*/
static void* _mem_stack = 4096*11; /*Leaving first 9 pages for kernel, 1 for bitmap and the 11th for mem stack*/

static uint64_t _free_blocks = PMM_TOTAL_BLOCKS - 11;


/*TODO checks de rango*/



/*
	BIT MAP	
*/

void mmap_alloc (uint64_t bit) {

  _pmm_map[bit / 64] |= (1 << (bit % 64));
}

void mmap_free (uint64_t bit){

  _pmm_map[bit / 64] &= ~(1 << (bit % 64));
}

int mmap_is_occ(uint64_t bit)
{
	if(_pmm_map[bit / 64] & (1 << (bit%64))){
		return 1;
	}
	else{
		return 0;
	}
}



/*
	STACK
*/

void* pop()
{
		void* addr = _mem_stack + (_free_blocks * PMM_BLOCK_SIZE);
		_free_blocks -= 1;
		return addr;

}


void push(void * addr)
{
		_mem_stack + _free_blocks * PMM_BLOCK_SIZE + PMM_BLOCK_SIZE = addr;
		_free_blocks +=1 ; 
}



/*
	Getting & Freeing pages
*/

void* gmem()
{
	void* addr;
	if ( _free_blocks > 0) {
		addr = pop();
		mmap_alloc( addr / PMM_BLOCK_SIZE );
		return addr;
	}
	return -1;
}

/*Returns -1 if addr is not a valid page addr, 0 if mem was allready free and
1 if memory was succesfully freed*/
int fmem()
{
	if(addr % PMM_BLOCK_SIZE != 0){
		return -1;
	}
	uint64_t bit_index = addr / PMM_BLOCK_SIZE;
	if( mmap_is_occ(bit_index) ){
		mmap_free(bit_index);
		push(addr);
		return 1;
	}
	return 0;
}