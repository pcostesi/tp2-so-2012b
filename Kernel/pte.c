#include <pte.h>
#include <stdint.h>

void pte_add_attrib (entry * e, uint64_t attrib)
{
	*e |= 1 << attrib;
}

void pte_del_attrib (entry * e, uint64_t attrib)
{
	*e &= ~(1 << attrib);
}

void pte_set_frame (entry * e, void * phys_addr, int level)
{
	// do i have to cast phys addr to int???
	uint64_t mask = MASK_TABLE << (level * 9 + 12);
	*e |= ((uint64_t)phys_addr & mask) >> (level * 9);
}

int pte_is_present (entry e)
{
	return (e & MASK_PRESENT) > 0;
}

int pte_is_writable (entry e)
{
	return (e & MASK_WRITEABLE) > 0;
}

void * pte_pfn (entry e)
{
	return (void*)(e & MASK_FRAME);
}