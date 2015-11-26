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

void clearBSS(void * bssAddress, uint64_t bssSize)
{
	memset(bssAddress, 0, bssSize);
}

void * getStackBase(void)
{
	return (void*)(
		(uint64_t)&endOfKernel
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
	kbrd_install();
	vid_clr();

	// init vmm with 13 identity mapped pages
	vmm_initialize(init_mem((uint64_t)getStackBase() * 2+ sizeof(uint64_t))/4096);

	// -------- TEST -----------

	// void* dirs[3];
	// for (int i = 0; i < 4; i++) {
	// 	dirs[i] = vmm_alloc_pages(512*4096, 1);
	// }

	//vmm_print_bitmap(5);
	// vmm_free_pages(dirs[1], 512*4096);
	//vmm_print_bitmap(5);

	// vmm_alloc_pages(1, 1);
	//vmm_print_bitmap(5);

	// ------ TEST END ---------

	//sched_spawn_process((void *) test2);
	sched_spawn_process((void *) shellModuleAddress);
	
	/* Drop to environment */

	sched_drop_to_user();
	_sti();

    while (1) 
    	_drool();
    syscall_halt();
	return 0;
}
