#ifndef __syscalls
#define __syscalls 1

#include <interrupts.h>
#include <stdint.h>

/* Syscall numbers taken from http://blog.rchapman.org/post/36801038863/linux-system-call-table-for-x86-64 */

enum SyscallNumber {
	SYSCALL_WRITE 	= 0,
	SYSCALL_READ 	= 1,
	SYSCALL_OPEN 	= 2,
	SYSCALL_CLOSE 	= 3,
	SYSCALL_MMAP	= 9,
	SYSCALL_MUNMAP	= 11,
	SYSCALL_IOCTL 	= 16,
	SYSCALL_PIPE 	= 22,
	SYSCALL_PAUSE 	= 34,
	SYSCALL_GETPID 	= 39,
	SYSCALL_BEEP 	= 42,
	SYSCALL_HALT 	= 48,
	SYSCALL_FORK 	= 57,
	SYSCALL_EXECV 	= 59,
	SYSCALL_EXIT 	= 60,
	SYSCALL_SETTIME = 227,
	SYSCALL_GETTIME = 228
};

int syscall_write(unsigned int fd, char *str, unsigned int size);
int syscall_read(unsigned int fd, char * buf, unsigned int size);
int syscall_ioctl(unsigned int fd, unsigned long request, void * params);
void syscall_pause(void);
void syscall_halt(void);

uint64_t int80h(uint64_t sysno,
	uint64_t RDI,
	uint64_t RSI,
	uint64_t RDX,
	uint64_t RCX,
	uint64_t R8,
	uint64_t R9);


#define STDIN  0
#define STDOUT 1
#define STDERR 2
#define STDRAW 3

#endif