#include <mmu.h>

block* base_addr = NULL;
block* last_alloced_block = NULL;

// MMU SYSCALLS

void* syscall_mmap(void* from, uint64_t size) {
	return mmu_kmalloc_from(from, size);
}

// TODO: UNMAP

void* mmu_kmalloc(uint64_t size) {
	return mmu_kmalloc_from(NULL, size);
}

void * mmu_kmalloc_from(void* alloc_from, uint64_t size){

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
			current_block = expand_heap(last_block, size, alloc_from);
			if(!current_block){
				// Couldn't expand heap	
				return NULL;
			}
		}
	}else{
		// First initialization
		current_block = expand_heap(NULL, size, alloc_from);
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

block* expand_heap(block* last_block, uint64_t size, void* from){

	// Mmap enough for the new block and requested size
	void* result;
	int alloced;
	if (from){
		alloced = vmm_alloc_pages_from(from, BLOCK_SIZE + size, MASK_WRITEABLE | MASK_USER, &result);
	} else {
		alloced = vmm_alloc_pages(BLOCK_SIZE + size, MASK_WRITEABLE | MASK_USER, &result);
	}

	if(alloced == 0){
		return NULL;
	}else{
		last_alloced_block = result;
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
	int free_bytes = VMM_PAGE_SIZE - size % VMM_PAGE_SIZE;
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

block* get_base_block(){
	return base_addr;
}

void mmu_kfree(void * address){

	// Check if its valid and get the actual block
	block* block_to_free;
	if(valid_address(address)){
		block_to_free = get_block(address);
		block_to_free->free = 1;

		// Try to merge with next or previous block if they're free to avoid fragmentation
		if(block_to_free->prev && block_to_free->prev->free){
			block_to_free = merge_free_blocks(block_to_free->prev, block_to_free);
		}
		if(block_to_free->next){
			// Check wether the next one is also free for a merge
			if(block_to_free->next->free){
				block_to_free = merge_free_blocks(block_to_free, block_to_free->next);
			}
		}else{
			// If it was the last block, make sure prev points to null
			if (block_to_free->prev){
				block_to_free->prev->next = NULL;
			}else{
				// Has no next nor previous so heap is empty
				base_addr = NULL;
			}
		}
	}
}

block* merge_free_blocks(block* prev_block, block* block_to_free){

	//Merges the blocks and data
	prev_block->size += block_to_free->size + BLOCK_SIZE;
	prev_block->next = block_to_free->next;
	if (prev_block->next){
		prev_block->next->prev = prev_block;
	}
	return prev_block;
 }
 
int valid_address(void * address){

	// Check if its a value in the heap
	if(base_addr){
		if(address >= (void *)base_addr && address <= (void *)last_alloced_block){
			return get_block((void*)((uint64_t)address - BLOCK_SIZE)) != NULL;
		}
	}

	return 0;
}

block* get_block(void * address){

	// Check if the address is a block in the structure
	block* cur_block = base_addr;
	while (cur_block){
		if (cur_block == address){
			return cur_block;
		}
		cur_block = cur_block->next;
	}
	return NULL;
}