#ifndef __pmm
#define __pmm 1


void mmap_alloc (uint64_t bit);
void mmap_free (uint64_t bit);
int mmap_is_occ(uint64_t bit);
void* pop();
void push(void * addr);
void* gmem();
int fmem();
void init_mem(int reserve);

#endif