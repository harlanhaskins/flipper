#ifndef __uart0_h__
#define __uart0_h__

/* Include all types and macros exposed by the Flipper Toolbox. */
#include <flipper.h>

/* Declare the prototypes for all of the functions within this module. */
int uart0_configure(void);
int uart0_setbaud(uint32_t baud);
int uart0_reset(void);
int uart0_ready(void);
void uart0_put(uint8_t byte);
uint8_t uart0_get(void);
void uart0_enable(void);
void uart0_disable(void);
int uart0_write(void *src, size_t length);
int uart0_read(void *dst, size_t length);

#endif
