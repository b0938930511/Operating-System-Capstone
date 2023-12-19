ENTRY(_start_relo)
SECTIONS
{
	. = 0x60000;
	_stext = .;
	.text : { *(.text.relo)
	_bl_entry = .;
	*(.text.boot)
	*(.text) 
	*(.rodata)}
	. = ALIGN(0x1000);
	_etext = .;
	_sdata = .;
	.data : { *(.data) }
	. = ALIGN(0x1000);
	_edata = .;
	bss_begin = .;
	.bss : { *(.bss) } 
	. = ALIGN(0x1000);
	bss_end = .;
	_blsize = bss_end - _stext;
}
