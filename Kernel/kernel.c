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

extern uint8_t text;
extern uint8_t rodata;
extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;
extern uint8_t bss2;

extern void _cli(void);
extern void _sti(void);
extern void _halt(void);
extern void _drool(void);

static const uint64_t PageSize = 0x4000;
static const void * shellModuleAddress = (void*)0x400000;

typedef int (*EntryPoint)(unsigned int pcount, char * pgname[], void * pgptrs[]);

void clearBSS(void * bssAddress, uint64_t bssSize)
{
	memset(bssAddress, 0, bssSize);
}

void * getStackBase()
{
	return (void*)(
		//(uint64_t)&endOfKernel
		(uint64_t)&bss2
		+ PageSize * 8				//The size of the stack itself, 32KiB
		- sizeof(uint64_t)			//Begin at the top of the stack
	);
}

void * initializeKernelBinary()
{
	/* THIS HAS TO BE IN THE SAME ORDER THE PACKER PACKS IT OR
	 * IT BREAKS, LIKE, *REALLY* BAD.
	 */

	void * moduleAddresses[] = {
	    (void *) shellModuleAddress
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
	syscall_wake();
	kbrd_irq(irq);
}

int main()
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

	/* Drop to environment */
	sched_spawn_process((void *) shellModuleAddress);
	_sti();
	
    while (1) _drool();
    syscall_write(1, "banana", 6);
    _halt();
	return 0;
}
