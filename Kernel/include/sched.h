#ifndef __SCHED
#define __SCHED 1

#include <stdint.h>
#define SCHED_MAX_PROC (16)

typedef uint64_t pid_t;
typedef uint64_t size_t;

enum sched_sleeping {
	IO,
	WAIT,
	TIME
};

struct sched_process {
	volatile pid_t pid;
	void * symbol;
	void * stack;
	size_t page_count;
	void * pagetable;
	void * kernel_stack;
	unsigned short code;

	enum sched_sleeping sleeping;
	union {
		uint64_t epoch;
		pid_t pid;
	} reason;

	struct sched_process * parent;
	struct sched_process * children;
	struct sched_process * next;
};


uint64_t sched_switch_to_kernel_stack(uint64_t stack);
uint64_t sched_spawn_process(void * symbol);
uint64_t sched_pick_process(void);
uint64_t _sched_get_current_process_entry(void);
uint64_t sched_init(void);
uint64_t sched_terminate_process(pid_t pid, unsigned short retval);
pid_t sched_getpid(void);

extern void sched_drop_to_user(void);

#endif