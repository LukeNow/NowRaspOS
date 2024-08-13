#include <stdint.h>
#include <stddef.h>
#include <kernel/mbox.h>
#include <kernel/gpio.h>
#include <common/common.h>

#define VIDEOCORE_MBOX  (MMIO_BASE + 0x0000B880)
#define MBOX_READ       ((volatile uint32_t*)(VIDEOCORE_MBOX + 0x0))
#define MBOX_POLL       ((volatile uint32_t*)(VIDEOCORE_MBOX + 0x10))
#define MBOX_SENDER     ((volatile uint32_t*)(VIDEOCORE_MBOX + 0x14))
#define MBOX_STATUS     ((volatile uint32_t*)(VIDEOCORE_MBOX + 0x18))
#define MBOX_CONFIG     ((volatile uint32_t*)(VIDEOCORE_MBOX + 0x1C))
#define MBOX_WRITE      ((volatile uint32_t*)(VIDEOCORE_MBOX + 0x20))
#define MBOX_RESPONSE   0x80000000
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000

static size_t mbox_get_tag_len(mbox_prop_tag_t tag)
{
    switch (tag) {
        case MBOX_TAG_BOARD_MODEL:
        case MBOX_TAG_BOARD_REV:
        return 4;

        case MBOX_TAG_GETSERIAL:
        case MBOX_TAG_ARMMEM:
        case MBOX_TAG_VCMEM:
        return 8;
    }
}

void mbox_send(mbox_message_t msg, int channel)
{
    //msg = ((uint32_t)((uint64_t)&mbox) & ~0xF) | (channel & 0xF);

    while (*MBOX_STATUS & MBOX_FULL) {
        CYCLE_WAIT(3);
    }

    *MBOX_WRITE = (*(uint32_t *)&msg);
}

int mbox_read(uint32_t *mbox, uint32_t ch)
{
    mbox_message_t msg;

    while (1) {
        while (*MBOX_STATUS & MBOX_EMPTY) {
            CYCLE_WAIT(3);
        }

        *(uint32_t *)&msg = *MBOX_READ;

        //if (msg != 0 && (msg & 0xF) == channel)
        if (*(uint32_t *)&msg != 0)
            return mbox[1] == MBOX_RESPONSE;
    }

    return 0;
}

mbox_message_t mbox_make_msg(uint32_t *mbox, uint32_t ch)
{
    // CHECK ALIGNMENT

    return (mbox_message_t) (((uint64_t)mbox & ~0xF) | (ch & 0xF));
}

void mbox_set_ch(mbox_message_t *msg, uint32_t ch)
{
    *msg = (mbox_message_t)(((uint32_t)(*msg)) | (ch & 0xF));
}
/* 
int mbox_call(unsigned char ch)
{
    mbox_send(ch);

    return mbox_read(ch);
}
*/