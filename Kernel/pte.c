void entry_add_attrib (entry * e, uint64_t attrib)
{
	*e |= 1 << attrib;
}

void entry_del_attrib (entry * e, uint64_t attrib)
{
	*e &= ~(1 << attrib);
}

void entry_set_frame (entry * e, void * phys_addr, int level)
{
	// do i have to cast phys addr to int???
	uint64_t mask = MASK_TABLE << (level * 9 + 12);
	*e |= (phys_addr & mask) >> (level * 9);
}

int entry_is_present (entry e)
{
	return (e & MASK_PRESENT) > 0
}

int entry_is_writable (entry e)
{
	return (e & MASK_WRITEABLE) > 0
}

void * entry_pfn (entry e)
{
	return (void*)(e & MASK_FRAME)
}