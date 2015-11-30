#include <vmm.h>
#include <pmm.h>
#include <stdio.h>

static	uint64_t* cur_bitmap;

void pt_complete (uint64_t bit) {

  cur_bitmap[bit / 64] |= (1 << (bit % 64));
}

void pt_incomplete (uint64_t bit) {

  cur_bitmap[bit / 64] &= ~(1 << (bit % 64));
}

int pt_is_complete(uint64_t bit) {

	if(cur_bitmap[bit / 64] & (1 << (bit % 64))){
		return 1;
	}
	else{
		return 0;
	}
}

int vmm_alloc_pages_from (void* from, uint64_t size, int attributes, void** result) {

	// calculate necessary pages
	int needed_pages;
	if (size % VMM_PAGE_SIZE == 0) {
		needed_pages = size / VMM_PAGE_SIZE;
	} else {
		needed_pages = size / VMM_PAGE_SIZE + 1;
	}

	uint64_t pt_num = ((uint64_t)from) >> 21;
	uint64_t pt_offset = (((uint64_t)from) << 43) >> 55;

	return alloc_pages(pt_num, pt_offset, needed_pages, attributes, result);
}

int vmm_alloc_pages (uint64_t size, int attributes, void** result) {

	// calculate necessary pages
	int needed_pages;
	if (size % VMM_PAGE_SIZE == 0) {
		needed_pages = size / VMM_PAGE_SIZE;
	} else {
		needed_pages = size / VMM_PAGE_SIZE + 1;
	}

	// traverse bits until we find enough free pages
	return alloc_pages(0, 0, needed_pages, attributes, result);
}

int alloc_pages(uint64_t pt_number, uint64_t pt_entry_offset, int needed_pages, int attributes, void** result) {

	// if it made it to the end of the bits there is no more virtual space available
	if (pt_number > VMM_TOTAL_PTS) {
		return 0;
	}

	// the cur page table has no more space
	if (pt_is_complete(pt_number)){
		return alloc_pages(pt_number + 1, 0, needed_pages, attributes, result);
	}	

	uint64_t cur_addr = pt_number * VMM_PT_SIZE + pt_entry_offset * VMM_PAGE_SIZE;
	uint64_t next_pt_addr = (pt_number + 1) * VMM_PT_SIZE;

	int pt_incomplete = 0;

	while (1){
		// find the first free page in the pt
		cur_addr = get_free_page_in_pt(cur_addr, next_pt_addr);

		// no more free pages in this pt
		if (cur_addr == next_pt_addr) {
			break;
		}

		// we check if consecutive pages are free aswell so			
		// calculate address to hold needed pages and check if they are available
		uint64_t finish_addr = cur_addr + needed_pages * VMM_PAGE_SIZE;
		uint64_t last_addr = cur_addr;
		int found_memory = mappable_from(cur_addr, finish_addr, attributes, &last_addr);

		// check how far it was able to get
		if (found_memory) {
			// made it to finish_addr so update bitmap and rtn
			*result = (void*)cur_addr;
			update_bitmap(cur_addr, finish_addr, pt_incomplete);
			return 1;
		} else {
			// not enough contiguous pages, start searching again from another directory
			int last_addr_pt_num = last_addr / VMM_PT_SIZE;
			int last_addr_offset = (last_addr % VMM_PT_SIZE) >> 12;
			if (pt_number != last_addr_pt_num){
				// keep searching in another pt since we made it that far
				return alloc_pages(last_addr_pt_num, last_addr_offset, needed_pages, attributes, result);
			} else {
				// if it failed, lets point to the next page in this directory
				cur_addr = last_addr;
				// if it failed in the same directorie keep trying with next page until 
				// we find another candidate or finish traversing the directorie
				pt_incomplete = 1;
			}		
		}
	}
	return alloc_pages(pt_number + 1, 0, needed_pages, attributes, result);
}

uint64_t get_free_page_in_pt(uint64_t start_addr, uint64_t max_addr) {

	if (start_addr == max_addr) {
		return max_addr;
	}

	entry* e;
	int found_entry = get_entry(start_addr, &e);
	if (!found_entry || !pte_is_present(*e)){
		return start_addr;
	}
	return get_free_page_in_pt(start_addr + VMM_PAGE_SIZE, max_addr);
}

int mappable_from(uint64_t cur_addr, uint64_t max_addr, int attributes, uint64_t* last_addr) {

	if (cur_addr == max_addr){
		*last_addr = max_addr;
		return 1;
	}

	// check if the entry doesnt exist and try mapping it
	entry* e;
	int found_entry = get_entry(cur_addr, &e);
	if (!found_entry || !pte_is_present(*e)) {
		*last_addr = cur_addr;
		mappable_from(cur_addr + VMM_PAGE_SIZE, max_addr, attributes, last_addr);
		if (*last_addr == max_addr) {
			// make the actual phys block, map it and update page entry
			if (!alloc_page(cur_addr, attributes)) {
				return 0;
			} else {
				return 1;
			}
		}
	}
	return 0;
}

int get_entry(uint64_t virt_addr, entry** e) {

	// get current cr3 addr
	table* cur_table = (table*)_read_cr3();
	for (int level = 3; level > 0; level--){
		// check table isnt null
		if (!cur_table){			
			return 0;
		}
		// get corresponding entry and check if it is present
		get_entry_from_table(cur_table, (void*)virt_addr, level, e);
		if (pte_is_present(**e)) {
			// get next table
			cur_table = (table*)pte_pfn(**e);
		} else {
			return 0;
		}
	}

	// get the entry pointing to the page
	return get_entry_from_table(cur_table, (void*)virt_addr, 0, e);
}

int get_entry_from_table(table* table, void* virt_addr, int level, entry** entry_ptr) {

	if(table) {
		// get table position from virtual address
		int table_pos = (((uint64_t)virt_addr) & (MASK_TABLE << (level*9+12))) >> (level*9+12);
		*entry_ptr = &table->entries[table_pos];
		return 1;
	}
	return 0;
}

int alloc_page(uint64_t virt_addr, int attributes) {

	// get the phys block
	void* phys_addr = gmem();
	if (!phys_addr) {
		return 0;
	}

	// map the phys block to the virt addr
	entry* e;
	int mapped_page = map_page(phys_addr, (void*) virt_addr, attributes, &e);
	if(!mapped_page) {
		return 0;
	}

	return 1;
}

void update_bitmap(uint64_t start_addr, uint64_t finish_addr, int incomplete) {


	// check dirs used completely
	uint64_t from = start_addr / VMM_PT_SIZE;
	uint64_t to = finish_addr / VMM_PT_SIZE;
	for (uint64_t cur_bit = from; cur_bit <= to; cur_bit++) {
		//printf("updating bit %x\n")
		pt_complete(cur_bit);
	}

	// if first dir was incomplete uncheck it
	if (incomplete) {
		pt_incomplete(from);
	}

	// check if last bit is complete
	if (is_pt_incomplete(to)) {
		pt_incomplete(to);
	}	
}

int is_pt_incomplete(uint64_t pt_num) {

	// where the cur dir ends starting from from_addr
	uint64_t from_addr = pt_num * VMM_PT_SIZE;
	uint64_t finish_addr = (pt_num+1) * VMM_PT_SIZE;

	// recursively check if every page left in the dir is present or exists already
	return is_pt_range_incomplete(from_addr, finish_addr);
}

int is_pt_range_incomplete(uint64_t from_addr, uint64_t finish_addr) {

	if (from_addr == finish_addr) {
		return 0;
	}

	// if its present keep making sure the rest of the pages are as well
	entry* e;
	int found_entry = get_entry(from_addr, &e);
	if (!found_entry || !pte_is_present(*e)) {
		return 1;
	} else {
		return is_pt_range_incomplete(from_addr + VMM_PAGE_SIZE, finish_addr);
	}
}

void vmm_free_pages(void* start_addr, uint64_t size) {

	// mark bits as incomplete
	uint64_t end_addr = ((uint64_t) start_addr) + size;
	int start_pt = ((uint64_t) start_addr) / VMM_PT_SIZE;
	int end_pt = end_addr / VMM_PT_SIZE;
	for (uint64_t cur_pt = start_pt; cur_pt < end_pt; cur_pt++) {
		pt_incomplete(cur_pt);
	}

	if (end_addr % VMM_PT_SIZE != 0) {
		pt_incomplete(end_pt);
	}

	// free pages
	for (uint64_t cur_page = (uint64_t)start_addr; cur_page < end_addr; cur_page += VMM_PAGE_SIZE) {
		entry* e;
		int found_entry = get_entry(cur_page, &e);
		if (found_entry){
			free_page(e);
		}
	}

	// TODO: mark not present page tables that were freed completely
}

void free_page(entry* e) {

 	// get physical block and free it
	fmem((void*) pte_pfn (*e));
 
 	// entry no longer present
	pte_del_attrib (e, MASK_PRESENT);
}

int map_page(void* phys_addr, void* virt_addr, int attributes, entry** e) {

	// get current cr3 addr
	table* cur_table = (table*)_read_cr3();
	// verify tables are allocated and is present
	for (int level = 3; level > 0; level--){
		// verify table exists
		if (!cur_table){
			return 0;
		}

		// get corresponding entry and check if it is present
		int found_entry = get_entry_from_table(cur_table, virt_addr, level, e);
		
		if (found_entry && !pte_is_present(**e)) {

			// since table isnt present we must allocate it
			table* alloc_table = (table*)gmem();
			if (!alloc_table) {
				return 0;
			}

			// clear page table
      		memset (alloc_table, 0, sizeof(table));

      		// make the entry point to the new table and set attributes
      		pte_set_frame (*e, (void*)alloc_table);
      		pte_add_attrib (*e, MASK_PRESENT);
			pte_add_attrib (*e, attributes);
		}

		// get next table
		cur_table = (table*)pte_pfn(**e);
	}

	// im at the 4th table so I need to get the entry and map it
	int found_entry = get_entry_from_table(cur_table, virt_addr, 0, e);

	pte_set_frame(*e, phys_addr);
	pte_add_attrib (*e, MASK_PRESENT);
	pte_add_attrib(*e, attributes);

	return found_entry;
}

void vmm_switch_process(void* cr3, void* bitmap) {
	uint64_t cr3_frame = (uint64_t)cr3) << 12; 	
 	uint64_t new_cr3 = cr3_frame | 0x8;
	cur_bitmap = bitmap;
	_write_cr3(new_cr3);
}

void vmm_shutdown_process(void* cr3, void* bitmap) {

	// addrese for the start and end of vm frees
	uint64_t one_gb_addr = VMM_PAGE_SIZE * (uint64_t)ONE_GB_OF_PAGES;
	uint64_t end_of_vm_addr = VMM_PAGE_SIZE * (uint64_t)(MAX_PAGES - 1);

	// free from 1gb onwards
	for (uint64_t cur_addr = one_gb_addr; cur_addr < end_of_vm_addr; cur_addr += VMM_PAGE_SIZE) {
		vmm_free_pages((void*)cur_addr, VMM_PAGE_SIZE);
	}

	// destroy tables
	recursively_destroy_tables(cr3, 3);
}

void recursively_destroy_tables(void* table_addr, int level) {
	if (level == 0){
		return;
	}
	int cur_entry = 0;
	while (cur_entry < ENTRIES_PER_TABLE) {
		// get next table and go deep enough
		entry e = ((table*)table_addr)->entries[cur_entry];
		if (pte_is_present(e)) {
			void* next_table = pte_pfn(e);
	 		recursively_destroy_tables(next_table, level-1);
	 		fmem(next_table);
		}
		cur_entry++;
	}
}

int vmm_initialize(void** new_bitmap_addr) {

	uint64_t pages_to_identity_map = ONE_GB_OF_PAGES;

	// identity map the pages asked
	int cur_page = 0;
	void* pml4_table;
	uint64_t frame;
	int made_tables = identity_paging(3, &cur_page, pages_to_identity_map, &frame, &pml4_table);
	if (!made_tables) {
		return 0;
	}
	
 	// clean rsv bits to 0 and then clean offset bits, finally add wt bit
 	uint64_t cr3_frame = ((((uint64_t)pml4_table) << 16) >> 28) << 12; 	
 	uint64_t new_cr3 = cr3_frame | 0x8;

 	// tables have been set up properly
	_write_cr3(new_cr3);

	// map bitmap, clean it and mark the used pages so far
	uint64_t phys_bitmap_add = (uint64_t)gmem();
	if (!phys_bitmap_add) {
		return 0;
	}

	// update bitmap addr
	*new_bitmap_addr = (void*)phys_bitmap_add;

	// clean the bitmap
	for (int i=0; i < VMM_PAGE_SIZE; i++) {
		pt_incomplete(i);
	}

	// mark used bits + 1 because of the bitmap page
	mark_bits(0, pages_to_identity_map / ENTRIES_PER_TABLE);

	return 1; 
}

void mark_bits(int from, int to) {

	for (int i = from; i < to; i++) {
		pt_complete(i);
	}
}

int identity_paging(int level, int* cur_page_ptr, int needed_pages, uint64_t* frame, void** new_pml4) {

	if (level == -1) {
		// update the cur pages pointer and return corresponding identity frame
		*frame = ((uint64_t)*cur_page_ptr) * VMM_PAGE_SIZE;
		*cur_page_ptr += 1;
		return 1;
	}

	// create new table
	table* new_table = (table*)gmem();
	memset(new_table, 0, VMM_TABLE_SIZE);
	if (!new_table) {
		return 0;
	}

	// fill the new table
	int cur_entry = 0;
	while (cur_entry < ENTRIES_PER_TABLE && *cur_page_ptr < needed_pages){
		int paged = identity_paging(level-1, cur_page_ptr, needed_pages, frame, new_pml4);
		if (!paged) {
			return 0;
		}

		entry new_entry = 0;
		pte_add_attrib (&new_entry, MASK_PRESENT | MASK_WRITEABLE);
 		pte_set_frame (&new_entry, (void*)(*frame));
 		new_table->entries[cur_entry] = new_entry;
		cur_entry++;
	}

	// if 1 pml4 wasnt enough make more pml4s but return the first one
	if (level == 3 && *cur_page_ptr < needed_pages) {
		identity_paging(level, cur_page_ptr, needed_pages, frame, new_pml4);
	}

	*new_pml4 = (void*)new_table;
	*frame = (uint64_t)new_table;
	return 1;
}

void vmm_print_bitmap(uint64_t from, uint64_t to) {

	for (int i = from; i <= to; i++) {
		printf("%d", pt_is_complete(i));
	}
	printf("\n");
}

void vmm_print_pt(uint64_t pt_number) {

	uint64_t from = pt_number * VMM_PT_SIZE;
	uint64_t to = (pt_number + 1) * VMM_PT_SIZE;
	for (uint64_t cur_addr = from; cur_addr < to; cur_addr += VMM_PAGE_SIZE) {
		entry* e;
		int found_entry = get_entry(cur_addr, &e);
		if (found_entry) {
			printf("%d", pte_is_present(*e));
		} else {
			printf("%d", 0);
		}		
	}
	printf("\n");
}