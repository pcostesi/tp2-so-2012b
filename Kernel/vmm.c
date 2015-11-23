#include <vmm.h>

#define VMM_TOTAL_DIRECTORIES 134217728
#define VMM_DIR_SIZE 2097152
#define VMM_PAGE_SIZE 4096

static	uint64_t* dir_bitmap;

void vmm_dir_complete (uint64_t bit) {

  dir_bitmap[bit / 64] |= (1 << (bit % 64));
}

void vmm_dir_incomplete (uint64_t bit) {

  dir_bitmap[bit / 64] &= ~(1 << (bit % 64));
}

int vmm_dir_is_complete(uint64_t bit) {

	if(dir_bitmap[bit / 64] & (1 << (bit % 64))){
		return 1;
	}
	else{
		return 0;
	}
}

void vmm_init_bitmap(uint64_t* dir_bitmap_pos) {

	// initiliaze directories as incomplete
	dir_bitmap = dir_bitmap_pos;
	memset(dir_bitmap, 0, VMM_TOTAL_DIRECTORIES/64);
}

entry* vmm_lookup_entry_from_table(table* table, void* virt_addr, int level) {
 
	if (table){
		// get table position from virtual address
		int table_pos = (((uint64_t)virt_addr) & (MASK_TABLE << (level*9+12))) >> (level*9+12);
		return &table->entries[table_pos];
	}
	return 0;
}

int vmm_lookup_dir (void* virt_addr) {
 
 	// get current cr3 addr
	table* cur_table = (table*)_read_cr3();

	for (int level = 3; level > 1; level--){
		// check table isnt null
		if (!cur_table){
			return 0;
		}

		// get corresponding entry and check if it is present
		entry* e = vmm_lookup_entry_from_table(cur_table, virt_addr, level);

		if (e != 0 && pte_is_present(*e)) {
			// get next table
			cur_table = (table*)pte_pfn(*e);
		} else {
			return 0;
		}		
	}
	return 1;
}

entry* page_is_free(void* virt_addr) {

	// get the page entry if possible
	entry* page = vmm_lookup_entry(virt_addr);

	// check the page entry
	if (page == 0 || !pte_is_present(*page)) {
		return page;
	} else {
		return (entry*) 0;
	}
}

uint64_t vmm_mappable_from(uint64_t cur_addr, uint64_t last_addr) {

	if (cur_addr > last_addr){
		return last_addr;
	}

	// check if the entry doesnt exist and try mapping it
	entry* e = page_is_free((void*) cur_addr);
	if (!e) {
		uint64_t result = vmm_mappable_from(cur_addr + PAGE_SIZE, last_addr);
		if (result == last_addr) {
			// make the actual phys block, map it and update page entry
			if (!vmm_alloc_page(e, cur_addr)){
				return 0;
			} else {
				return result;
			}
		}
	}
	// didnt make it to the end, return where it failed
	return cur_addr + PAGE_SIZE;
}

uint64_t try_allocing_pages(uint64_t dir_offset, uint64_t page_offset, int needed_pages) {

	// if it made it to the end of the directories there is no space available
	if (dir_offset > VMM_TOTAL_DIRECTORIES) {
		return 0;
	}

	// the cur dir has no more space
	if (vmm_dir_is_complete(dir_offset)){
		return try_allocing_pages(dir_offset + 1, 0, needed_pages);
	}

	uint64_t dir_addr = dir_offset * VMM_DIR_SIZE + page_offset * VMM_PAGE_SIZE;
	uint64_t cur_addr = dir_addr;

	// TODO: check the present thing
	if (!vmm_lookup_dir((void*) dir_addr)){
		// calculate address to hold such pages
		uint64_t finish_addr = cur_addr + (needed_pages * PAGE_SIZE);
		uint64_t last_addr = vmm_mappable_from(dir_addr, finish_addr);
		if (!last_addr) {
			return 0;
		}
		// check how far it was able to get
		if (last_addr == finish_addr) {
			// found enough pages!
			change_bits(dir_offset, finish_addr, cur_addr, 0);
			return dir_addr;
		} else {
			// not enough pages, start searching again from another directory
			int last_addr_dir = last_addr / VMM_DIR_SIZE;
			uint64_t page_offset = last_addr % VMM_PAGE_SIZE;
			return try_allocing_pages(last_addr_dir, page_offset, needed_pages);		
		}	
	} else {
		int dir_incomplete = 0;
		uint64_t next_dir = dir_addr + VMM_DIR_SIZE;
		while (cur_addr < next_dir){
			// when we finally find a free page we check if consecutive pages are free aswell			
			if (page_is_free((void*) cur_addr)) {
				// calculate address to hold such pages
				uint64_t finish_addr = cur_addr + (needed_pages * VMM_PAGE_SIZE);
				uint64_t last_addr = vmm_mappable_from(cur_addr, finish_addr);
				if (!last_addr) {
					return 0;
				}
				// check how far it was able to get
				if (last_addr == finish_addr) {
					change_bits(dir_offset, finish_addr, cur_addr, dir_incomplete);
					return cur_addr;
				} else {
					// not enough pages, start searching again from another directory
					int last_addr_dir = last_addr % VMM_DIR_SIZE;
					if (dir_offset != last_addr_dir){
						uint64_t page_offset = last_addr % VMM_PAGE_SIZE;
						return try_allocing_pages(last_addr_dir, page_offset, needed_pages);
					} else {
						// if it failed, lets point to the next page in this directory
						cur_addr = last_addr - PAGE_SIZE;
						// if it failed in the same directorie keep trying with next page until 
						// we find another candidate or finish traversing the directorie
						dir_incomplete = 1;
					}					
				}
			}
			cur_addr += PAGE_SIZE;
		}
	}
	// not enough starting from this directorie, lets try with the next one
	return try_allocing_pages(dir_offset + 1, 0, needed_pages);	
}

void* vmm_alloc_pages (uint64_t size) {

	// calculate necessary pages
	int needed_pages;
	if (size % VMM_PAGE_SIZE == 0) {
		needed_pages = size / VMM_PAGE_SIZE;
	} else {
		needed_pages = size / VMM_PAGE_SIZE + 1;
	}

	// recursively traverse directories until we find enough free pages
	return (void*) try_allocing_pages(0, 0, needed_pages);
}

int vmm_alloc_page(entry* e, uint64_t virt_addr) {

	// get the phys block
	void* phys_addr = gmem();
	if (!phys_addr) {
		return 0;
	}

	// map the phys block to the virt addr
	e = vmm_map_page (phys_addr, (void*) virt_addr);
	if(!e) {
		return 0;
	}

	// we know the entry now exists
	pte_set_frame (e, phys_addr, 0);
	pte_add_attrib (e, MASK_PRESENT);
	pte_add_attrib (e, MASK_USER);
	return 1;
}

void change_bits(uint64_t cur_dir, uint64_t finish_addr, uint64_t start_addr, int dir_incomplete) {

	// check dirs used completely
	int used_bits = (finish_addr - start_addr) / (1 << 21);
	for (int i = 0; i < used_bits; i++) {
		vmm_dir_complete(cur_dir + i);
	}

	// if first dir was incomplete uncheck it
	if (dir_incomplete) {
		vmm_dir_incomplete(cur_dir);
	}

	// check if last bit is complete
	if (finish_addr % (1 << 21) == 0) {
		return;

	// check from finish addr to make sure if the last dir is actually complete
	} else if (used_bits > 0 && check_complete_dir_from(finish_addr)){
		vmm_dir_complete(used_bits + 1);
	}	
}

int check_complete_dir_from(uint64_t from_addr) {

	// where the cur dir ends starting from from_addr
	uint64_t finish_addr = ((from_addr / (1 << 21)) + 1) * (1 << 21);

	// recursively check if every page left in the dir is present or exists already
	return check_complete_dir_range(from_addr, finish_addr);
}

int check_complete_dir_range(uint64_t from_addr, uint64_t finish_addr) {

	if (finish_addr > from_addr) {
		return 1;
	}

	// if its present keep making sure the rest of the pages are aswell
	entry* page = vmm_lookup_entry((void*) from_addr);
	if (page != 0 && pte_is_present(*page)) {
		return check_complete_dir_range(from_addr + VMM_PAGE_SIZE, finish_addr);
	} else {
		return 0;
	}
}

void vmm_free_pages(void* start_addr, uint64_t size) {

	// mark directories as incomplete
	uint64_t end_addr = ((uint64_t) start_addr) + size;
	int cur_dir = ((uint64_t) start_addr) / (1 << 21);
	int end_dir = end_addr / (1 << 21);
	for (int i = cur_dir; i <= end_dir; i++) {
		vmm_dir_incomplete(i);
	}

	// free pages
	for (uint64_t cur_page = (uint64_t)start_addr; cur_page <= end_addr; cur_page += 4096) {
		entry* page = vmm_lookup_entry((void*) cur_page);
		if (page != 0){
			vmm_free_page(page);
		}
	}

	// TODO: mark not present directories that were freed completely
}

void vmm_free_page (entry* e) {
 
 	// get physical block
	void* p = pte_pfn (*e);

	// free physical block
	if (p){
		fmem(p);
	}
 
 	// page no longer present
	pte_del_attrib (e, MASK_PRESENT);
}

entry* vmm_lookup_entry(void* virt_addr) {
 
 	// get current cr3 addr
	table* cur_table = (table*)_read_cr3();

	for (int level = 3; level > 0; level--){
		// check table isnt null
		if (!cur_table){
			return 0;
		}

		// get corresponding entry and check if it is present
		entry* e = vmm_lookup_entry_from_table(cur_table, virt_addr, level);

		if (e != 0 && pte_is_present(*e)) {
			// get next table
			cur_table = (table*)pte_pfn(*e);
		} else {
			return 0;
		}
		
	}

	// get the entry pointing to the page
	int table_pos = (((uint64_t)virt_addr) & (MASK_TABLE << 12)) >> 12;
	return &cur_table->entries[table_pos];
}

entry* vmm_map_page (void* phys_addr, void* virt_addr) {

	// get current cr3 addr
	table* cur_table = (table*)_read_cr3();

	// verify tables are allocated and is present
	for (int level = 3; level > 0; level--){
		// verify table exists
		if (!cur_table){
			return 0;
		}

		// get corresponding entry and check if it is present
		entry* e = vmm_lookup_entry_from_table(cur_table, virt_addr, level);

		if ((*e & MASK_PRESENT) != MASK_PRESENT) {
			// since table isnt present we must allocate it
			table* alloc_table = (table*)gmem();
			if (!alloc_table) {
				return 0;
			}

			// clear page table
      		memset (alloc_table, 0, sizeof(table));

      		// make the entry point to the new table and set attributes
			pte_add_attrib (e, MASK_PRESENT);
      		pte_add_attrib (e, MASK_WRITEABLE);
      		pte_set_frame (e, (void*)alloc_table, level);
		}

		// get next table
		cur_table = (table*)pte_pfn(*e);
	}

	// im at the 4th table so I need to get the entry and map it
	entry* page_entry = vmm_lookup_entry_from_table(cur_table, virt_addr, 0);

	pte_set_frame(page_entry, phys_addr, 0);
	pte_add_attrib(page_entry, MASK_PRESENT);

	return page_entry;
}

void vmm_initialize(void* bitmap_position, uint64_t pages_to_identity_map) {

	vmm_init_bitmap(bitmap_position);

	// allocate tables for the identity mapping
	table* pml4_table = (table*)gmem();
	if (!pml4_table) {
		return;
	}
 
	table* directory_ptr_table = (table*)gmem();
	if (!directory_ptr_table) {
		return;
	}

	table* directory_table = (table*)gmem();
	if (!directory_table) {
		return;
	}

	table* page_table = (table*)gmem();
	if (!page_table) {
		return;
	}

	uint64_t frame=0x0;

	// identity map the first pages_to_identity_map pages
	for(uint64_t i=0; i < pages_to_identity_map; i++, frame+=PAGE_SIZE)	{
		
		// create a new page
		entry page = 0;
		pte_add_attrib (&page, MASK_PRESENT);
 		pte_set_frame (&page, (void*)frame, 0);

 		// add it to the page table
		page_table->entries[i] = page;
	};

	// lets add the first entries to the directories that point to such table
	entry first_directory_entry = 0;
	pte_add_attrib (&first_directory_entry, MASK_PRESENT);
 	pte_set_frame (&first_directory_entry, page_table, 1);
 	directory_table->entries[0] = first_directory_entry;

 	entry first_direc_ptr_entry = 0;
	pte_add_attrib (&first_direc_ptr_entry, MASK_PRESENT);
 	pte_set_frame (&first_direc_ptr_entry, directory_table, 2);
 	directory_ptr_table->entries[0] = first_direc_ptr_entry;

 	entry first_pml4_entry = 0;
	pte_add_attrib (&first_pml4_entry, MASK_PRESENT);
 	pte_set_frame (&first_pml4_entry, directory_ptr_table, 3);
 	pml4_table->entries[0] = first_pml4_entry;

 	// write_cr3, read_cr3, write_cr0, and read_cr0 all come from the asm functions
	_write_cr3((uint64_t)pml4_table); // put that pml4 address into CR3
	_write_cr0(_read_cr0() | 0x80000000); // set the paging bit in CR0 to 1
}