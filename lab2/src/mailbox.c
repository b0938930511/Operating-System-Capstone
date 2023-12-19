#include "peripherals/mailbox.h"
#include "StringUtils.h"
#include "mini_uart.h"
/* mailbox message buffer */
volatile unsigned int  __attribute__((aligned(16))) mbox[36];

int mailbox_call()
{
    unsigned int readchannel = (((unsigned int)((unsigned long)&mbox)&~0xF) | (0x8 & 0xF));
    /* wait until we can write to the mailbox */
    while(*MAILBOX_STATUS & MAILBOX_FULL){}
    /* write the address of our message to the mailbox with channel identifier */
    *MAILBOX_WRITE = readchannel;
    /* now wait for the response */
    while(1) {
        /* is there a response? */
        while(*MAILBOX_STATUS & MAILBOX_EMPTY){}
        /* is it a response to our message? */
        if(readchannel == *MAILBOX_READ)
            /* is it a valid successful response? */
            return mbox[1]==MAILBOX_RESPONSE;
    }
    return 0;
}

void get_board_revision() {
    uart_send_string("\rget_board_revision\r\n");
    mbox[0] = 7 * 4; // buffer size in bytes
    mbox[1] = REQUEST_CODE;
    // tags begin
    mbox[2] = GET_BOARD_REVISION; // tag identifier
    mbox[3] = 4; // maximum of request and response value buffer's length.
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0; // value buffer
    // tags end
    mbox[6] = END_TAG;
    unsigned int a = mailbox_call(); // message passing procedure call, you should implement it following the 6 steps provided above.
    char str[256];
    // printf("0x%x\n", mailbox[5]); // it should be 0xa020d3 for rpi3 b+
    utils_uint2str_hex(mbox[5], str);
    uart_send_string(str);
    uart_send_string("\r\n");
}

void get_ARM_memory() {
    uart_send_string("\rget_ARM_memory\r\n");
    mbox[0] = 8 * 4; // buffer size in bytes
    mbox[1] = REQUEST_CODE;
    // tags begin
    mbox[2] = GET_ARM_MEMORY; // tag identifier
    mbox[3] = 8; // maximum of request and response value buffer's length.
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0; // value buffer
    mbox[6] = 0;        // tags end
    mbox[7] = END_TAG;
    unsigned int a = mailbox_call();  // message passing procedure call, you should implement it following the 6 steps provided above.
    char str[256];
    uart_send_string("base\r\n");
    utils_uint2str_hex(mbox[5], str);
    uart_send_string(str);
    uart_send_string("\r\n");
    uart_send_string("size\r\n");
    utils_uint2str_hex(mbox[6], str);
    uart_send_string(str);
    uart_send_string("\r\n");
}