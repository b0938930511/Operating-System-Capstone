.section ".data"

.section ".text"

.align 11 // aligned to 0x800 (2^11)
.global _exception_vector_table
_exception_vector_table:
    b TODO // branch to a handler function.
    .align 7 // entry size is 0x80 (2^7), .align will pad 0
    b TODO
    .align 7
    b TODO
    .align 7
    b TODO
    .align 7

    b curr_sync
    .align 7
    b curr_iqr
    .align 7
    b TODO
    .align 7
    b TODO
    .align 7

    b lower_sync
    .align 7
    b lower_iqr
    .align 7
    b TODO
    .align 7
    b TODO
    .align 7

    b TODO
    .align 7
    b TODO
    .align 7
    b TODO
    .align 7
    b TODO

// save general registers to stack
.macro save_all
    sub sp, sp, 36 * 8
    stp x0, x1, [sp ,16 * 0]
    stp x2, x3, [sp ,16 * 1]
    stp x4, x5, [sp ,16 * 2]
    stp x6, x7, [sp ,16 * 3]
    stp x8, x9, [sp ,16 * 4]
    stp x10, x11, [sp ,16 * 5]
    stp x12, x13, [sp ,16 * 6]
    stp x14, x15, [sp ,16 * 7]
    stp x16, x17, [sp ,16 * 8]
    stp x18, x19, [sp ,16 * 9]
    stp x20, x21, [sp ,16 * 10]
    stp x22, x23, [sp ,16 * 11]
    stp x24, x25, [sp ,16 * 12]
    stp x26, x27, [sp ,16 * 13]
    stp x28, x29, [sp ,16 * 14]
    str x30, [sp, 16 * 15]

    mrs x19, sp_el0
	mrs x20, elr_el1
	mrs x21, spsr_el1
	stp x19,x20, [sp, 16 * 16]
	str x21, [sp, 16 * 17]
.endm

// load general registers from stack
.macro load_all
    ldp x21, x20, [sp, 16 * 17]
	ldp x19, x20, [sp, 16 * 16]
	msr spsr_el1, x21
	msr elr_el1, x20
	msr sp_el0, x19

    ldp x0, x1, [sp ,16 * 0]
    ldp x2, x3, [sp ,16 * 1]
    ldp x4, x5, [sp ,16 * 2]
    ldp x6, x7, [sp ,16 * 3]
    ldp x8, x9, [sp ,16 * 4]
    ldp x10, x11, [sp ,16 * 5]
    ldp x12, x13, [sp ,16 * 6]
    ldp x14, x15, [sp ,16 * 7]
    ldp x16, x17, [sp ,16 * 8]
    ldp x18, x19, [sp ,16 * 9]
    ldp x20, x21, [sp ,16 * 10]
    ldp x22, x23, [sp ,16 * 11]
    ldp x24, x25, [sp ,16 * 12]
    ldp x26, x27, [sp ,16 * 13]
    ldp x28, x29, [sp ,16 * 14]
    ldr x30, [sp, 16 * 15]
    add sp, sp, 36 * 8
.endm

.global TODO
TODO:
    save_all    //save_all
    bl error_handler
    load_all     //load_all
    eret


  //Synchronous from lower level
lower_sync:
	save_all
    mov x0, sp	// points to the kernel stack of the running user thread
	bl lower_sync_handler
	load_all
	eret

//IRQ from lower level
lower_iqr:
	save_all
	bl lower_iqr_handler
	load_all
	eret

curr_sync:
	save_all
	bl curr_sync_handler
	load_all
	eret

curr_iqr:
	save_all
	bl curr_iqr_handler
	load_all
	eret


.global child_return_from_fork
child_return_from_fork:
    load_all
    eret


