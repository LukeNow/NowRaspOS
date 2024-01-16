#ifndef __ASSERT_H
#define __ASSERT_H 

#include  <kernel/printf.h>

#define ASSERT(X) do {if (!(X))  {printf("ASSERTION FAILED"); printf("\n");} }while(0);

#endif

