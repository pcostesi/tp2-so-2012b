#include <syscalls.h>
#include <video.h>
#include <sound.h>
#include <time.h>
#include <interrupts.h>
#include <keyboard.h>
#include <rtc-driver.h>
#include <sched.h>
#include <stdio.h>

extern void panic(char *);

uint64_t int80h(uint64_t sysno, uint64_t RDI, uint64_t RSI, uint64_t RDX, uint64_t RCX,
	uint64_t R8, uint64_t R9)
{
	int exitno = 0;
	switch (sysno) {
		case SYSCALL_WRITE: /* sys_write fd buf size */
		exitno = syscall_write((unsigned int) RDI, (char *) RSI, (unsigned int) RDX);
		break;

		case SYSCALL_READ: /* sys_read fd buf size */
		exitno = syscall_read((unsigned int) RDI, (char *) RSI, (unsigned int) RDX);
		break;

		case SYSCALL_IOCTL:
		exitno = syscall_ioctl((unsigned int) RDI, (unsigned long) RSI, (void *) RDX);
		break;

		case SYSCALL_PAUSE: /* sys_pause */
		syscall_pause();
		break;

		case SYSCALL_HALT: /* sys_shutdown */
		syscall_halt();
		break;

		case SYSCALL_GETPID:
		exitno = syscall_getpid();
		break;

		case SYSCALL_BEEP: /* sys_beep */
		beep();
		break;

		case SYSCALL_EXIT:
		exitno = syscall_exit((unsigned char) RDI);
		break;

		case SYSCALL_KILL:
		exitno = syscall_kill(RDI, (int) RSI);
		break;

		case SYSCALL_GETTIME:
		syscall_get_time((struct rtc_time *) RDI);
		break;

		case SYSCALL_SETTIME:
		syscall_set_time((struct rtc_time *) RDI);
		break;

		case SYSCALL_MMAP:
		return (uint64_t) syscall_mmap((void *) RDI, (uint64_t) RSI);
		break;

		default:
		return -1;
	}
	return exitno;
}
