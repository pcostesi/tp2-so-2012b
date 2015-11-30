#include <mmu.h>

void* syscall_mmap(void* address, uint64_t size) {
	void* new_page;
	if (address == 0) {
		int result = vmm_alloc_pages(size, MASK_USER | MASK_WRITEABLE, &new_page);
		if (result) {
			return new_page;
		} else {
			return 0;
		}
	} else {
		int result = vmm_alloc_pages_from(address, size, MASK_USER | MASK_WRITEABLE, &new_page);
		if (result) {
			return new_page;
		} else {
			return 0;
		}
	}
}

// typedef struct node_struct {
// 	node* prev;
// 	void* addr;
// 	int size;
// 	node* next;
// } node;

// node* cur_node;
// int bytes_left;

// void* kmalloc(int size){
// 	if (cur_node == NULL){
// 		void* new_page;
// 		int result = vmm_alloc_pages(size, MASK_USER | MASK_WRITEABLE, &new_page);
// 		if (!result) {
// 			return NULL;
// 		} else{
// 			cur_node = {NULL, new_page, size}
// 			cur_page = new_page;
// 			bytes_left = size % VMM_PAGE_SIZE;
// 			return cur_page;
// 		}
// 	}
// 	if (bytes_left == 0) {

// 	}
// 	if (bytes_left - size >= 0) {
// 		bytes_left -= size;
// 		return (void *)((uint64_t)last_page + bytes_used);
// 	} else {

// 	}

// }

// void kfree(void* k_addr);

