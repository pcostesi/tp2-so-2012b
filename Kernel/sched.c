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
	char name[128];
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

typedef struct {uint8_t data[4096];} page_t;

extern void * _sched_init_stack(void * stack, void * symbol);

volatile pid_t max_pid = 0;
volatile pid_t fg_pid = 0;
static struct sched_process processes[SCHED_MAX_PROC];
static int proc_idx = 0;

static uint64_t _sched_idle_process(void)
{
    while (1) {
    	syscall_write(1, "*IDLING!* ", 10);
    	_drool();
    }
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
	memset(page, 0, size);
	return (page_t *) page;
}

static void _sched_free_pages(void * base, uint64_t size)
{

}


static void * get_stack_base(void * stack_base, int pages)
{
	return (void *)(
		(uint64_t) stack_base
		+ sizeof(page_t) * pages	//The size of the stack itself, 4KiB
		- sizeof(uint64_t)			//Begin at the top of the stack
	);
}


static void _sched_init_process(struct sched_process * process, void * symbol, void * stack, void * kernel_stack, int pages)
{
	process->symbol = symbol;
	process->stack_base = stack;
	process->stack = _sched_init_stack(get_stack_base(stack, pages), symbol);
	process->kernel_stack = get_stack_base(kernel_stack, 1);
	process->page_count = pages;
}

void _sched_print_proclist(void)
{
	puts("processes: ");
	volatile struct sched_process * p;
	if (!active) {
		puts("<idle>\n");
		return;
	}
	for (p = active; p->next != NULL; p = p->next) {
		printf("%s (%d) ", p->name, p->pid);
		if (p->next == active) break;		
	}
	puts("\n");
}


uint64_t sched_init(void * pagetable)
{
	uint64_t symbol = (uint64_t) &_sched_idle_process;

	void * stack = _sched_alloc_pages(PROC_BASE_STACK, 1);
	void * kernel_stack = _sched_alloc_pages(NULL, 1);

	_sched_init_process(&idle_process,
					   (void *) symbol,
					   stack,
					   kernel_stack,
					   1);
	idle_process.pagetable = pagetable;
	idle_process.next = &idle_process;
	memcpy(idle_process.name, "idle", 5);
	return 0;
}

static void _sched_load_module(struct module_entry * entry, struct sched_process * proc)
{	
	void * stack = _sched_alloc_pages(NULL, 16);
	void * kernel_stack = _sched_alloc_pages(NULL, 1);

	_sched_init_process(proc, proc->symbol, stack, kernel_stack, 16);
}

uint64_t sched_spawn_module(struct module_entry * entry, void * symbol)
{
	struct sched_process * process = _sched_alloc_process();
	struct sched_process * last;

	process->pid = ++max_pid;
	process->symbol = symbol;
	memcpy(process->name, entry->name, sizeof(process->name));
	_sched_load_module(entry, process);

	if (!active) {
		active = process;
	}
	process->next = active;
	last = active;

	while (last && last->next != active) {
		last = last->next;
	}
	last->next = process;
	return process->pid;
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
	_sched_free_pages(defunct->stack_base, defunct->page_count);
	_sched_free_pages(defunct->kernel_stack, 1);
}


uint64_t sched_terminate_process(pid_t pid, unsigned short retval)
{
	struct sched_process * process = active;
	struct sched_process * next = NULL;
	struct sched_process * prev = NULL;

	if (!active) return -1;

	for (prev = active; prev != NULL; prev = prev->next) {
		if (prev->next == active) break;
	}
	
	for (process = active; process != NULL; process = process->next) {
		if (active == active->next && process->pid == pid) {
			_sched_terminate_process(process, retval);
			active = NULL;
			return 0;
		}
		if (process->pid == pid) {
			next = process->next;

			prev->next = next;
			_sched_terminate_process(process, retval);

			return 0;
		}
		prev = process;
		if (process->next == active) break;
	}
	return -1;
}

uint64_t sched_forkexec(pid_t parent_pid, void * new_symbol)
{
	return -1;
}

uint64_t sched_get_process(void)
{
	if (!active) {
		return (uint64_t) idle_process.stack;
	}
	return (uint64_t) active->stack;
}


void show_stack(uint64_t * sp)
{
	for (int i = 0; i < 17; i++)
		printf("%d\t%x\n", i, sp[i]);
	printf("%x\n", sp);
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


uint64_t sched_switch_to_user_stack(uint64_t stack)
{
	if (!active) {
		idle_process.kernel_stack = (void *) stack;
		return (uint64_t) idle_process.stack;
	}

	active->kernel_stack = (void *) stack;
	active = active->next;
	return (uint64_t) active->stack;
}
