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



master:
	bl  _from_el2_to_el1

	// set_exception_vector_table
	adr x0, _exception_vector_table
  	msr vbar_el1, x0

	adr	x10, bss_begin // clear bss
	adr	x11, bss_end
	sub	x11, x11, x10
	bl 	memzero

	mov	sp, #0x400000 
	bl	kernel_main
	b 	proc_hang		// should never come here

proc_hang: 
	b 	proc_hang

_from_el2_to_el1:
    mov x0, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x0
    mov x0, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x0
    msr elr_el2, lr

    mov     x0, #(3 << 20)
    msr     cpacr_el1, x0
    eret // return to EL1


.global _dtb_ptr
.section .data
_dtb_ptr: .dc.a 0x0
