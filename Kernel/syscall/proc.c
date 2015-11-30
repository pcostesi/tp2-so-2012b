#include <syscalls.h>
#include <asm.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>

void syscall_pause(void)
{
}

int syscall_exit(unsigned int code)
{
	printf("Killing current process %d with code %d\n", sched_getpid(), code);
	return (int) sched_terminate_process(sched_getpid(), code);
}

int syscall_getpid(void)
{
	return (int) sched_getpid();
}

int syscall_fork(pid_t pid)
{
	return -1;
}

int syscall_kill(int pid, int sig)
{
	switch (sig) {
		case SIGKILL:
		printf("Killing process %d\n", pid);
		return (int) sched_terminate_process((pid_t) pid, 0);

		default:
		return -1;	
	}
	return -1;
}
