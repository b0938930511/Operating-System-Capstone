SECTIONS
{	
	. = 0xffff000000000000;
    . += 0x80000;
	.text.boot : { *(.text.boot) }
	.text : { *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	. = ALIGN(0x8);
	__bss_start = .;
	.bss : { *(.bss*) }
	__bss_end = .; 
	__stk_start = __bss_end + (1<<16);
}

__bss_size = (__bss_end - __bss_start) >> 3;