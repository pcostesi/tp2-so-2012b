#include <mmu.h>
#include <stdio.h>
#include <pmm.h>

void* cur_page;
uint64_t used_bytes;
uint64_t bytes_reserved;

void* syscall_mmap(void* addr, uint64_t size) {
	void* ret_addr;
	if (cur_page && bytes_reserved - used_bytes > size) {
		ret_addr = (void*)((uint64_t) cur_page + used_bytes);
		used_bytes += size;
		return ret_addr;
	} else {
		int result;
		if (addr) {
			result = vmm_alloc_pages_from(addr, size, MASK_WRITEABLE | MASK_USER, &ret_addr);
		} else {
			result = vmm_alloc_pages(size, MASK_WRITEABLE | MASK_USER, &ret_addr);
		}
		if (result) {
			uint64_t pages_used = size / VMM_PAGE_SIZE;
			bytes_reserved = pages_used * VMM_PAGE_SIZE;
			if (size % VMM_PAGE_SIZE != 0) {
				bytes_reserved += VMM_PAGE_SIZE;
			}
			used_bytes = size;
			cur_page = ret_addr;
			return ret_addr;
		} else {
			return NULL;
		}
	}
}

void syscall_munmap(void* addr, uint64_t size) {
	// TODO
	/*if (used_bytes - size <= 0) {
		vmm_free_pages(cur_page, VMM_PAGE_SIZE);
		cur_page = NULL;
		used_bytes = 0;
	} else {
		used_bytes -= size;
	}*/
}

block* base_addr = NULL;

void * mmu_kmalloc(uint64_t size){

	if (size > VMM_PAGE_SIZE) {
		printf("Way too big bro\n");
		return 0;
	}

	if(size < 0){
		size = 0;
	}

	block* current_block, *last_block;
	
	if(base_addr){

		// Find a block starting from base address
		last_block = base_addr;
		current_block = find_block(&last_block, size);
		if(current_block){

			// Try to split the current block, using what is just necessary
			if(current_block->size - size >= BLOCK_SIZE + MIN_ALLOC_SIZE){
				split_block(current_block, size);
			}
			current_block->free = 0;
		}else{

			// Expand heap because it didnt find any available block to split
			current_block = expand_heap(last_block, size);
			if(!current_block){
				// Couldn't expand heap	
				return NULL;
			}
		}
	}else{
		// First initialization
		current_block = expand_heap(NULL, size);
		if(!current_block){
			// Couldn't expand heap	
			return NULL;
		}
	}
	return (void *)(current_block + 1);
}

block* find_block(block** last_block, uint64_t size){

	// Find the first free block with enough space or return null
	block* cur_block = base_addr;
	while(cur_block && !(cur_block->free && cur_block->size >= size)){
		*last_block = cur_block;
		cur_block = cur_block->next;
	}
	return cur_block;
}

block* split_block(block* b, uint64_t size){
	
	// New block metadata
	block* new_block;
	new_block = (block*)((uint64_t)(b + 1) + size);
	new_block->next = b->next;
	new_block->prev = b;
	new_block->free = 1;
	new_block->size = b->size - size - BLOCK_SIZE;

	//Update old block
	b->next = new_block;
	b->size = size;
	if(new_block->next){
		new_block->next->prev = new_block;
	}
	return b;
}

block* expand_heap(block* last_block, uint64_t size){

	// Mmap enough for the new block and requested size
	void* result = gmem();

	if(result == NULL){
		return NULL;
	}
	
	// Create metadata block
	block* new_block;
	new_block = (block *)result;
	new_block->size = size;
	new_block->free = 0;
	new_block->prev = last_block;
	new_block->next = NULL;

	// Update last block references
	if(last_block){
		last_block->next = new_block;
	}else{
		base_addr = new_block;
	}

	// create extra free block if space is available
	int free_bytes = VMM_PAGE_SIZE % size;
	if (free_bytes > BLOCK_SIZE + MIN_ALLOC_SIZE && free_bytes < 4096) {
		block* free_block;
		free_block = (block *)((uint64_t) result + size + BLOCK_SIZE);
		free_block->size = free_bytes - BLOCK_SIZE;
		free_block->free = 1;
		free_block->prev = new_block;
		free_block->next = NULL;
		new_block->next = free_block;
	}

	return new_block;	
}

void mmu_kfree(void * address){

	// Check if its valid and get the actual block
	block* block_to_free;
	block_to_free = get_block(address);
	if (block_to_free == NULL){
		printf("Block doesnt exist\n");
	}
	block_to_free->free = 1;

	if(!block_to_free->next){			
		// If it was the last block, make sure prev points to null
		if (block_to_free->prev){
			block_to_free->prev->next = NULL;
		}else{
			// Has no next nor previous so heap is empty
			base_addr = NULL;
		}

		// Unmap since it was the last block
		fmem((void*)block_to_free);
	}
}

block* get_block(void * address){

	// Check if the address is a block in the structure
	block* cur_block = base_addr;
	while (cur_block){
		if (cur_block+1 == address){
			return cur_block;
		}
		cur_block = cur_block->next;
	}
	return NULL;
}

void mmu_print_kheap() {
	block* cur_block = base_addr;
    
    if (!cur_block){
        printf("Heap is empty bro\n");
        return;
    }

    while (cur_block){
        printf("Block address: %d\nData address: %d\nBlock size: %d\nPrev block: %d\nNext block: %d\nFree: %d\nString: %s\n\n", cur_block, cur_block + 1, cur_block->size, cur_block->prev, cur_block->next, cur_block->free, cur_block + 1);
        cur_block = cur_block->next;
    }
}