#include <lib.h>
#include <stdint.h>
#include <pte.h>

// architecture defines 512 entries per table (9 bits)
#define ENTRIES_PER_TABLE 512 

// 4KB bits of directories (65 GB of virtual memory)
#define VMM_TOTAL_PTS 32768

// sizes for structures used
#define VMM_PT_SIZE 2097152
#define VMM_PAGE_SIZE 4096
#define VMM_TABLE_SIZE 4096

#define ONE_GB_OF_PAGES 262144
#define MAX_PAGES 16777216

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

void pt_complete (uint64_t bit);
void pt_incomplete (uint64_t bit);
int pt_is_complete(uint64_t bit);

void vmm_print_pt(uint64_t pt_number);
void vmm_print_bitmap(uint64_t from, uint64_t to);
int vmm_initialize(void** new_bitmap_addr);
void vmm_free_pages(void* start_addr, uint64_t size);
int vmm_alloc_pages (uint64_t size, int attributes, void** result);
int vmm_alloc_pages_from (void* from, uint64_t size, int attributes, void** result);

void vmm_switch_process(void* cr3, void* bitmap);
int identity_paging(int level, int* cur_page_ptr, int needed_pages, uint64_t* frame, void** new_pml4);
void mark_bits(int from, int to);
int map_page(void* phys_addr, void* virt_addr, int attributes, entry** entry);
void free_page(entry* e);
int is_pt_incomplete(uint64_t pt_num);
int is_pt_range_incomplete(uint64_t from_addr, uint64_t finish_addr);
void update_bitmap(uint64_t start_addr, uint64_t finish_addr, int incomplete);
int alloc_page(uint64_t virt_addr, int attributes);
int get_entry_from_table(table* table, void* virt_addr, int level, entry** e);
int get_entry(uint64_t virt_addr, entry** e);
int mappable_from(uint64_t cur_addr, uint64_t max_addr, int attributes, uint64_t* last_addr);
uint64_t get_free_page_in_pt(uint64_t start_addr, uint64_t max_addr);
int alloc_pages(uint64_t pt_number, uint64_t pt_entry_offset, int needed_pages, int attributes, void** result);
void recursively_destroy_tables(void* table_addr, int level);
void vmm_shutdown_process(void* cr3, void* bitmap);
void vmm_print_bitmap_addr(void* addr, uint64_t from, uint64_t to);