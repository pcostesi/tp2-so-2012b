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
#pragma pack(push, 1)
typedef struct table_struct {
 	entry entries[ENTRIES_PER_TABLE];
} table;
#pragma pack(pop)

uint64_t* vmm_initialize_kernel(uint64_t pages_to_identity_map);
void* vmm_alloc_pages (uint64_t size, int attributes);
void vmm_free_pages(void* start_addr, uint64_t size);
void vmm_print_bitmap(uint64_t from, uint64_t to);

void pt_complete (uint64_t bit);
void pt_incomplete (uint64_t bit);
int pt_is_complete(uint64_t bit);

uint64_t alloc_pages(uint64_t pt_number, uint64_t pt_entry_offset, int needed_pages, int attributes);
uint64_t get_free_page_in_pt(uint64_t start_addr, uint64_t max_addr);
uint64_t mappable_from(uint64_t cur_addr, uint64_t last_addr, int attributes);
entry* get_entry(uint64_t virt_addr);
entry* get_entry_from_table(table* table, void* virt_addr, int level);
int alloc_page(uint64_t virt_addr, int attributes);
void update_bitmap(uint64_t pt_number, uint64_t start_addr, uint64_t finish_addr, int incomplete);
int is_pt_incomplete(int pt_num);
int is_pt_range_incomplete(uint64_t from_addr, uint64_t finish_addr);
void free_page(entry* e);
entry* map_page(void* phys_addr, void* virt_addr, int attributes);
void mark_bits(int from, int to);
uint64_t identity_paging(int level, int* cur_page_ptr, int needed_pages);