/*
 * Authors:
 *   http://wiki.osdev.org/Higher_Half_With_GDT
 *   Justin R. Wilson
 */   

OUTPUT_FORMAT("elf32-i386")

/* The "start" symbol is the entry point. */
ENTRY (start)

PHDRS
{
	setup PT_LOAD FLAGS (7) ;
	text PT_LOAD FLAGS (5);
	data PT_LOAD FLAGS (6);
	action_info PT_NOTE ;
}

SECTIONS
{
	/* Start at 1M. */
	. = 0x00100000;

	/* Place the setup code on a page boundary. */
	.setup ALIGN (0x1000) :
	{
		*(.setup)
	} :setup

	/* Jump forward by 0xC0000000. */
	. += 0xC0000000;


	/* Place the program text after the previous segment but logically after 0xC000000. */
	.text ALIGN (0x1000) : AT(ADDR(.text) - 0xC0000000)
	{
		text_begin = .;
		*(.text*)
		*(.gnu.linkonce.t*)

	} :text

	/* Place the initialized data on page boundary physically after the previous segment but logically after 0xC0000000. */
	.rodata : AT(ADDR(.rodata) - 0xC0000000)
	{
		ctors_begin = .;
		*(.ctor*)
		ctors_end = .;
		dtors_begin = .;
		*(.dtor*)
		dtors_end = .;
		*(.rodata*)
		*(.gnu.linkonce.r*)
		text_end = .;
	} :text

	/* Place the initialized data on page boundary physically after the previous segment but logically after 0xC0000000. */
	.data ALIGN (0x1000) : AT(ADDR(.data) - 0xC0000000)
	{
		data_begin = .;
		*(.data)
		*(.gnu.linkonce.d*)
	} :data

	/* Place the uninitialized data on a page boundary physically after the previous segment but logically after 0xC0000000. */
	.bss : AT(ADDR(.bss) - 0xC0000000)
	{
		bss_begin = .;
		*(COMMON)
		*(.bss*)
		*(.gnu.linkonce.b*)
		bss_end = .;
		data_end = .;
	} :data

	.action_info :
	{
		*(.action_info)
	} :action_info

	/DISCARD/ :
	{
		*(.comment)
		*(.eh_frame) /* Exception handling frames. */
	}
}