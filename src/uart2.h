
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "defines.h"

#ifdef PRINT_TO_BUFFER2
extern char testBuffer[2048];
#endif

void uart_tx2(unsigned char *s);
uint8_t uart_raw_tx2(unsigned char *s, uint16_t size);
unsigned char uart_rx_char2(void);
bool uart_available_rx2(void);
bool uart_available_rx_wait_once(uint16_t waitTimeMs);
void uart_flush_rx2(void);
uint8_t uart_read_expected2(char* expected);
uint8_t uart_read_expected_many2(char* expected1, char* expected2, /*OUT*/uint8_t* foundStringOrdinal);
uint8_t uart_read_and_get_until_char(char untilChar, char* receivedData, uint8_t maxLen);


// Used to enable debug prints.
extern uint8_t uart_debug_print;

// uart log buffer
#define UART_LOG_RING_BUFFER_SIZE 512
extern uint16_t uart_log_ring_buffer_pos;
extern char uart_log_ring_buffer[UART_LOG_RING_BUFFER_SIZE];
void add_to_log_ring_buffer(char c);
void add_str_to_log_ring_buffer(char* s);
void add_to_log_ring_buffer_with_color(char c, uint8_t color);
void add_str_to_log_ring_buffer_with_color(char* s, uint8_t color);
void log_buffer_set_ink_color(uint8_t c);
void print_log_buffer(void);




