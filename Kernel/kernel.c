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

extern uint8_t text;
extern uint8_t rodata;
extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x4000;
static const void * shellModuleAddress = (void*)0x400000;
static const void * test2 = (void*)0x800000;
static enum vid_term active_term = VID_PROC;

void clearBSS(void * bssAddress, uint64_t bssSize)
{
	memset(bssAddress, 0, bssSize);
}

void * getStackBase(void)
{
	return (void*)(
		(uint64_t)&bss
		+ PageSize * 8				//The size of the stack itself, 32KiB
		- sizeof(uint64_t)			//Begin at the top of the stack
	);
}

void * initializeKernelBinary(void)
{
	/* THIS HAS TO BE IN THE SAME ORDER THE PACKER PACKS IT OR
	 * IT BREAKS, LIKE, *REALLY* BAD.
	 */

	void * moduleAddresses[] = {
	    (void *) shellModuleAddress,
	    (void *) test2,
	};

	loadModules(&endOfKernelBinary, moduleAddresses);
	clearBSS(&bss, &endOfKernel - &bss);
	return getStackBase();
}

void pit_irq(int irq)
{
	tick_sound();
}

void kbrd_irq_with_activity(int irq)
{
	kbrd_irq(irq);
}

void handle_esc(void) {
	active_term = (active_term + 1) % 2;
	vid_show(active_term);
	vid_update();
}

int main(void)
{	
	_cli();
	sched_init();

	/* set up IDTs & int80h */
	install_syscall_handler((IntSysHandler) &int80h);
	install_hw_handler((IntHwHandler) &kbrd_irq_with_activity, INT_KEYB);
	install_hw_handler((IntHwHandler) &pit_irq, INT_PIT);
	install_interrupts();

	/* driver initialization */
	kbrd_install(&handle_esc);
	vid_clr(VID_PROC);
	vid_color(VID_SYSLOG, WHITE, BLUE);
	vid_clr(VID_SYSLOG);

	// init pmm
	init_mem((uint64_t)getStackBase() * 2+ sizeof(uint64_t));
	
	// init vmm with 1GB worth of vmm for the kernel
	void* bitmap;
	vmm_initialize(262144, &bitmap);
	
	// -------- TEST -----------

	 vmm_print_bitmap(513, 520);

	void* result;
	void* result2;

	vmm_alloc_pages(512*4096, MASK_WRITEABLE, &result);
	vmm_alloc_pages(512*4096, MASK_WRITEABLE, &result2);
	vmm_alloc_pages(512*4096, MASK_WRITEABLE, &result);

	vmm_print_bitmap(513, 520);
	
	vmm_free_pages(result2, 512*4096);

	vmm_print_bitmap(513, 520);

	// vmm_print_pt(513);
	// vmm_print_pt(514);

	vmm_alloc_pages(511*4096, MASK_WRITEABLE, &result);
	
	vmm_print_bitmap(513, 520);
	
	vmm_alloc_pages(1*4096, MASK_WRITEABLE, &result);
	
	vmm_print_bitmap(513, 520);

	vmm_alloc_pages(512*4096, MASK_WRITEABLE, &result);

	vmm_print_bitmap(513, 520);
	
	while(1);
	// ------ TEST END ---------

	//sched_spawn_process((void *) test2);
	sched_spawn_process((void *) shellModuleAddress);
	
	/* Drop to environment */

	printf("This might be useful:\n");
	printf("- Stack base: %x\n", (uint64_t)getStackBase() + sizeof(uint64_t));
	printf("- text: %x\n", &text);
	printf("- rodata: %x\n", &rodata);
	printf("- data: %x\n", &data);
	printf("- bss: %x\n", &bss);
	printf("- endOfKernelBinary: %x\n", &endOfKernelBinary);
	printf("- endOfKernel: %x\n", &endOfKernel);
	printf("Dropping to userland\n");
	sched_drop_to_user();
	_sti();

    while (1) 
    	_drool();
    syscall_halt();
	return 0;
}
