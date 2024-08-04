#ifndef __ASSERT_H
#define __ASSERT_H 

#include <common/common.h>
#include <kernel/printf.h>
#include <kernel/uart.h>

#define PRINTF_FILE_LINE printf(__FILE__); printfdigit(":", __LINE__)
#define PRINTF_DEBUG_HEADER printf(__FUNCTION__); printf(" at "); PRINTF_FILE_LINE; printf(" ")
#define PRINTF_DEBUG_MSG(MSG) PRINTF_DEBUG_HEADER; printf(MSG)
#define DEBUG(MSG) do { PRINTF_DEBUG_MSG((MSG)); printf("\n"); } while(0)
#define DEBUG_DATA(MSG, DATA) do { PRINTF_DEBUG_MSG((MSG)); printfdata("", DATA);} while (0)
#define DEBUG_DATA_BINARY(MSG, DATA) do { PRINTF_DEBUG_MSG((MSG)); printfbinary("", DATA);} while (0)
#define DEBUG_DATA_DIGIT(MSG, DATA) do { PRINTF_DEBUG_MSG((MSG)); printfdigit("", DATA);} while (0)
#define DEBUG_FUNC(MSG, DATA) do { printf(__FUNCTION__); printf(": "); printfdata(MSG, DATA);} while (0)
#define DEBUG_FUNC_BINARY(MSG, DATA) do { printf(__FUNCTION__); printf(": "); printf(MSG); printfbinary(" ", DATA);} while (0)
#define DEBUG_FUNC_DIGIT(MSG, DATA) do { printf(__FUNCTION__); printf(": "); printf(MSG); printfdigit(" ", DATA);} while (0)
#define ASSERT(X) do { if (!(X))  {printf("!!!Assertion failed at "); PRINTF_DEBUG_HEADER; printf("\n"); } }while(0)
#define ASSERT_PANIC(X, MSG) do { ASSERT((X)); if (!(X)){ DEBUG(MSG); CYCLE_INFINITE;} } while(0)
#define DEBUG_THROW(MSG) do { ASSERT(0); DEBUG(MSG); } while(0)
#define DEBUG_PANIC(MSG) ASSERT_PANIC(0, MSG)

#endif

