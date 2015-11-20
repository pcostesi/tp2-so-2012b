#include <pte.h>
#include <stdint.h>
#include <lib.h>

// architecture defines 512 entries per table (9 bits)
#define PAGES_TO_INDENTITY_MAP 13
#define ENTRIES_PER_TABLE 512
#define PAGE_SIZE 4096
 
extern void _write_cr3(uint64_t addr);
extern uint64_t _read_cr3(void);
extern void _write_cr0(uint64_t addr);
extern uint64_t _read_cr0(void);

// tables structure
typedef struct table_struct {
 	entry entries[ENTRIES_PER_TABLE];
} table;

typedef struct page_alloc_node_struct {
	page_alloc_node* prev;
	page_alloc_node* next;
	void* page_addr;
	int free;

} page_alloc_node;

int vmm_alloc_page (entry* e);
void vmm_free_page (entry* e);
entry* vmm_lookup_entry (table* table, void* virt_addr, int level);
void vmm_map_page (void* phys_addr, void* virt_addr);
void vmm_initialize();