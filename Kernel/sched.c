#include <sched.h>
#include <lib.h>
#include <syscalls.h>

extern void _drool(void);
extern void _halt(void);

static struct sched_process idle_process;
static struct sched_process processes[SCHED_MAX_PROC];
static volatile int current_process_idx = 0;

typedef uint8_t page_t[4096];

extern uint64_t _sched_init_stack(void * stack, void * symbol);

static page_t idle_stack;
static page_t idle_kernel_stack;

static page_t process_stacks[SCHED_MAX_PROC];
static page_t kernel_stacks[SCHED_MAX_PROC];

volatile pid_t max_pid = 0;
static volatile int idle_active = 1;

static uint64_t _sched_idle_process(void)
{
    while (1) _drool();
	return 0;
}

static void * get_stack_base(void * stack_base)
{
	return (void *)(
		(uint64_t) stack_base
		+ 4096 					//The size of the stack itself, 4KiB
		- sizeof(uint64_t)		//Begin at the top of the stack
	);
}

uint64_t sched_init(void)
{
	uint64_t symbol = (uint64_t) &_sched_idle_process;
	sched_init_process(&idle_process, (void *) symbol, get_stack_base(&idle_stack), get_stack_base(&idle_kernel_stack));
	return 0;
}

uint64_t sched_init_process(struct sched_process * process, void * symbol, void * stack, void * kernel_stack)
{
	process->symbol = symbol;
	process->status = WAITING;
	process->stack = (void *) _sched_init_stack(stack, symbol);
	process->kernel_stack = kernel_stack;
	return 0;
}

uint64_t sched_spawn_process(void * symbol)
{
	struct sched_process * process = &processes[max_pid % SCHED_MAX_PROC];
	void * stack = get_stack_base(process_stacks[max_pid]);
	void * kernel_stack = get_stack_base(kernel_stacks[max_pid]);
	sched_init_process(process, symbol, stack, kernel_stack);
	return max_pid++;
}

uint64_t sched_switch_to_kernel_stack(uint64_t stack)
{
	struct sched_process * process = &processes[current_process_idx];
	if (idle_active) {
		idle_process.stack = (void *) stack;
		return (uint64_t) idle_process.kernel_stack;
	}

	process->stack = (void *) stack;
	return (uint64_t) process->kernel_stack;
}

uint64_t _sched_get_current_process_entry(void)
{
	struct sched_process * process = &processes[current_process_idx];
	if (idle_active) {
		return (uint64_t) idle_process.symbol;
	}
	return (uint64_t) process->symbol;
}

uint64_t sched_kill_current_process(unsigned short RDI)
{
	struct sched_process * process = &processes[current_process_idx];
	if (process->status != ACTIVE && process->status != WAITING) {
		return -1;
	}
	process->status = TERMINATED;
	process->code	= RDI;
	return RDI;
}


uint64_t sched_pick_process(void)
{
	int idx;
	int next;
	struct sched_process * process;
	struct sched_process * current = &processes[current_process_idx];

	idle_active = 1;
	if (current->status == ACTIVE) {
		current->status = WAITING;
	}

	for (idx = 1; idx < SCHED_MAX_PROC + 1; idx++) {
		next = (current_process_idx + idx) % SCHED_MAX_PROC;
		process = &processes[next];

		if (process->status == WAITING) {
			current->status = WAITING;
			process->status = ACTIVE;
			current_process_idx = next;
			idle_active = 0;
			return (uint64_t) process->stack;
		}
	}
	return (uint64_t) idle_process.stack;
}
