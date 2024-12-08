#ifndef UART_H
#define UART_H

#include <stdint.h>

__sfr __banked __at 0x153b IO_153B;   // until it is added to headers

extern void uart_send_at_cmd_and_print(unsigned char *cmd);

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
extern void uart_raw_data_tx(unsigned char *s, uint8_t len);
extern void uart_send_raw_data_and_print(unsigned char *cmd, uint8_t len);

#endif
