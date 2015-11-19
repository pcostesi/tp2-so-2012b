#include <sched.h>
#include <lib.h>

extern uint64_t bss2;

extern void _drool(void);
extern void _halt(void);
extern int syscall_write(int, char *, int);

static struct sched_process idle_process = {0};
static struct sched_process processes[SCHED_MAX_PROC] = {{0}};
static volatile int current_process_idx = 0;

extern uint64_t _sched_init_stack(void * stack, void * symbol);
extern uint8_t idle_stack;
extern uint8_t idle_kernel_stack;

volatile pid_t max_pid = 0;
static volatile int idle_active = 1;

static uint64_t _sched_idle_process(void)
{
    while (1) syscall_write(2, "HELP! ", 6); _drool();
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
	struct sched_process * process = &processes[max_pid++ % SCHED_MAX_PROC];
	void * stack = (&bss2 + 4096 * max_pid - sizeof(uint64_t));
	void * kernel_stack = (&bss2 + 4096 * max_pid * 2 - sizeof(uint64_t));
	return sched_init_process(process, symbol, stack, kernel_stack);
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

uint64_t sched_switch_from_kernel_stack(uint64_t kernel_stack)
{
	struct sched_process * process = &processes[current_process_idx];
	if (idle_active) {
		idle_process.kernel_stack = (void *) kernel_stack;
		return (uint64_t) idle_process.stack;
	}

	process->kernel_stack = (void *) kernel_stack;
	return (uint64_t) process->stack;
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
			process->status = ACTIVE;
			current_process_idx = next;
			idle_active = 0;

			syscall_write(2, "switching to task ", 18);
			return (uint64_t) process->stack;
		}
	}
	syscall_write(2, "switching to idle ", 18);

	return (uint64_t) idle_process.stack;
}
