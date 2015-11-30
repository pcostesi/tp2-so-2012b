#ifndef __MMU_H
#define __MMU_H 1

#include <vmm.h>
#include <stdint.h>
#include <syscalls.h>

void* syscall_mmap(void* address, uint64_t size);

// void* kmalloc(int size);
// void kfree(void* k_addr);

#endif