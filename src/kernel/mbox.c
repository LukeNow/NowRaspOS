#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <kernel/mbox.h>
#include <kernel/gpio.h>
#include <common/common.h>
#include <common/aarch64_common.h>
#include <common/assert.h>
#include <kernel/mmu.h>
#include <kernel/mm.h>
#include <kernel/cpu.h>

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

bool mbox_request(uint32_t * response_buf, uint8_t data_count, ...)
{   
    uint32_t __attribute__((aligned(16))) msg[data_count + MBOX_HEADER_SIZE];
    uint32_t msg_addr = (uint32_t) mmu_get_phys_addr((uint64_t)&msg[0]);

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

    if (msg[1] == MBOX_RESPONSE_SUCCESS) {
        if (response_buf) {
            for (int i = 0; i < data_count; i++) {
                response_buf[i] = msg[i + 2];
            }
        }

        return true;
    }
    
    return false;
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

void mbox_clear_core_msg(uint8_t corenum, uint8_t fromcore)
{
    *(volatile uint32_t *)(((uint32_t *)&QA7->CoreMboxRead[corenum]) + fromcore) = 0xFFFFFFFF;
}

/* Local core mailbox "doorbell" register functions */
void mbox_enable_irq(uint8_t corenum)
{
    QA7->CoreMailboxIntControl[corenum].Mailbox0_FIQ = 0;
    QA7->CoreMailboxIntControl[corenum].Mailbox0_IRQ = 1;
    mbox_clear_core_msg(corenum, 0);
    QA7->CoreMailboxIntControl[corenum].Mailbox1_FIQ = 0;
    QA7->CoreMailboxIntControl[corenum].Mailbox1_IRQ = 1;
    mbox_clear_core_msg(corenum, 1);
    QA7->CoreMailboxIntControl[corenum].Mailbox2_FIQ = 0;
    QA7->CoreMailboxIntControl[corenum].Mailbox2_IRQ = 1;
    mbox_clear_core_msg(corenum, 2);
    QA7->CoreMailboxIntControl[corenum].Mailbox3_FIQ = 0;
    QA7->CoreMailboxIntControl[corenum].Mailbox3_IRQ = 1;
    mbox_clear_core_msg(corenum, 3);
}


uint32_t mbox_get_core_msg(uint8_t corenum, uint8_t fromcore)
{
    return *(volatile uint32_t *)(((uint32_t *)&QA7->CoreMboxRead[corenum]) + fromcore);
}

uint32_t mbox_get_clear_core_msg(uint8_t corenum, uint8_t fromcore)
{
    uint32_t msg = mbox_get_core_msg(corenum, fromcore);
    mbox_clear_core_msg(corenum, fromcore);
    return msg;
}

void mbox_send_core_msg(uint8_t tocore, uint8_t fromcore, uint32_t msg)
{   
    if (mbox_get_core_msg(tocore, fromcore) != 0) {
        DEBUG_PANIC("MBOX STILL HAS MESSAGE");
    }

    *(volatile uint32_t *)(((uint32_t *)&QA7->CoreMboxWrite[tocore]) + fromcore) = msg;
}

void mbox_core_cmd_int(uint8_t tocore, uint8_t fromcore, mbox_core_cmd_t cmd, uint32_t data)
{
    if (data & MBOX_CORE_CMD_BYTE) {
        DEBUG_PANIC("There is data in the core command byte section");
    }

    data = data | (cmd & MBOX_CORE_CMD_BYTE);

    mbox_send_core_msg(tocore, fromcore, data);
}

/* ISR CONTEXT */
void mbox_handle_core_int(uint8_t corenum, uint8_t fromcore)
{
    uint32_t msg = mbox_get_core_msg(corenum, fromcore);
    uint32_t cmd = msg & MBOX_CORE_CMD_BYTE;
    msg = msg &~ MBOX_CORE_CMD_BYTE;


    mbox_clear_core_msg(corenum, fromcore);

    switch (cmd) {
        case CORE_EXEC:
        //DEBUG("CORE EXEC MESSAGE");
            if (msg == 0) {
                DEBUG_PANIC("CORE EXEC MSG IS 0");
            }
            uint64_t func_addr = mmu_get_kern_addr((uint64_t)msg);
            
            ((void (*)())func_addr)();
            break;
        case CORE_DUMP:
            cpu_core_dump();
            cpu_core_stop();
            break;
        case CORE_STOP:
            cpu_core_stop();
            break;
        case CORE_INVALIDATE:
            break;
        default:
            DEBUG_PANIC("INVALID CMD CODE");
    }
}
