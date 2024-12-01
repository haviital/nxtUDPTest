#ifndef UART_H
#define UART_H

#include <stdint.h>

__sfr __banked __at 0x153b IO_153B;   // until it is added to headers

// UART BAUD RATE

extern uint32_t uart_compute_prescaler(uint32_t bps);
extern void uart_set_prescaler(uint32_t prescaler);

// UART TX

extern void uart_tx(unsigned char *s) __z88dk_fastcall;

// UART RX

#define URR_OK 0x00
#define URR_INCOMPLETE 0x40
#define URR_TIMEOUT 0x80

extern unsigned char uart_rx_readline(unsigned char *s, unsigned int len);
extern unsigned char uart_rx_readline_last(unsigned char *s, unsigned int len);

#endif
