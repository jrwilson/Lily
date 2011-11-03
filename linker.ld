/*
 * Authors:
 *   http://wiki.osdev.org/Higher_Half_With_GDT
 *   Justin R. Wilson
 */   

OUTPUT_FORMAT("elf32-i386")

/* The "start" symbol is the entry point. */
ENTRY (start)

SECTIONS
{
	/* Start at 1M. */
	. = 0x00100000;

	/* Place the setup code on a page boundary. */
	.setup ALIGN (0x1000) :
	{
		*(.setup)
	}

	/* Jump forward by 0xC0000000. */
	. += 0xC0000000;


	/* Place the program text after the previous segment but logically after 0xC000000. */
	.text ALIGN (0x1000) : AT(ADDR(.text) - 0xC0000000)
	{
		kernel_begin = .;	
		*(.text)
	}

	/* Place read-only data on page boundary physically after the previous segment but logically after 0xC0000000. */
	.rodata ALIGN (0x1000) : AT(ADDR(.rodata) - 0xC0000000)
	{
		*(.rodata)
	}

	/* Place the initialized data on page boundary physically after the previous segment but logically after 0xC0000000. */
	.data ALIGN (0x1000) : AT(ADDR(.data) - 0xC0000000)
	{
		*(.data)
	}

	/* Place the uninitialized data on a page boundary physically after the previous segment but logically after 0xC0000000. */
	.bss ALIGN (0x1000) : AT(ADDR(.bss) - 0xC0000000)
	{
		*(.bss)
		kernel_end = .;
	}

}