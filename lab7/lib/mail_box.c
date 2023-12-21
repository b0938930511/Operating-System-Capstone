#include "peripherals/mail_box.h"
#include "mail_box.h"
#include "mini_uart.h"
#include "utils.h"

/* mailbox message buffer */
volatile unsigned int  __attribute__((aligned(16))) mailbox[36];

/**
 * Make a mailbox call. Returns 0 on failure, non-zero on success
 */
int mailbox_call(unsigned char ch, volatile unsigned int *mailbox, volatile unsigned int *mailbox_va)
{
    volatile unsigned int r = (((volatile unsigned int)((volatile unsigned long)mailbox)&~0xF) | (ch&0xF));
    /* wait until we can write to the mailbox */
    do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_FULL);
    /* write the address of our message to the mailbox with channel identifier */
    
    *MBOX_WRITE = r;
    
    /* now wait for the response */
    while(1) {
        /* is there a response? */
        do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_EMPTY);
        /* is it a response to our message? */
        if(r == *MBOX_READ) {
            /* is it a valid successful response? */
            return mailbox_va[1]==MBOX_RESPONSE;
        }
    }
    return 0;
}

// volatile unsigned int  __attribute__((aligned(16))) mailbox[8];

// int mailbox_call(unsigned char ch, volatile unsigned int *mbox) {
//     uart_printf("==========\n");
//     for (int i = 0; i < 8; ++i)
//         uart_printf("%x\n", mbox[i]);
//     uart_printf("==========\n");
//     /* 
//     1. Combine the message address (upper 28 bits) with channel number (lower 4 bits)
//     2. Check if Mailbox 0 status register’s full flag is set.
//     3. If not, then you can write to Mailbox 1 Read/Write register.
//     4. Check if Mailbox 0 status register’s empty flag is set.
//     5. If not, then you can read from Mailbox 0 Read/Write register.
//     6. Check if the value is the same as you wrote in step 1.
//     */
//     unsigned int msg = (((unsigned int)((unsigned long)mailbox) & ~0xF) | (ch & 0xF));
//     while (*MAILBOX_STATUS & MAILBOX_FULL) {}
//     *MAILBOX_WRITE = msg;
//     while (1) {
//         while (*MAILBOX_STATUS & MAILBOX_EMPTY) {}
//         if(msg == *MAILBOX_READ){
//             return mbox[1] == REQUEST_SUCCEED;
//         }
//     }
//     return 0;
// }

void get_board_revision() {
    mailbox[0] = 7 * 4; // buffer size in bytes
    mailbox[1] = MBOX_REQUEST;
    // tags begin
    mailbox[2] = GET_BOARD_REVISION; // tag identifier
    mailbox[3] = 4; // maximum of request and response value buffer's length.
    mailbox[4] = 8;
    mailbox[5] = 0; // value buffer
    // tags end
    mailbox[6] = MBOX_TAG_LAST;
    if (mailbox_call(MBOX_CH_PROP, mailbox, mailbox))
        uart_printf("board revision number: 0x%x\n", mailbox[5]);
    else
        uart_printf("can not get board revision number!\n");
}

void get_arm_memory() {
    mailbox[0] = 8 * 4; // buffer size in bytes
    mailbox[1] = MBOX_REQUEST;
    // tags begin
    mailbox[2] = GET_ARM_EMEORY; // tag identifier
    mailbox[3] = 8; // maximum of request and response value buffer's length.
    mailbox[4] = 8;
    mailbox[5] = 0; // value buffer
    mailbox[6] = 0; // value buffer
    // tags end
    mailbox[7] = MBOX_TAG_LAST;
    if (mailbox_call(MBOX_CH_PROP, mailbox, mailbox)) {
        uart_printf("base address: %x\n", mailbox[5]);
        uart_printf("memory size: %x\n", mailbox[6]);
    }
    else
        uart_printf("can not get memory info!\n");
}

void get_serial_number() {
    mailbox[0] = 8*4;                  // length of the message
    mailbox[1] = MBOX_REQUEST;         // this is a request message
    mailbox[2] = MBOX_TAG_GETSERIAL;   // get serial number command
    mailbox[3] = 8;                    // buffer size
    mailbox[4] = 8;
    mailbox[5] = 0;                    // clear output buffer
    mailbox[6] = 0;
    mailbox[7] = MBOX_TAG_LAST;
    if(mailbox_call(MBOX_CH_PROP, mailbox, mailbox))
        uart_printf("serial number is: %x%x\n", mailbox[5], mailbox[6]);
    else
        uart_printf("can not get serial number!\n");
}