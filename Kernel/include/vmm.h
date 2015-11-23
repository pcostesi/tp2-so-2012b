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

void vmm_dir_complete (uint64_t bit);
void vmm_dir_incomplete (uint64_t bit);
int vmm_dir_is_complete(uint64_t bit);
void vmm_init_bitmap(uint64_t* dir_bitmap_pos);
uint64_t try_allocing_pages(uint64_t dir_offset, uint64_t page_offset, int needed_pages, int supervisor);
void* vmm_alloc_pages (uint64_t size, int supervisor);
uint64_t vmm_mappable_from(uint64_t cur_addr, uint64_t last_addr, int supervisor);
int vmm_alloc_page(entry* e, uint64_t virt_addr, int supervisor) ;
entry* page_is_free(void* virt_addr);
void change_bits(uint64_t cur_dir, uint64_t finish_addr, uint64_t start_addr, int dir_incomplete);
int check_complete_dir_from(uint64_t from_addr);
int check_complete_dir_range(uint64_t from_addr, uint64_t finish_addr);
void vmm_free_pages(void* start_addr, uint64_t size);
void vmm_free_page (entry* e);
entry* vmm_lookup_entry (void* virt_addr);
entry* vmm_lookup_entry_from_table (table* table, void* virt_addr, int level);
int vmm_lookup_dir (void* virt_addr);
entry* vmm_map_page (void* phys_addr, void* virt_addr);
void vmm_initialize(void* bitmap_position, uint64_t pages_to_identity_map);