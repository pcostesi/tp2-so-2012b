#include <sched.h>
#include <lib.h>
#include <syscalls.h>

extern void _drool(void);
extern void _halt(void);

static struct sched_process idle_process;
static struct sched_process * active = NULL;
static struct sched_process * terminated = NULL;

typedef uint8_t page_t[4096];

extern uint64_t _sched_init_stack(void * stack, void * symbol);

volatile pid_t max_pid = 0;
volatile pid_t fg_pid = 0;
static volatile int idle_active = 1;

static page_t idle_stack = {0};
static page_t idle_kstack = {0};

static uint64_t _sched_idle_process(void)
{
    while (1) _drool();
	return 0;
}

static struct sched_process * _sched_alloc_process(void)
{
	static struct sched_process processes[SCHED_MAX_PROC] = {{0}};
	static int idx = 0;
	return &processes[idx++];
}

void _sched_free_process(struct sched_process * process)
{

}

static page_t * _sched_alloc_pages(void * base, uint64_t size)
{
	static page_t pages[SCHED_MAX_PROC * 2] = {{0}};
	static int idx = 0;
	return &pages[idx++];
}

static void _sched_free_pages(void * base, uint64_t size)
{

}



static void * get_stack_base(void * stack_base)
{
	return (void *)(
		(uint64_t) stack_base
		+ 4096 					//The size of the stack itself, 4KiB
		- sizeof(uint64_t)		//Begin at the top of the stack
	);
}


static uint64_t _sched_init_process(struct sched_process * process, void * symbol, void * stack, void * kernel_stack, int pages)
{
	process->symbol = symbol;
	process->stack = (void *) _sched_init_stack(stack, symbol);
	process->kernel_stack = kernel_stack;
	process->page_count = pages;
	return 0;
}


uint64_t sched_init(void)
{
	uint64_t symbol = (uint64_t) &_sched_idle_process;
	_sched_init_process(&idle_process,
					   (void *) symbol,
					   get_stack_base(&idle_stack),
					   get_stack_base(&idle_kstack),
					   sizeof(page_t));
	return 0;
}


uint64_t sched_spawn_process(void * symbol)
{
	struct sched_process * process = _sched_alloc_process();
	void * stack = get_stack_base(_sched_alloc_pages(NULL, 1));
	void * kernel_stack = get_stack_base(_sched_alloc_pages(NULL, 1));
	_sched_init_process(process, symbol, stack, kernel_stack, 1);

	process->pid = ++max_pid;
	if (!active) {
		active = process;
	} else {
		process->next = active->next;
	}

	active->next = process;
	return process->pid;
}

uint64_t sched_switch_to_kernel_stack(uint64_t stack)
{
	if (!active) {
		idle_process.stack = (void *) stack;
		return (uint64_t) idle_process.kernel_stack;
	}

	active->stack = (void *) stack;
	return (uint64_t) active->kernel_stack;
}

uint64_t _sched_get_current_process_entry(void)
{
	if (!active) {
		return (uint64_t) idle_process.symbol;
	}
	return (uint64_t) active->symbol;
}

pid_t sched_getpid(void)
{
	return active ? active->pid : 0;
}

static struct sched_process * _sched_terminate_process(struct sched_process * defunct, unsigned short code)
{
	struct sched_process * next = defunct->next;
	defunct->code	= code;
	defunct->next 	= terminated;
	terminated 		= defunct;
	_sched_free_pages(defunct->stack, defunct->page_count);
	_sched_free_pages(defunct->kernel_stack, 1);
	return next;
}

uint64_t sched_terminate_process(pid_t pid, unsigned short retval)
{
	struct sched_process * process = NULL;
	if (active && active->pid == pid) {
		active = _sched_terminate_process(active, retval);
		return 0;
	}
	
	for (process = active->next; process != active; process = process->next) {
		if (process->next->pid != pid) continue;
		process->next = _sched_terminate_process(process->next, retval);
		return 0;
	}

	return -1;
}



uint64_t sched_pick_process(void)
{
	if (!active) return (uint64_t) idle_process.stack;	
	active = active->next;
	return (uint64_t) active->stack;
}
