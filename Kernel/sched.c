#include <sched.h>
#include <lib.h>

uint64_t kernel_stack;

static struct sched_process null_process = {0};
static struct sched_process processes[SCHED_MAX_PROC] = {{0}};
static int current_process_idx = 0;

extern uint64_t _sched_init_stack(void * stack, void * symbol);
extern void _halt(void);


pid_t max_pid = 0;

void * null_stack = (void *) (1024 * 1024 * 256);

static uint64_t _sched_null_process(void)
{
	while (1);
	return 0;
}

uint64_t sched_init(void)
{
	uint64_t symbol = (uint64_t) &_sched_null_process;
	symbol = (uint64_t) &_halt;
	memset(null_stack, 0x01, 4096);
	sched_init_process(&null_process, (void *) symbol, null_stack);
	return 0;
}

uint64_t sched_init_process(struct sched_process * process, void * symbol, void * stack)
{
	process->symbol = symbol;
	process->status = WAITING;
	process->stack = (void *) _sched_init_stack(stack, symbol);
	return 0;
}

uint64_t sched_switch_to_kernel_stack(uint64_t stack)
{
	struct sched_process * process = &processes[current_process_idx];
	process->stack = (void *) stack;
	kernel_stack = stack;

	return stack;
}

uint64_t sched_switch_from_kernel_stack(uint64_t stack)
{
	struct sched_process * process = &processes[current_process_idx];
	kernel_stack = stack;
	return (uint64_t) process->stack;
}

uint64_t sched_pick_process(void)
{
	int idx;
	int next;
	struct sched_process * process;
	struct sched_process * current = &processes[current_process_idx];

	current->status = WAITING;

	for (idx = 1; idx < SCHED_MAX_PROC; idx++) {
		next = (current_process_idx + idx) % SCHED_MAX_PROC;
		process = &processes[next];
		if (process->status == WAITING) {
			process->status = ACTIVE;
			current_process_idx = next;
			return (uint64_t) process->stack;
		}
	}

	current->status = WAITING;
	return (uint64_t) null_process.stack;
}
