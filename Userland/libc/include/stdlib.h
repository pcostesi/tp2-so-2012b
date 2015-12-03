#include <stdint.h>

#define BLOCK_SIZE (sizeof(block))
#define MIN_ALLOC_SIZE 4
#define NULL 0

typedef struct block block;

struct block{
	uint64_t size;
	block* next;
	block* prev;
	int free;
};

extern uint8_t bss;
extern uint8_t endOfBinary;

void* malloc(uint64_t size);
block* find_block(block** last_block, uint64_t size);
block* split_block(block* b, uint64_t size);
block* expand_heap(block* last_block, uint64_t size);
block* get_base_block();

void free(void* address);
int valid_address(void* address);
block* merge_free_blocks(block* prev, block* block_to_free);
block* get_block(void * address);