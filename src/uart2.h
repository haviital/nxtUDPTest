
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defines.h"

#ifdef PRINT_TO_BUFFER2
extern char testBuffer[2048];
#endif

void uart_tx2(unsigned char *s);
unsigned char uart_rx_char2(void);
void uart_flush_rx2(void);
uint8_t uart_read_expected2(char* expected);
uint8_t uart_read_expected_many2(char* expected1, char* expected2);
void prog_failed(char* sourceFile, int32_t lineNum);


