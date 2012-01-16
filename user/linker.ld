/*
 * Authors:
 *   http://wiki.osdev.org/Higher_Half_With_GDT
 *   Justin R. Wilson
 */   

OUTPUT_FORMAT("elf32-i386")

PHDRS
{
	headers PT_PHDR PHDRS ;
	text PT_LOAD FILEHDR PHDRS;
	data PT_LOAD;
	action_info PT_NOTE ;
}

SECTIONS
{
	. = SIZEOF_HEADERS;

	/* Start at 1M. */
	. = 0x00100000;

	.text :
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

	. = . + 0x1000;

	.data :
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