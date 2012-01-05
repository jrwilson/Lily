/*
 * Authors:
 *   http://wiki.osdev.org/Higher_Half_With_GDT
 *   Justin R. Wilson
 */   

OUTPUT_FORMAT("elf32-i386")

PHDRS
{
	text PT_LOAD FLAGS (5);
	data PT_LOAD FLAGS (6);
	action_info PT_NOTE ;
}

SECTIONS
{
	/* Start at 1M. */
	. = 0x00100000;

	.text ALIGN (0x1000) :
	{
		*(.text*)
		*(.gnu.linkonce.t*)
	} :text

	.rodata :
	{
		ctors_begin = .;
		*(.ctor*)
		ctors_end = .;
		dtors_begin = .;
		*(.dtor*)
		dtors_end = .;
		*(.rodata*)
		*(.gnu.linkonce.r*)
	} :text

	.data ALIGN (0x1000) :
	{
		data_begin = .;
		*(.data)
		*(.gnu.linkonce.d*)
	} :data

	.bss :
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