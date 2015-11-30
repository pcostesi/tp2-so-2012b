#include <sched.h>
#include <lib.h>
#include <stdio.h>
#include <syscalls.h>


#define PROC_BASE_ADDR ((void *) 0x40000000)

struct sched_process {
	volatile pid_t pid;
	void * symbol;
	void * stack;
	void * stack_base;
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



static struct sched_process idle_process;
static struct sched_process * active = NULL;
static struct sched_process * last = NULL;
static struct sched_process * terminated = NULL;

typedef struct {uint8_t data[4096];} page_t;

extern uint64_t _sched_init_stack(void * stack, void * symbol);

volatile pid_t max_pid = 0;
volatile pid_t fg_pid = 0;
static volatile int idle_active = 1;

static page_t idle_stack = {{0}};
static page_t idle_kstack = {{0}};

static page_t pages[SCHED_MAX_PROC * 2];
static int pidx = 0;
static struct sched_process processes[SCHED_MAX_PROC];
static int proc_idx = 0;

static uint64_t _sched_idle_process(void)
{
    while (1) _drool();
	return 0;
}

static struct sched_process * _sched_alloc_process(void)
{
	return &processes[proc_idx++];
}

void _sched_free_process(struct sched_process * process)
{

}

static page_t * _sched_alloc_pages(void * base, uint64_t size)
{
	page_t * ptr = &pages[pidx];
	pidx += size;
	return ptr;
}

static void _sched_free_pages(void * base, uint64_t size)
{

}


static void * get_stack_base(void * stack_base)
{
	return (void *)(
		(uint64_t) stack_base
		+ sizeof(page_t) 		//The size of the stack itself, 4KiB
		- sizeof(uint64_t)		//Begin at the top of the stack
	);
}


static uint64_t _sched_init_process(struct sched_process * process, void * symbol, void * stack, void * kernel_stack, int pages)
{
	process->symbol = symbol;
	process->stack_base = stack;
	process->stack = (void *) _sched_init_stack(get_stack_base(stack), symbol);
	process->kernel_stack = get_stack_base(kernel_stack);
	process->page_count = pages;
	return 0;
}


uint64_t sched_init(void)
{
	uint64_t symbol = (uint64_t) &_sched_idle_process;
	_sched_init_process(&idle_process,
					   (void *) symbol,
					   &idle_stack,
					   &idle_kstack,
					   1);
	return 0;
}

void print_proc()
{
	struct sched_process * process;
	for (process = active; process != NULL; process = process->next) {
		printf("%d->", process->pid);
		if (process == last) break;
	}
}

uint64_t sched_spawn_process(void * symbol, int size)
{
	struct sched_process * process = _sched_alloc_process();
	void * stack = _sched_alloc_pages(NULL, 1);
	void * kernel_stack = _sched_alloc_pages(NULL, 1);
	_sched_init_process(process, symbol, stack, kernel_stack, 1);
	process->pid = ++max_pid;
	
	if (!last) {
		last = process;
	}
	if (!active) {
		active = process;
	}
	process->next = active;
	last->next = process;
	last = process;
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


static void _sched_terminate_process(struct sched_process * defunct, unsigned short code)
{
	defunct->code	= code;
	defunct->next 	= terminated;
	terminated 		= defunct;
	_sched_free_pages(defunct->stack_base, defunct->page_count);
	_sched_free_pages(defunct->kernel_stack, 1);
}


uint64_t sched_terminate_process(pid_t pid, unsigned short retval)
{
	struct sched_process * process = active;
	struct sched_process * next = NULL;
	struct sched_process * prev = last;

	if (!active) return -1;
	
	for (process = active; process != NULL; process = process->next) {
		if (active == last) {
			_sched_terminate_process(process, retval);
			active = last = NULL;
			return 0;
		}
		if (process->pid == pid) {
			next = process->next;
			_sched_terminate_process(process, retval);
			prev->next = next;
			if (process == active) {
				active = next;
			}
			if (process == last) {
				last = next;
			}
			return 0;
		}
		prev = process;
	}
	return -1;
}

uint64_t sched_forkexec(pid_t parent_pid, void * new_symbol)
{
	//copy parent stack
	struct sched_process * child;
	struct sched_process * parent = NULL;
	for (parent = active; parent != active; parent = parent->next) {
		if (parent->pid == parent_pid) break;
	}
	if (!parent || parent->pid != parent_pid) return -1;
	child = _sched_alloc_process();
	child->pid = max_pid++;
	child->stack_base = _sched_alloc_pages(parent->stack_base, parent->page_count);
	memcpy(child->stack_base, parent->stack_base, sizeof(page_t) * parent->page_count);
	child->kernel_stack = _sched_alloc_pages(parent->stack_base, parent->page_count);
	child->stack = parent->stack;
	child->page_count = parent->page_count;
	child->next = active;
	last->next = child;
	last = child;
	sched_step_syscall_rax(child->stack, 0); //the parent will get it normally
	return child->pid;
}



uint64_t sched_add_page_to_current_process(void)
{
	void * page;
	if (!active) return -1;
	page = (void *)((uint64_t) active->stack_base - sizeof(page_t));
	page = _sched_alloc_pages(page, 1);
	if (page == NULL) {
		return -1;
	}
	active->stack_base = page;
	active->page_count++;
	return 0;
}

uint64_t sched_pick_process(void)
{
	if (!active) {
		return (uint64_t) idle_process.stack;
	}
	active = active->next;
	last = last->next;
	return (uint64_t) active->stack;
}
