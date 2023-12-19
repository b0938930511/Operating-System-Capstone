
#include "system_call.h"
#include "mailbox.h"
#include "peripherals/mailbox.h"


volatile unsigned int  __attribute__((aligned(16))) my_mailbox[36];

void mbox_test() {

    printf("\nTest mbox\n");



    my_mailbox[0] = 7 * 4; // buffer size in bytes
    my_mailbox[1] = REQUEST_CODE;
    // tags begin
    my_mailbox[2] = GET_BOARD_REVISION; // tag identifier
    my_mailbox[3] = 4; // maximum of request and response value buffer's length.
    my_mailbox[4] = 8;
    my_mailbox[5] = 0; // value buffer
    // tags end
    my_mailbox[6] = MBOX_TAG_LAST;
    if (mbox_call(MBOX_CH_PROP, (unsigned int*)my_mailbox))
        printf("board revision number: 0x%x\n", my_mailbox[5]);
    else
        printf("can not get board revision number!\n");
    

    
    exit();
}