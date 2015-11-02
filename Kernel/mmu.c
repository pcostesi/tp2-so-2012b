#include <mmu.h>
#include <stddef.h>
void * mmu_page_alloc(size_t size);
void * mmu_page_free(void * ptr);