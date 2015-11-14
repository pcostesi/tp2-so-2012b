#ifndef __SCHED
#define __SCHED 1

#include <stdint.h>
#define SCHED_MAX_PROC (1 << 10)

typedef uint64_t pid_t;
typedef enum {
	ACTIVE,
	WAITING,
	TERMINATED,
	FREE
} pstat_t;

struct sched_process {
	pid_t pid;
	void * symbol;
	void * stack;
	pstat_t status;

	struct sched_process * children;
	struct sched_process * next_sibling;
};


uint64_t sched_switch_to_kernel_stack(uint64_t stack);
uint64_t sched_switch_from_kernel_stack(uint64_t stack);
uint64_t sched_pick_process(void);
uint64_t sched_init_process(struct sched_process * process, void * symbol, void * stack);
uint64_t sched_exec(struct sched_process * process, int argc, char * argv[]);
uint64_t sched_init(void);


#endif