#include <sched.h>
#include <lib.h>
#include <stdio.h>
#include <syscalls.h>
#include <vmm.h>


#define PROC_BASE_ADDR ((void *) 0x60000000)
#define PROC_BASE_STACK ((void *) 0x50000000)
#define OK_OR_PANIC(A, B) do { if (!(A)) panic(B); } while (0)

struct sched_process {
	volatile pid_t pid;
	void * symbol;
	void * stack;
	void * stack_base;
	size_t page_count;
	void * pagetable; //the bitmap
	uint64_t cr3; //the cr3
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
	void * page;
	size = sizeof(page_t) * size;
	if (base)
		vmm_alloc_pages_from(base, size, MASK_WRITEABLE, &page);
	else
		vmm_alloc_pages(size, MASK_WRITEABLE, &page);
	printf("Allocated %x @ %x\n", size, page);
	return (page_t *) page;
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


static void _sched_init_process(struct sched_process * process, void * symbol, void * stack, void * kernel_stack, int pages)
{
	process->symbol = symbol;
	process->stack_base = stack;
	process->stack = (void *) _sched_init_stack(get_stack_base(stack), symbol);
	process->kernel_stack = get_stack_base(kernel_stack);
	process->page_count = pages;
}


uint64_t sched_init(void * pagetable)
{
	uint64_t symbol = (uint64_t) &_sched_idle_process;
	_sched_init_process(&idle_process,
					   (void *) symbol,
					   &idle_stack,
					   &idle_kstack,
					   1);
	idle_process.cr3 = _read_cr3();
	idle_process.pagetable = pagetable;
	return 0;
}

static void _sched_load_module(struct module_entry * entry, struct sched_process * proc)
{
	int res = 0;
	printf("Current cr3 is %x\n", _read_cr3());
	OK_OR_PANIC(vmm_initialize(&proc->pagetable), "Failed to start page dir");
	// create a new page table
	printf("Loading %s <%d bytes> into %x\n", entry->name, entry->size, PROC_BASE_ADDR);
	res = vmm_alloc_pages_from(PROC_BASE_ADDR, entry->size, MASK_WRITEABLE, &proc->symbol);
	OK_OR_PANIC(res, "Failed to alloc pages");
	memcpy(proc->symbol, entry->start, entry->size);
	printf("Loaded %s into %x\n", entry->name, proc->symbol);
	proc->cr3 = _read_cr3();
	printf("CR3 is %x, bitmap is %x\n", proc->cr3, proc->pagetable);
}

uint64_t sched_spawn_module(struct module_entry * entry)
{
	struct sched_process * process = _sched_alloc_process();
	_sched_load_module(entry, process);
	void * stack = _sched_alloc_pages(PROC_BASE_STACK, 1);
	void * kernel_stack = _sched_alloc_pages(NULL, 1);
	_sched_init_process(process, process->symbol, stack, kernel_stack, 1);
	process->pid = ++max_pid;

	if (active) {
		vmm_switch_process((void *) active->cr3, active->pagetable);
	} else {
		vmm_switch_process((void *) idle_process.cr3, idle_process.pagetable);
	}

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
		vmm_switch_process((void *) idle_process.cr3, idle_process.pagetable);
		idle_process.stack = (void *) stack;
		return (uint64_t) idle_process.kernel_stack;
	}

	vmm_switch_process((void *) active->cr3, active->pagetable);
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
	terminated 	= defunct;
	_sched_free_pages(defunct->stack_base, defunct->page_count);
	//XXX: BUG
	//_sched_free_pages(defunct->kernel_stack, 1);
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
		vmm_switch_process((void *) idle_process.cr3, idle_process.pagetable);
		return (uint64_t) idle_process.stack;
	}
	active = active->next;
	last = last->next;
        vmm_switch_process((void *) active->cr3, active->pagetable);
	return (uint64_t) active->stack;
}
