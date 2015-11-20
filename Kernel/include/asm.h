#ifndef __LIB_ASM
#define __LIB_ASM 1

#include <stdint.h>
#include <stddef.h>

#define CPU_VENDOR_BUFFER_SIZE 10

extern char * cpu_vendor(char * result);
extern uint32_t get_memory_size(void);

extern void _cli(void);
extern void _sti(void);
extern void _halt(void);
extern void _drool(void);

#endif