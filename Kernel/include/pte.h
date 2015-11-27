#include <stdint.h>

#define MASK_PRESENT 0x1 			//0000000000000000000000000000001
#define MASK_WRITEABLE 0x2 			//0000000000000000000000000000010
#define MASK_USER 0x4				//0000000000000000000000000000100
#define MASK_WRITETHOUGH 0x8			//0000000000000000000000000001000
#define MASK_NOT_CACHEABLE 0x10		//0000000000000000000000000010000
#define MASK_ACCESSED 0x20		    //0000000000000000000000000100000
#define MASK_DIRTY 0x40				//0000000000000000000000001000000
#define MASK_PAT 0x80 		    	//0000000000000000000000010000000
#define MASK_GLOBAL 0x0100		    	//0000000000000000000000100000000
#define MASK_IGN 0xE00
#define MASK_TABLE 0x1FF
#define MASK_FRAME (~0xFFF)

typedef uint64_t entry;

void pte_set_frame (entry * e, void * phys_addr);
void pte_add_attrib (entry * e, uint64_t attrib);
void pte_del_attrib (entry * e, uint64_t attrib);
int pte_is_present (entry e);
int pte_is_writable (entry e);
void * pte_pfn (entry e);