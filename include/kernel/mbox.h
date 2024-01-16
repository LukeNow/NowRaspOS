#ifndef __MBOX_H
#define __MBOX_H

#define MBOX_REQUEST 0

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


typedef enum {
    MBOX_TAG_BOARD_MODEL = 0x10001,
    MBOX_TAG_BOARD_REV = 0x10002,
    MBOX_TAG_GETSERIAL = 0x10004,
    MBOX_TAG_ARMMEM = 0x10005,
    MBOX_TAG_VCMEM = 0x10006,
    MBOX_TAG_LAST = 0
} mbox_prop_tag_t;

typedef enum {
    REQUEST = 0x00000000,
    RESPONSE_SUCCESS = 0x80000000,
    RESPONSE_ERROR = 0x80000001
} mbox_res_code_t;

typedef struct {
    mbox_prop_tag_t tag;
    uint32_t size;
    mbox_res_code_t res_code;
    uint32_t *buffer; // Must align to 32bits
} mbox_tag_t;

typedef struct {
    uint32_t size;                     
    mbox_res_code_t res_code;
    mbox_tag_t *tags;             
} mbox_prop_msg_buf_t;

/* tags */
#define MBOX_TAG_BOARD_MODEL    0x10001
#define MBOX_TAG_BOARD_REV      0x10002
#define MBOX_TAG_GETSERIAL      0x10004
#define MBOX_TAG_ARMMEM         0x10005
#define MBOX_TAG_VCMEM          0x10006
#define MBOX_TAG_LAST           0

typedef uint32_t mbox_message_t;
typedef uint32_t mbox_status_t;


void mbox_send(mbox_message_t msg, int channel);
int mbox_read(uint32_t *mbox, uint32_t channel);
mbox_message_t mbox_make_msg(uint32_t *mbox, uint32_t ch);


#endif