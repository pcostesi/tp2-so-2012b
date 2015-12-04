#include <stdint.h>
#include <lib.h>
#include <moduleLoader.h>
#include <video.h>
#include <sound.h>
#include <interrupts.h>
#include <keyboard.h>
#include <rtc-driver.h>
#include <syscalls.h>
#include <sched.h>
#include <vmm.h>
#include <pmm.h>
#include <stdio.h>
#include <motd.h>

extern uint8_t text;
extern uint8_t rodata;
extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;
void * bitmap = NULL;

#define STACK_SIZE (0x4000 * 8)
#define INIT "shell.bin"
#define ALIGN4K(A) (void *)((((uint64_t)(A) >> 12) + 1) << 12)

static enum vid_term active_term = VID_PROC;
void print_log(void);

void clearBSS(void * bssAddress, uint64_t bssSize)
{
	memset(bssAddress, 0, bssSize);
}

void * getStackBase(void)
{
	return (void*)(
		(uint64_t)&bss
		+ STACK_SIZE				//The size of the stack itself, 32KiB
		- sizeof(uint64_t)			//Begin at the top of the stack
	);
}

uint8_t * get_module_zone(void)
{
	return ALIGN4K(&bss + STACK_SIZE);
}

uint8_t * get_safe_zone(void)
{
	//sorry
	return ALIGN4K(0xFFFFFFF);
}


void * initializeKernelBinary(void)
{
	memmove(get_module_zone(), &endOfKernelBinary, ldr_module_section_size(&endOfKernelBinary));
	clearBSS(&bss, STACK_SIZE);
	return getStackBase();
}

void panic(char * msg)
{
	puts("\nKERNEL PANIC\n");
	printf("Error: %s\n", msg);
	vid_show(VID_SYSLOG);
	vid_update();
	syscall_halt();
}

void pit_irq(int irq)
{
	tick_sound();
}

void handle_esc(void) {
	active_term = (active_term + 1) % 2;
	vid_show(active_term);
	vid_update();
}

void * get_entry_point(char * name)
{
	struct module_entry module;
	if (!ldr_module_load(get_module_zone(), name, &module)) return NULL;
	return module.start;
}

void vid_init(void)
{
	vid_color(VID_SYSLOG, GRAY, BLACK);
	vid_clr(VID_PROC);
	vid_color(VID_SYSLOG, WHITE, BLUE);
	vid_clr(VID_SYSLOG);
}


void print_log(void)
{
	printf("This might be useful:\n");
	printf("- Detected memory size: %d Mb\n", get_memory_size());
	printf("- base: %x\n", (uint64_t)getStackBase());
	printf("- text: %x\n", &text);
	printf("- rodata: %x\n", &rodata);
	printf("- data: %x\n", &data);
	printf("- bss: %x\n", &bss);
	printf("- endOfKernelBinary: %x\n", &endOfKernelBinary);
	printf("- endOfKernel: %x\n", &endOfKernel);
	printf("- Module zone: %x\n", get_module_zone());
	printf("- Safe zone:   %x\n", get_safe_zone());

	printf("Module shell.bin located at %x\n", get_entry_point("shell.bin"));
	printf("Module template.bin located at %x\n", get_entry_point("template.bin"));
}

int main(void)
{	
	struct module_entry init;
	struct module_entry template;

	vid_init();
	print_log();	
	// init pmm
	init_mem((uint64_t) get_safe_zone());
	
	// init vmm with 1GB worth of vmm for the kernel
	if (!vmm_initialize(&bitmap)) panic("Failed to start vmm.");
	sched_init(bitmap);

	/* set up IDTs & int80h */
	install_syscall_handler((IntSysHandler) &int80h);
	install_hw_handler((IntHwHandler) &kbrd_irq, INT_KEYB);
	install_hw_handler((IntHwHandler) &pit_irq, INT_PIT);
	install_interrupts();

	/* driver initialization */
	kbrd_install(&handle_esc);
	if (!ldr_module_load(get_module_zone(), INIT, &init)) {
		panic("Failed to load INIT. Halting.");
	}
	
	if (!ldr_module_load(get_module_zone(), "template.bin", &template)) {
		panic("Failed to load Template. Halting.");
	}

	handle_esc();
	memcpy((void *) 0x4100000, template.start, template.size);
	sched_spawn_module(&template, (void *) 0x4100000);

	memcpy((void *) 0x4000000, init.start, init.size);
	printf("init size is %d\n", init.size);
	sched_spawn_module(&init,	  (void *) 0x4000000);
	printf("Dropping to userland.\n");
	/* Drop to environment */
	sched_drop_to_user();
    while (1) 
    	_drool();
    syscall_halt();
	return 0;
}
