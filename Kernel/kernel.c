#include <stdint.h>
#include <string.h>
#include <lib.h>
#include <moduleLoader.h>
#include <naiveConsole.h>
#include <video.h>
#include <interrupts.h>
#include <keyboard.h>
#include <rtc-driver.h>
#include <syscalls.h>

extern uint8_t text;
extern uint8_t rodata;
extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;

static void * shellModuleAddress = (void*)0x40000;
static void * sampleDataModuleAddress = (void*)0x60000;
static void * sampleCodeModuleAddress = (void*)0x80000;

typedef int (*EntryPoint)(unsigned int pcount, char * pgname[], void * pgptrs[]);

void clearBSS(void * bssAddress, uint64_t bssSize)
{
	memset(bssAddress, 0, bssSize);
}

void * getStackBase()
{
	return (void*)(
		(uint64_t)&endOfKernel
		+ PageSize * 8				//The size of the stack itself, 32KiB
		- sizeof(uint64_t)			//Begin at the top of the stack
	);
}

void * initializeKernelBinary()
{
	void * moduleAddresses[] = {
	    shellModuleAddress,
	    sampleDataModuleAddress,
	    sampleCodeModuleAddress
	};

	char * moduleNames[] = {
		"shell",
		"sampleDataModule",
		"sampleCodeModule"
	};


	ncPrint("[x64BareBones]");
	ncNewline();
	ncNewline();

	ncPrint("[Loading modules]");
	ncNewline();

	loadModules(&endOfKernelBinary, moduleAddresses);
	ncPrint("[Done]");
	ncNewline();
	ncNewline();

	ncPrint("[Initializing kernel's binary]");
	ncNewline();

	clearBSS(&bss, &endOfKernel - &bss);

	ncPrint("  text: 0x");
	ncPrintHex((uint64_t)&text);
	ncNewline();
	ncPrint("  rodata: 0x");
	ncPrintHex((uint64_t)&rodata);
	ncNewline();
	ncPrint("  data: 0x");
	ncPrintHex((uint64_t)&data);
	ncNewline();
	ncPrint("  bss: 0x");
	ncPrintHex((uint64_t)&bss);
	ncNewline();

	ncPrint("[Done]");
	ncNewline();
	ncNewline();
	return getStackBase();
}

int main()
{	
	/* driver initialization */
	/* set up IDTs & int80h */

	install_syscall_handler((IntSysHandler) &int80h);
	install_hw_handler((IntHwHandler) &kbrd_irq, INT_KEYB);
	install_interrupts();

	kbrd_install();
	vid_clr();
	
	/* timer/"proto-scheduler" initialization */
	/* call shell (how do we call as userspace?) */

	/*
	ncPrint("[Kernel Main]");
	ncNewline();
	ncPrint("  Sample code module at 0x");
	ncPrintHex((uint64_t)sampleCodeModuleAddress);
	ncNewline();
	*/
    /*
	ncPrint("  Calling the sample code module returned: ");
	ncPrintHex(((EntryPoint)sampleCodeModuleAddress)());
     */
	/*
	ncNewline();
	ncNewline();

	ncPrint("  Sample data module at 0x");
	ncPrintHex((uint64_t)sampleDataModuleAddress);
	ncNewline();
	ncPrint("  Sample data module contents: ");
	ncPrint((char*)sampleDataModuleAddress);
	ncNewline();
	ncNewline();

	ncPrint("[Finished]");
	*/
	void * moduleAddresses[] = {
	    shellModuleAddress,
	    sampleDataModuleAddress,
	    sampleCodeModuleAddress
	};

	char * moduleNames[] = {
		"shell",
		"sampleDataModule",
		"sampleCodeModule"
	};
<<<<<<< HEAD
    ((EntryPoint)shellModuleAddress)(sizeof(moduleNames) / sizeof(char *), moduleNames, moduleAddresses);
||||||| merged common ancestors

    ((EntryPoint)shellModuleAddress)(sizeof(moduleNames) / sizeof(char *), moduleNames, moduleAddresses);

=======

	uint8_t modules = sizeof(moduleNames) / sizeof(char *);
    ((EntryPoint)shellModuleAddress)(modules, moduleNames, moduleAddresses);

>>>>>>> add entrypoints
    vid_print("\nHalting", 8);
	while (1);
	return 0;
}
