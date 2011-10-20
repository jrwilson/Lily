/*
 * Authors:
 *   http://wiki.osdev.org/Bare_bones
 *   Justin R. Wilson
 */   

/* The "loader" symbol in the entry point. */
ENTRY (loader)

SECTIONS
{
	/* Start at 1M. */
	. = 0x00100000;

	/* Place the program text aligned on a page boundary. */
	.text ALIGN (0x1000) :
	{
		*(.text)
	}

	/* Place the read-only data aligned on a page boundary.
	   Why is there a * after rodata?
	 */
	.rodata ALIGN (0x1000) :
	{
		*(.rodata*)
	}

	/* Place the initialized data on a page boundary. */
	.data ALIGN (0x1000) :
	{
		*(.data)
	}

	/* Include the common segment (uninitialized data). */
	.bss :
	{
		/* Define a symbol for the beginning. */
		begin_bss = .;
		*(COMMON)
		*(.bss)
		/* Defind a symbol for the end. */
		end_bss = .;
	}
}