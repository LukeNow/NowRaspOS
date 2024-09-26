#include <stdint.h>
#include <stddef.h>
#include <kernel/mbox.h>
#include <kernel/gpio.h>
#include <common/common.h>

static size_t mbox_get_tag_len(mbox_prop_tag_t tag)
{
    switch (tag) {
        case MAILBOX_TAG_GET_BOARD_MODEL:
        case MAILBOX_TAG_GET_BOARD_REVISION:
        return 4;

        case MAILBOX_TAG_GET_BOARD_SERIAL:
        case MAILBOX_TAG_GET_ARM_MEMORY:
        case MAILBOX_TAG_GET_VC_MEMORY:
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