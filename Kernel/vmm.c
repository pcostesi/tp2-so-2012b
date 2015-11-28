#include <vmm.h>
#include <pmm.h>

#include <stdio.h>

// 4KB bits of directories (65 GB of virtual memory)
#define VMM_TOTAL_PTS 32768

#define VMM_PT_SIZE 2097152
#define VMM_PAGE_SIZE 4096
#define VMM_TABLE_SIZE 4096

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

void* vmm_alloc_pages (uint64_t size, int attributes) {

	// calculate necessary pages
	int needed_pages;
	if (size % VMM_PAGE_SIZE == 0) {
		needed_pages = size / VMM_PAGE_SIZE;
	} else {
		needed_pages = size / VMM_PAGE_SIZE + 1;
	}

	// traverse bits until we find enough free pages
	return (void*) alloc_pages(0, 0, needed_pages, attributes);
}

uint64_t alloc_pages(uint64_t pt_number, uint64_t pt_entry_offset, int needed_pages, int attributes) {

	// if it made it to the end of the bits there is no more virtual space available
	if (pt_number > VMM_TOTAL_PTS) {

		return 0;
	}

	// the cur page table has no more space
	if (pt_is_complete(pt_number)){
		return alloc_pages(pt_number + 1, 0, needed_pages, attributes);
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
		uint64_t last_addr = mappable_from(cur_addr, finish_addr, attributes);

		// check how far it was able to get
		if (last_addr == finish_addr) {
			// made it to finish_addr so update bitmap and rtn

			update_bitmap(pt_number, cur_addr, finish_addr, pt_incomplete);
			return cur_addr;
		} else {
			// not enough pages, start searching again from another directory
			int last_addr_pt_num = last_addr / VMM_PT_SIZE;
			int last_addr_offset = ((last_addr % VMM_PT_SIZE) >> 12) << 12;
			if (pt_number != last_addr_pt_num){
				// keep searching in another pt since we made it that far
				return alloc_pages(last_addr_pt_num, last_addr_offset, needed_pages, attributes);
			} else {
				// if it failed, lets point to the next page in this directory
				cur_addr = last_addr;
				// if it failed in the same directorie keep trying with next page until 
				// we find another candidate or finish traversing the directorie
				pt_incomplete = 1;
			}		
		}
	}
	return alloc_pages(pt_number + 1, 0, needed_pages, attributes);
}

uint64_t get_free_page_in_pt(uint64_t start_addr, uint64_t max_addr){
	if (start_addr == max_addr) {
		return max_addr;
	}

	entry* e = get_entry(start_addr);
	if (e == NULL || pte_is_present(*e)){
		return start_addr;
	}
	return get_free_page_in_pt(start_addr + PAGE_SIZE, max_addr);
}

uint64_t mappable_from(uint64_t cur_addr, uint64_t last_addr, int attributes) {

	if (cur_addr == last_addr){
		return last_addr;
	}

	// check if the entry doesnt exist and try mapping it
	entry* e = get_entry(cur_addr);
	if (e == NULL || !pte_is_present(*e)) {
		uint64_t result = mappable_from(cur_addr + VMM_PAGE_SIZE, last_addr, attributes);
		if (result == last_addr) {
			// make the actual phys block, map it and update page entry
			if (!alloc_page(cur_addr, attributes)){
				return 0;
			} else {
				return result;
			}
		}
	}
	// didnt make it to the end, return where it failed
	return cur_addr + VMM_PAGE_SIZE;
}

entry* get_entry(uint64_t virt_addr){

	// get current cr3 addr
	table* cur_table = (table*)_read_cr3();

	for (int level = 3; level > 0; level--){
		// check table isnt null
		if (!cur_table){
			return 0;
		}

		// get corresponding entry and check if it is present
		entry* e = get_entry_from_table(cur_table, (void*)virt_addr, level);

		if (e != 0 && pte_is_present(*e)) {
			// get next table
			cur_table = (table*)pte_pfn(*e);
		} else {
			return 0;
		}
		
	}

	// get the entry pointing to the page
	return get_entry_from_table(cur_table, (void*)virt_addr, 0);
}

entry* get_entry_from_table(table* table, void* virt_addr, int level) {
 
	if (table){
		// get table position from virtual address
		int table_pos = (((uint64_t)virt_addr) & (MASK_TABLE << (level*9+12))) >> (level*9+12);
		return &table->entries[table_pos];
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
	entry* e = map_page(phys_addr, (void*) virt_addr, attributes);
	if(!e) {
		return 0;
	}

	return 1;
}

void update_bitmap(uint64_t pt_number, uint64_t start_addr, uint64_t finish_addr, int incomplete) {

	// check dirs used completely
	int from = start_addr / VMM_PT_SIZE;
	int to = finish_addr / VMM_PT_SIZE;
	for (int cur_bit = from; cur_bit <= to; cur_bit++) {
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

int is_pt_incomplete(int pt_num) {

	// where the cur dir ends starting from from_addr
	uint64_t from = pt_num * VMM_PT_SIZE;
	uint64_t finish_addr = from = (pt_num+1) * VMM_PT_SIZE;

	// recursively check if every page left in the dir is present or exists already
	return is_pt_range_incomplete(from, finish_addr);
}

int is_pt_range_incomplete(uint64_t from_addr, uint64_t finish_addr) {

	if (from_addr == finish_addr) {
		return 1;
	}

	// if its present keep making sure the rest of the pages are as well
	entry* e = get_entry(from_addr);
	if (e != NULL && pte_is_present(*e)) {
		return is_pt_range_incomplete(from_addr + VMM_PAGE_SIZE, finish_addr);
	} else {
		return 0;
	}
}

void vmm_free_pages(void* start_addr, uint64_t size) {

	// mark bits as incomplete
	uint64_t end_addr = ((uint64_t) start_addr) + size;
	int start_pt = ((uint64_t) start_addr) / VMM_PT_SIZE;
	int end_pt = end_addr / VMM_PT_SIZE;
	for (int cur_pt = start_pt; cur_pt < end_pt; cur_pt++) {
		pt_incomplete(cur_pt);
	}

	if (end_addr % VMM_PT_SIZE > 0) {
		pt_incomplete(end_pt);
	}

	// free pages
	for (uint64_t cur_page = (uint64_t)start_addr; cur_page <= end_addr; cur_page += 4096) {
		entry* page = get_entry(cur_page);
		if (page != 0){
			free_page(page);
		}
	}

	// TODO: mark not present directories that were freed completely
}

void free_page(entry* e) {
 
 	// get physical block and free it
	void* p = pte_pfn (*e);
	fmem(p);
 
 	// entry no longer present
	pte_del_attrib (e, MASK_PRESENT);
}

entry* map_page(void* phys_addr, void* virt_addr, int attributes) {

	// get current cr3 addr
	table* cur_table = (table*)_read_cr3();

	// verify tables are allocated and is present
	for (int level = 3; level > 0; level--){
		// verify table exists
		if (!cur_table){
			return 0;
		}

		// get corresponding entry and check if it is present
		entry* e = get_entry_from_table(cur_table, virt_addr, level);
		
		if ((*e & MASK_PRESENT) != MASK_PRESENT) {

			// since table isnt present we must allocate it
			table* alloc_table = (table*)gmem();
			if (!alloc_table) {
				return 0;
			}

			// clear page table
      		memset (alloc_table, 0, sizeof(table));

      		// make the entry point to the new table and set attributes
      		pte_set_frame (e, (void*)alloc_table);
      		pte_add_attrib (e, MASK_PRESENT);
			pte_add_attrib (e, attributes);
		}

		// get next table
		cur_table = (table*)pte_pfn(*e);
	}

	// im at the 4th table so I need to get the entry and map it
	entry* page_entry = get_entry_from_table(cur_table, virt_addr, 0);

	pte_set_frame(page_entry, phys_addr);
	pte_add_attrib (page_entry, MASK_PRESENT);
	pte_add_attrib(page_entry, attributes);

	return page_entry;
}

uint64_t* vmm_initialize_kernel(uint64_t pages_to_identity_map) {

	// identity map the pages asked
	int cur_page = 0;
	void* pml4_table = (void*) identity_paging(3, &cur_page, pages_to_identity_map);
	
 	// clean rsv bits to 0 and then clean offset bits, finally add wt bit
 	uint64_t cr3_frame = (((uint64_t)pml4_table << 16) >> 28) << 12; 	
 	uint64_t new_cr3 = cr3_frame | 0x8;

 	// tables have been set up properly
	_write_cr3(new_cr3);	

	// map bitmap, clean it and mark the used pages so far
	uint64_t virt_bitmap_add = cur_page * PAGE_SIZE;
	uint64_t phys_bitmap_add = (uint64_t)gmem();
	map_page((void*)phys_bitmap_add, (void*)virt_bitmap_add, MASK_PRESENT | MASK_WRITEABLE);

	// clean the bitmap and set it
	memset((void*)virt_bitmap_add, 0, VMM_TABLE_SIZE);
	cur_bitmap = (uint64_t*)virt_bitmap_add;

	// mark used bits + 1 because of the bitmap page
	mark_bits(0, (pages_to_identity_map) / ENTRIES_PER_TABLE + 1);

	return cur_bitmap; 
}

void mark_bits(int from, int to) {
	for (int i = from; i <= to; i++) {
		pt_complete(i);
	}
}

uint64_t identity_paging(int level, int* cur_page_ptr, int needed_pages){

	if (level == -1) {
		// update the cur pages pointer and return corresponding identity frame
		*cur_page_ptr += 1;
		return (*cur_page_ptr - 1) * PAGE_SIZE;
	}

	// create new table
	table* new_table = (table*)gmem();
	memset(new_table, 0, VMM_TABLE_SIZE);
	if (!new_table) {
		return NULL;
	}

	// fill the new table
	int cur_entry = 0;
	while (cur_entry < ENTRIES_PER_TABLE && *cur_page_ptr < needed_pages){
		void* next_frame = (void*) identity_paging(level-1, cur_page_ptr, needed_pages);

		entry new_entry = 0;
		pte_add_attrib (&new_entry, MASK_PRESENT | MASK_WRITEABLE);
 		pte_set_frame (&new_entry, next_frame);
 		new_table->entries[cur_entry] = new_entry;
		cur_entry++;
	}

	// if 1 pml4 wasnt enough make more pml4s but return the first one
	if (level == 3 && *cur_page_ptr < needed_pages) {
		identity_paging(level, cur_page_ptr, needed_pages);
	}
	return (uint64_t) new_table;
}

void vmm_print_bitmap(uint64_t from, uint64_t to){

	for (int i = from; i <= to; i++) {
		printf("%d", pt_is_complete(i));
	}
	printf("\n");
}