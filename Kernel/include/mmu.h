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

void free_thread_pages(int pid);
void* syscall_mmap(void* address, uint64_t size);
void syscall_munmap(void* addr, uint64_t size);

void* mmu_kmalloc(uint64_t size);
block* find_block(block** last_block, uint64_t size);
block* split_block(block* b, uint64_t size);
block* expand_heap(block* last_block, uint64_t size);

void mmu_kfree(void* address);
block* merge_free_blocks(block* prev, block* block_to_free);
block* get_block(void * address);

void mmu_print_kheap();

#endif