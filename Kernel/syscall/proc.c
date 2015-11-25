#include <syscalls.h>
#include <asm.h>
#include <sched.h>

void syscall_pause(void)
{
}

int syscall_exit(unsigned int code)
{
	return (int) sched_terminate_process(sched_getpid(), code);
}

int syscall_getpid(void)
{
	return (int) sched_getpid();
}
