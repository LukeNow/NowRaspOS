#ifndef __ASSERT_H
#define __ASSERT_H 

#include <kernel/printf.h>
#include <kernel/uart.h>

#define PRINTF_FILE_LINE printf(__FILE__); printfdigit(":", __LINE__)
#define PRINTF_DEBUG_HEADER printf(__FUNCTION__); printf(" at "); PRINTF_FILE_LINE; printf(" ")
#define PRINTF_DEBUG_MSG(MSG) PRINTF_DEBUG_HEADER; printf(MSG)
#define ASSERT(X) do { if (!(X))  {printf("!!!Assertion failed at "); PRINTF_DEBUG_HEADER; printf("\n"); } }while(0);
#define DEBUG(MSG) do { PRINTF_DEBUG_MSG((MSG)); printf("\n"); } while(0)
#define DEBUG_DATA(MSG, DATA) do { PRINTF_DEBUG_MSG((MSG)); printfdata("", DATA);} while (0)
#define DEBUG_DATA_DIGIT(MSG, DATA) do { PRINTF_DEBUG_MSG((MSG)); printfdigit("", DATA);} while (0)
#define DEBUG_FUNC(MSG, DATA) do { printf(__FUNCTION__); printf(": "); printfdata(MSG, DATA);} while (0)

#endif

