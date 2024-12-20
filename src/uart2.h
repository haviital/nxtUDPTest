
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defines.h"

#ifdef PRINT_TO_BUFFER2
extern char testBuffer[2048];
#endif

void myitoa(uint8_t num, char* str); 
uint8_t myatoi(char* str); 
void uart_tx2(unsigned char *s);
unsigned char uart_rx_char2(void);
void uart_flush_rx2(void);
uint8_t uart_read_expected2(char* expected);
uint8_t uart_read_expected_many2(char* expected1, char* expected2);
uint8_t uart_send_data_packet2(unsigned char *data, uint8_t len);
void prog_failed(char* sourceFile, int32_t lineNum);


