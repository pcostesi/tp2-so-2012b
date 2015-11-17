OUTPUT_FORMAT("binary")
ENTRY(loader)
SECTIONS
{
	.text 0x100000 :
	{
		text = .;
		*(.text*)
		. = ALIGN(0x4000);
		rodata = .;
		*(.rodata*)
	}
	.data ALIGN(0x4000) : AT(ADDR(.data))
	{
		data = .;
		*(.data*)
		endOfKernelBinary = .;
	}
	.bss ALIGN(0x4000) : AT(ADDR(.bss))
	{
		bss = .;
		*(.bss*)
		*(EXCLUDE_FILE (*.o) COMMON)
	}
	. = ALIGN(0x1000);
	.bss2 ALIGN(0x4000) : AT(ADDR(.bss2))
	{
		bss2 = .;
		*(.bss2*)
		*(EXCLUDE_FILE (*.o) COMMON)
	}
	. = ALIGN(0x1000);
	.idle_stack ALIGN(0x1000) : AT(ADDR(.idle_stack))
	{
		idle_stack = .;
		*(.idle_stack*)
		*(EXCLUDE_FILE (*.o) COMMON)
	}
	. = ALIGN(0x1000);
	.idle_kernel_stack ALIGN(0x1000) : AT(ADDR(.idle_kernel_stack))
	{
		idle_kernel_stack = .;
		*(.idle_kernel_stack*)
		*(EXCLUDE_FILE (*.o) COMMON)
	}
	. = ALIGN(0x1000);
	endOfKernel = .;
}
