#include "system_call.h"
#include "mail_box.h"


void mbox_test() {
    /* test mbox_call */
    printf("\nTest mbox\n");
    // my_mailbox[0] = 7 * 4;
    // my_mailbox[1] = REQUEST_CODE;
    // my_mailbox[2] = GET_BOARD_REVISION;
    // my_mailbox[3] = 4;
    // my_mailbox[4] = TAG_REQUEST_CODE;
    // my_mailbox[5] = 0;
    // my_mailbox[6] = END_TAG;
    // int status = mbox_call(8, my_mailbox);
    // printf("\rboard revision: 0x");
    // printf("%x\n", my_mailbox[5]);  // it should be 0xa020d3 for rpi3 b+
    // if (status)
    //     printf("success\n");
    // else
    //     printf("fail\n");

    volatile unsigned int  __attribute__((aligned(16))) my_mailbox[36];
    my_mailbox[0] = 7 * 4; // buffer size in bytes
    my_mailbox[1] = MBOX_REQUEST;
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
    

    /* test kill */
    // int child_id[3];
    // int parent_id = 0;
    // for (int i = 0; i < 3; ++i) {
    //     int id = fork();
    //     if (!id) // child
    //         break;
    //     child_id[i] = id;
    //     parent_id = get_pid();
    // }
    // if (get_pid() == parent_id) {
    //     for (int i = 0; i < 1000000; ++i) {}
    // }
    // else {
    //     printf("Thread %d before kill\n", get_pid());
    //     while (1) {}
    // }
    
    exit();
}
