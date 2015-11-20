#include <vmm.h>

int vmm_alloc_page (entry* e) {
 
	// allocate a free physical block
	void* p = pmmngr_alloc_block();
	if (!p){
		return 0;
	}
 
	// map it to the page
	pte_set_frame (e, p, 0);
	pte_add_attrib (e, MASK_PRESENT);
 
	return 1;
}

void vmm_free_page (entry* e) {
 
 	// get physical block
	void* p = pte_pfn (*e);

	// free physical block
	if (p){
		pmmngr_free_block (p);
	}
 
 	// page no longer present
	pte_del_attrib (e, MASK_PRESENT);
}

entry* vmm_lookup_entry (table* table, void* virt_addr, int level) {
 
	if (table){
		// get table position from virtual address
		int table_pos = (((uint64_t)virt_addr) & (MASK_TABLE << (level*9+12))) >> (level*9+12);
		return &table->entries[table_pos];
	}
	return 0;
}


void vmm_map_page (void* phys_addr, void* virt_addr) {

	// get current cr3 addr
	table* cur_table = (table*)_read_cr3();

	// verify tables are allocated and is present
	for (int level = 3; level > 0; level--){
		// verify table exists
		if (!cur_table){
			return;
		}

		// get corresponding entry and check if it is present
		entry* e = vmm_lookup_entry(cur_table, virt_addr, level);

		if ((*e & MASK_PRESENT) != MASK_PRESENT) {
			// since table isnt present we must allocate it
			table* alloc_table = (table*)pmmngr_alloc_block();
			if (!alloc_table) {
				return;
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
	entry* page_entry = vmm_lookup_entry(cur_table, virt_addr, 0);

	pte_set_frame(page_entry, phys_addr, 0);
	pte_add_attrib(page_entry, MASK_PRESENT);
}

void vmm_initialize() {

	// allocate tables for the identity mapping
	table* pml4_table = (table*)pmmngr_alloc_block();
	if (!pml4_table) {
		return;
	}
 
	table* directory_ptr_table = (table*)pmmngr_alloc_block();
	if (!directory_ptr_table) {
		return;
	}

	table* directory_table = (table*)pmmngr_alloc_block();
	if (!directory_table) {
		return;
	}

	table* page_table = (table*)pmmngr_alloc_block();
	if (!page_table) {
		return;
	}

	// map the first 13 pages
	for(uint64_t i=0, frame=0x0; i < PAGES_TO_INDENTITY_MAP; i++, frame+=PAGE_SIZE)	{
		
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