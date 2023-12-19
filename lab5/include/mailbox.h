#ifndef _MAIL_BOX_H
#define _MAIL_BOX_H

/* a properly aligned buffer */
extern volatile unsigned int mbox[36];

#define MBOX_REQUEST    0

/* channels */
#define MBOX_CH_POWER   0
#define MBOX_CH_FB      1
#define MBOX_CH_VUART   2
#define MBOX_CH_VCHIQ   3
#define MBOX_CH_LEDS    4
#define MBOX_CH_BTNS    5
#define MBOX_CH_TOUCH   6
#define MBOX_CH_COUNT   7
#define MBOX_CH_PROP    8

/* tags */
#define MBOX_TAG_GETSERIAL      0x10004
#define MBOX_TAG_LAST           0
#define GET_ARM_EMEORY      0x00010005




/* old macros from lab1 */
// #define GET_BOARD_REVISION  0x00010002
// #define GET_ARM_EMEORY      0x00010005
// #define REQUEST_CODE        0x00000000
// #define REQUEST_SUCCEED     0x80000000
// #define REQUEST_FAILED      0x80000001
// #define TAG_REQUEST_CODE    0x00000000
// #define END_TAG             0x00000000

// extern volatile unsigned int mailbox[8];

// int mailbox_call(unsigned char ch, volatile unsigned int *mbox);

void get_serial_number();
int mailbox_call(unsigned char ch, volatile unsigned int *mailbox);
void get_board_revision();
void get_ARM_memory();

#endif