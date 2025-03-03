#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <kernel/mbox.h>
#include <kernel/gpio.h>
#include <common/common.h>
#include <common/aarch64_common.h>
#include <common/assert.h>
#include <kernel/mmu.h>

#define MBOX_HEADER_SIZE 3

static size_t mbox_get_tag_len(mbox_prop_tag_t tag)
{
    switch (tag) {
        case MAILBOX_TAG_GET_BOARD_MODEL:
        case MAILBOX_TAG_GET_BOARD_REVISION:
        case MAILBOX_TAG_GET_CLOCK_RATE:
        return 4;

        case MAILBOX_TAG_GET_BOARD_SERIAL:
        case MAILBOX_TAG_GET_ARM_MEMORY:
        case MAILBOX_TAG_GET_VC_MEMORY:
        return 8;
    }
}

void mbox_send(mbox_message_t msg, mbox_channel_t ch)
{
    if (ch < 0 || ch > MB_CHANNEL_GPU) {
        DEBUG_PANIC("Channel is not in the right range");
    }

    msg &= ~0xF;
    msg |= ch;

    while ((MAILBOX->Status1 & MBOX_FULL) != 0) {}

    MAILBOX->Write1 = (uint32_t)msg;
}

uint32_t mbox_read(mbox_channel_t ch)
{
    uint32_t val;

    if (ch < 0 || ch > MB_CHANNEL_GPU) {
        DEBUG_PANIC("Channel is not in the right range");
    }

    do {
        while ((MAILBOX->Status0 & MBOX_EMPTY) != 0) {};
        val = MAILBOX->Read0;

    } while ((val & 0xF) != ch);

    val &= ~(0xF);

    return val;
}

int mbox_request(uint32_t * response_buf, uint8_t data_count, ...)
{   
    uint32_t __attribute__((aligned(16))) msg[data_count + MBOX_HEADER_SIZE];
    uint32_t msg_addr = (uint32_t) mmu_get_phys_addr(&msg[0]);

    DEBUG_DATA("Msg addr=", msg_addr);
    va_list list;
    va_start(list, data_count);

    msg[0] = (data_count + MBOX_HEADER_SIZE) * sizeof(uint32_t);
    msg[1] = MBOX_REQUEST;
    // The Null tag to signal end of message
    msg[data_count + 2] = 0;
    for (int i = 0; i < data_count; i++) {
        msg[i + 2] = va_arg(list, uint32_t);
    }
    va_end(list);

    aarch64_cache_flush_invalidate(msg_addr);

    mbox_send(msg_addr, MBOX_CH_PROP);

    mbox_read(MBOX_CH_PROP);

    aarch64_cache_flush_invalidate(msg_addr);

    if (response_buf) {
        for (int i = 0; i < data_count; i++) {
            response_buf[i] = msg[i + 2];
        }
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
