
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
unsigned char uart_rx_char2(void);
bool uart_available_rx2(void);
bool uart_available_rx_wait_once(uint16_t waitTimeMs);
void uart_flush_rx2(void);
uint8_t uart_read_expected2(char* expected);
uint8_t uart_read_expected_many2(char* expected1, char* expected2);

uint8_t uart_send_data_packet2(unsigned char *data, uint8_t len);
uint8_t uart_receive_data_packet2(char* receivedData, uint8_t size);
uint8_t uart_receive_data_packet_if_any(char* receivedData, uint8_t size);

void prog_failed(char* sourceFile, int32_t lineNum, uint8_t err);


