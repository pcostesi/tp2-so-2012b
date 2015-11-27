#include <pte.h>
#include <stdint.h>

void pte_add_attrib (entry * e, uint64_t attrib)
{
	*e |= attrib;
}

void pte_del_attrib (entry * e, uint64_t attrib)
{
	*e &= ~attrib;
}

void pte_set_frame (entry * e, void * phys_addr)
{
	uint64_t attributes = *e & (~MASK_FRAME);
	uint64_t frame_addr = (uint64_t)phys_addr & MASK_FRAME;
	*e = attributes | frame_addr;
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