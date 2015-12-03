#ifndef __MMU_H
#define __MMU_H 1

#include <vmm.h>
#include <stdint.h>
#include <syscalls.h>

#define BLOCK_SIZE (sizeof(block))
#define MIN_ALLOC_SIZE 4

typedef struct block block;

struct block{
	int size;
	block* next;
	block* prev;
	int free;
};

void* syscall_mmap(void* address, uint64_t size);

void* mmu_kmalloc(uint64_t size);
void * mmu_kmalloc_from(void* alloc_from, uint64_t size);
block* find_block(block** last_block, uint64_t size);
block* split_block(block* b, uint64_t size);
block* expand_heap(block* last_block, uint64_t size, void* from);
block* get_base_block();

void mmu_kfree(void* address);
int valid_address(void* address);
block* merge_free_blocks(block* prev, block* block_to_free);
block* get_block(void * address);

#endif