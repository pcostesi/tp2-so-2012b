#include <stdint.h>



/*! block size (4k)*/
#define PMM_BLOCK_SIZE 4096

/*1GB Total Memory*/
#define PMM_TOTAL_MEMORY 1073741824

/*262144 Blocks for 4kb blocks and 1 GB total memory*/
#define PMM_TOTAL_BLOCKS (PMM_TOTAL_MEMORY/ PMM_BLOCK_SIZE)

/*Kernel and PMM reserved memory*/
static uint64_t _reserved_pages;


/*! memory map bit array. Each bit represents a memory block*/
static	uint64_t* _pmm_map;


/*Base of the free blocks stack*/

static uint64_t* _mem_stack;

static uint64_t _free_blocks;




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
		uint64_t addr = *(_mem_stack + (_free_blocks));
		_free_blocks -= 1;
		return (void *) addr;
}


void push(void* addr)
{
		*(_mem_stack + _free_blocks + 1) = (uint64_t)addr;
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
		mmap_alloc( (uint64_t)addr / PMM_BLOCK_SIZE );
		return addr;
	}
	return (void *)(-1);
}

/*Returns -1 if addr is not a valid page addr, 0 if mem was allready free and
1 if memory was succesfully freed*/
int fmem(void* addr)
{
	if((uint64_t)addr % PMM_BLOCK_SIZE != 0){
		return -1;
	}
	uint64_t bit_index = (uint64_t)addr / PMM_BLOCK_SIZE;
	if( mmap_is_occ(bit_index) ){
		mmap_free(bit_index);
		push(addr);
		return 1;
	}
	return 0;
}


/*Receives the address needed to be reserved for kernelspace, and adds to it the amount of blocks needed for the PMM usage*/
int init_mem(int reserve)
{
	int bitmap_size = (PMM_TOTAL_BLOCKS/PMM_BLOCK_SIZE); /*Amount of pages needed for the bitmap*/
	int stack_size = (PMM_TOTAL_BLOCKS*size_of(int *)/PMM_BLOCK_SIZE);/*Amount of pages needed for the stack*/
	_reserved_pages = reserve/PMM_BLOCK_SIZE + 1 + bitmap_size + stack_size);
	_pmm_map = (uint64_t *)(PMM_BLOCK_SIZE * (_reserved_pages - bitmap_size) );
	_mem_stack = (uint64_t *)(PMM_BLOCK_SIZE* (_reserved_pages - bitmap_size - stack_size) );
	_free_blocks = PMM_TOTAL_BLOCKS - _reserved_pages;

	uint64_t indx;
	/*We initialize the bitmap memory with 0*/
	for(indx = (uint64_t)_pmm_map; indx < (uint64_t)_pmm_map + PMM_TOTAL_BLOCKS/64 +1 ; indx ++){
		*((uint64_t *)(indx)) = 0;
	}
	/*Bits used for kernel and PMM are marked as reserved*/
	for (indx = 0 ; indx < _reserved_pages ; indx++){
		mmap_alloc(indx*PMM_BLOCK_SIZE);
	}

	/*We initialize the stack with non-reserved memory addresses*/
	for( indx = _reserved_pages; indx < PMM_TOTAL_BLOCKS; indx++ ){
		*(_mem_stack + indx - _reserved_pages) = indx * PMM_BLOCK_SIZE;
	}
	return _reserved_pages*PMM_BLOCK_SIZE;
}

