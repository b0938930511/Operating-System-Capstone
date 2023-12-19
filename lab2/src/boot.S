#include "mm.h"

.section ".text.kernel"

.globl _start
_start:
	ldr    x1, =_dtb_ptr
    str    x0, [x1]
	mrs	x20, mpidr_el1		
	and	x20, x20,#0xFF		// Check processor id
	cbz	x20, master		// Hang for all non-primary CPU
	b	proc_hang

proc_hang: 
	b 	proc_hang

master:
	adr	x10, bss_begin
	adr	x11, bss_end
	sub	x11, x11, x0
	bl 	memzero

	mov	sp, #0x400000 
	bl	kernel_main
	b 	proc_hang		// should never come here


.global _dtb_ptr
.section .data
_dtb_ptr: .dc.a 0x0
