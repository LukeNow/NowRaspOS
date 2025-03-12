#ifndef __MBOX_H
#define __MBOX_H

#include <kernel/gpio.h>

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

/* Definitions taken from https://github.com/LdB-ECM/Raspberry-Pi/blob/master/10_virtualmemory/rpi-SmartStart.h */
/*--------------------------------------------------------------------------}
{	            ENUMERATED MAILBOX TAG CHANNEL COMMANDS						}
{  https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface  }
{--------------------------------------------------------------------------*/
typedef enum {
    /* Videocore info commands */
	MAILBOX_TAG_GET_VERSION					= 0x00000001,			// Get firmware revision

	/* Hardware info commands */
	MAILBOX_TAG_GET_BOARD_MODEL				= 0x00010001,			// Get board model
	MAILBOX_TAG_GET_BOARD_REVISION			= 0x00010002,			// Get board revision
	MAILBOX_TAG_GET_BOARD_MAC_ADDRESS		= 0x00010003,			// Get board MAC address
	MAILBOX_TAG_GET_BOARD_SERIAL			= 0x00010004,			// Get board serial
	MAILBOX_TAG_GET_ARM_MEMORY				= 0x00010005,			// Get ARM memory
	MAILBOX_TAG_GET_VC_MEMORY				= 0x00010006,			// Get VC memory
	MAILBOX_TAG_GET_CLOCKS					= 0x00010007,			// Get clocks

	/* Power commands */
	MAILBOX_TAG_GET_POWER_STATE				= 0x00020001,			// Get power state
	MAILBOX_TAG_GET_TIMING					= 0x00020002,			// Get timing
	MAILBOX_TAG_SET_POWER_STATE				= 0x00028001,			// Set power state

	/* GPIO commands */
	MAILBOX_TAG_GET_GET_GPIO_STATE			= 0x00030041,			// Get GPIO state
	MAILBOX_TAG_SET_GPIO_STATE				= 0x00038041,			// Set GPIO state

	/* Clock commands */
	MAILBOX_TAG_GET_CLOCK_STATE				= 0x00030001,			// Get clock state
	MAILBOX_TAG_GET_CLOCK_RATE				= 0x00030002,			// Get clock rate
	MAILBOX_TAG_GET_MAX_CLOCK_RATE			= 0x00030004,			// Get max clock rate
	MAILBOX_TAG_GET_MIN_CLOCK_RATE			= 0x00030007,			// Get min clock rate
	MAILBOX_TAG_GET_TURBO					= 0x00030009,			// Get turbo

	MAILBOX_TAG_SET_CLOCK_STATE				= 0x00038001,			// Set clock state
	MAILBOX_TAG_SET_CLOCK_RATE				= 0x00038002,			// Set clock rate
	MAILBOX_TAG_SET_TURBO					= 0x00038009,			// Set turbo

	/* Voltage commands */
	MAILBOX_TAG_GET_VOLTAGE					= 0x00030003,			// Get voltage
	MAILBOX_TAG_GET_MAX_VOLTAGE				= 0x00030005,			// Get max voltage
	MAILBOX_TAG_GET_MIN_VOLTAGE				= 0x00030008,			// Get min voltage

	MAILBOX_TAG_SET_VOLTAGE					= 0x00038003,			// Set voltage

    /* Temperature commands */
	MAILBOX_TAG_GET_TEMPERATURE				= 0x00030006,			// Get temperature
	MAILBOX_TAG_GET_MAX_TEMPERATURE			= 0x0003000A,			// Get max temperature

	/* Memory commands */
	MAILBOX_TAG_ALLOCATE_MEMORY				= 0x0003000C,			// Allocate Memory
	MAILBOX_TAG_LOCK_MEMORY					= 0x0003000D,			// Lock memory
	MAILBOX_TAG_UNLOCK_MEMORY				= 0x0003000E,			// Unlock memory
	MAILBOX_TAG_RELEASE_MEMORY				= 0x0003000F,			// Release Memory
																	
	/* Execute code commands */
	MAILBOX_TAG_EXECUTE_CODE				= 0x00030010,			// Execute code

	/* QPU control commands */
	MAILBOX_TAG_EXECUTE_QPU					= 0x00030011,			// Execute code on QPU
	MAILBOX_TAG_ENABLE_QPU					= 0x00030012,			// QPU enable

	/* Displaymax commands */
	MAILBOX_TAG_GET_DISPMANX_HANDLE			= 0x00030014,			// Get displaymax handle
	MAILBOX_TAG_GET_EDID_BLOCK				= 0x00030020,			// Get HDMI EDID block

	/* SD Card commands */
	MAILBOX_GET_SDHOST_CLOCK				= 0x00030042,			// Get SD Card EMCC clock
	MAILBOX_SET_SDHOST_CLOCK				= 0x00038042,			// Set SD Card EMCC clock

	/* Framebuffer commands */
	MAILBOX_TAG_ALLOCATE_FRAMEBUFFER		= 0x00040001,			// Allocate Framebuffer address
	MAILBOX_TAG_BLANK_SCREEN				= 0x00040002,			// Blank screen
	MAILBOX_TAG_GET_PHYSICAL_WIDTH_HEIGHT	= 0x00040003,			// Get physical screen width/height
	MAILBOX_TAG_GET_VIRTUAL_WIDTH_HEIGHT	= 0x00040004,			// Get virtual screen width/height
	MAILBOX_TAG_GET_COLOUR_DEPTH			= 0x00040005,			// Get screen colour depth
	MAILBOX_TAG_GET_PIXEL_ORDER				= 0x00040006,			// Get screen pixel order
	MAILBOX_TAG_GET_ALPHA_MODE				= 0x00040007,			// Get screen alpha mode
	MAILBOX_TAG_GET_PITCH					= 0x00040008,			// Get screen line to line pitch
	MAILBOX_TAG_GET_VIRTUAL_OFFSET			= 0x00040009,			// Get screen virtual offset
	MAILBOX_TAG_GET_OVERSCAN				= 0x0004000A,			// Get screen overscan value
	MAILBOX_TAG_GET_PALETTE					= 0x0004000B,			// Get screen palette

    MAILBOX_TAG_RELEASE_FRAMEBUFFER			= 0x00048001,			// Release Framebuffer address
	MAILBOX_TAG_SET_PHYSICAL_WIDTH_HEIGHT	= 0x00048003,			// Set physical screen width/heigh
	MAILBOX_TAG_SET_VIRTUAL_WIDTH_HEIGHT	= 0x00048004,			// Set virtual screen width/height
	MAILBOX_TAG_SET_COLOUR_DEPTH			= 0x00048005,			// Set screen colour depth
	MAILBOX_TAG_SET_PIXEL_ORDER				= 0x00048006,			// Set screen pixel order
	MAILBOX_TAG_SET_ALPHA_MODE				= 0x00048007,			// Set screen alpha mode
	MAILBOX_TAG_SET_VIRTUAL_OFFSET			= 0x00048009,			// Set screen virtual offset
	MAILBOX_TAG_SET_OVERSCAN				= 0x0004800A,			// Set screen overscan value
	MAILBOX_TAG_SET_PALETTE					= 0x0004800B,			// Set screen palette
	MAILBOX_TAG_SET_VSYNC					= 0x0004800E,			// Set screen VSync
	MAILBOX_TAG_SET_BACKLIGHT				= 0x0004800F,			// Set screen backlight

	/* VCHIQ commands */
	MAILBOX_TAG_VCHIQ_INIT					= 0x00048010,			// Enable VCHIQ

	/* Config commands */
	MAILBOX_TAG_GET_COMMAND_LINE			= 0x00050001,			// Get command line 

	/* Shared resource management commands */
	MAILBOX_TAG_GET_DMA_CHANNELS			= 0x00060001,			// Get DMA channels

	/* Cursor commands */
	MAILBOX_TAG_SET_CURSOR_INFO				= 0x00008010,			// Set cursor info
	MAILBOX_TAG_SET_CURSOR_STATE			= 0x00008011,			// Set cursor state
} mbox_prop_tag_t;

/*--------------------------------------------------------------------------}
{	                  ENUMERATED MAILBOX CHANNELS							}
{		  https://github.com/raspberrypi/firmware/wiki/Mailboxes			}
{--------------------------------------------------------------------------*/
typedef enum {
	MB_CHANNEL_POWER = 0x0,								// Mailbox Channel 0: Power Management Interface 
	MB_CHANNEL_FB = 0x1,								// Mailbox Channel 1: Frame Buffer
	MB_CHANNEL_VUART = 0x2,								// Mailbox Channel 2: Virtual UART
	MB_CHANNEL_VCHIQ = 0x3,								// Mailbox Channel 3: VCHIQ Interface
	MB_CHANNEL_LEDS = 0x4,								// Mailbox Channel 4: LEDs Interface
	MB_CHANNEL_BUTTONS = 0x5,							// Mailbox Channel 5: Buttons Interface
	MB_CHANNEL_TOUCH = 0x6,								// Mailbox Channel 6: Touchscreen Interface
	MB_CHANNEL_COUNT = 0x7,								// Mailbox Channel 7: Counter
	MB_CHANNEL_TAGS = 0x8,								// Mailbox Channel 8: Tags (ARM to VC)
	MB_CHANNEL_GPU = 0x9,								// Mailbox Channel 9: GPU (VC to ARM)
} mbox_channel_t;

/*--------------------------------------------------------------------------}
{					    ENUMERATED MAILBOX CLOCK ID							}
{		  https://github.com/raspberrypi/firmware/wiki/Mailboxes			}
{--------------------------------------------------------------------------*/
typedef enum {
	CLK_EMMC_ID		= 0x1,								// Mailbox Tag Channel EMMC clock ID 
	CLK_UART_ID		= 0x2,								// Mailbox Tag Channel uart clock ID
	CLK_ARM_ID		= 0x3,								// Mailbox Tag Channel ARM clock ID
	CLK_CORE_ID		= 0x4,								// Mailbox Tag Channel SOC core clock ID
	CLK_V3D_ID		= 0x5,								// Mailbox Tag Channel V3D clock ID
	CLK_H264_ID		= 0x6,								// Mailbox Tag Channel H264 clock ID
	CLK_ISP_ID		= 0x7,								// Mailbox Tag Channel ISP clock ID
	CLK_SDRAM_ID	= 0x8,								// Mailbox Tag Channel SDRAM clock ID
	CLK_PIXEL_ID	= 0x9,								// Mailbox Tag Channel PIXEL clock ID
	CLK_PWM_ID		= 0xA,								// Mailbox Tag Channel PWM clock ID
} mbox_clock_id_t;

/*--------------------------------------------------------------------------}
{			      ENUMERATED MAILBOX POWER BLOCK ID							}
{		  https://github.com/raspberrypi/firmware/wiki/Mailboxes			}
{--------------------------------------------------------------------------*/
typedef enum {
	PB_SDCARD		= 0x0,								// Mailbox Tag Channel SD Card power block 
	PB_UART0		= 0x1,								// Mailbox Tag Channel UART0 power block 
	PB_UART1		= 0x2,								// Mailbox Tag Channel UART1 power block 
	PB_USBHCD		= 0x3,								// Mailbox Tag Channel USB_HCD power block 
	PB_I2C0			= 0x4,								// Mailbox Tag Channel I2C0 power block 
	PB_I2C1			= 0x5,								// Mailbox Tag Channel I2C1 power block 
	PB_I2C2			= 0x6,								// Mailbox Tag Channel I2C2 power block 
	PB_SPI			= 0x7,								// Mailbox Tag Channel SPI power block 
	PB_CCP2TX		= 0x8,								// Mailbox Tag Channel CCP2TX power block 
} mbox_power_id_t;
typedef enum {
    REQUEST = 0x00000000,
    RESPONSE_SUCCESS = 0x80000000,
    RESPONSE_ERROR = 0x80000001
} mbox_res_code_t;

typedef enum {
	CORE_EXEC = 1,
	CORE_INVALIDATE = 2
} mbox_core_cmd_t;

#define MBOX_CORE_CMD_BYTE 0x0000000F
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


void mbox_send(mbox_message_t msg, mbox_channel_t ch);
uint32_t mbox_read(mbox_channel_t ch);
int mbox_request(uint32_t * response_buf, uint8_t data_count, ...);
mbox_message_t mbox_make_msg(uint32_t *mbox, uint32_t ch);

void mbox_enable_irq(uint8_t corenum);
void mbox_clear_core_msg(uint8_t corenum, uint8_t fromcore);
void mbox_send_core_msg(uint8_t tocore, uint8_t fromcore, uint32_t msg);
uint32_t mbox_get_core_msg(uint8_t corenum, uint8_t fromcore);
uint32_t mbox_get_clear_core_msg(uint8_t corenum, uint8_t fromcore);
void mbox_core_cmd_int(uint8_t tocore, uint8_t fromcore, mbox_core_cmd_t cmd, uint32_t data);
void mbox_handle_core_int(uint8_t corenum, uint8_t fromcore);

#endif