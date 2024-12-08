#include <arch/zxn.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <z80.h>

#include "esp.h"
#include "uart.h"
//#include "user.h"

void itoa_not_zero(uint8_t num, char* str) 
{
    int i = 0;
    if(num>9)
      i = 1;
    else if(num>99)
      i = 2;
    str[i+1] = '\0'; // Add the string terminator

    // Process individual digits
    while (num != 0) {
        int rem = num % 10;
        str[i--] = rem + '0';
        num = num / 10;
    }
   
    //return str;
}

uint8_t atoi(char* str) 
{
   uint8_t num = 0;
   uint8_t pos = strlen(str);
   if(pos==0)
      return 0;
   pos--;
   num += str[pos] - '0';
   if(pos-- > 0)
   {
      num += (str[pos] - '0') * 10;
      if(pos-- > 0)
         num += (str[pos] - '0') * 100;
   }
   
   return num;
}

void uart_send_at_cmd_and_print(unsigned char *cmd)
{
    // Send AT command to UART.
    //printf("call: %s\n", cmd);
    uart_tx(cmd);
    *buffer = 0;

    // Read response from UART. Print to to screen.
    do
    {
        //puts(buffer);  
        *buffer = 0;
        unsigned char ret = uart_rx_readline(buffer, sizeof(buffer)-1);
      //   if(ret==URR_TIMEOUT)
      //       puts("--> Error: timeout\n"); 
      //  else 
      if(ret==URR_INCOMPLETE)
            puts("--> Error: incomplete\n");         
    }
    while (*buffer);
}

void uart_send_raw_data_and_print(unsigned char *cmd, uint8_t len)
{
    // Send AT command to UART.
    //printf("call: %s\n", cmd);
    uart_raw_data_tx(cmd, len);
    *buffer = 0;

    // Read response from UART. Print to to screen.
    do
    {
        //puts(buffer);  
        *buffer = 0;
        unsigned char ret = uart_rx_readline(buffer, sizeof(buffer)-1);
      //   if(ret==URR_TIMEOUT)
      //       puts("--> Error: timeout\n"); 
      //  else 
      if(ret==URR_INCOMPLETE)
            puts("--> Error: incomplete\n");         
    }
    while (*buffer);
}

uint8_t uart_send_data_packet(unsigned char *data, uint8_t len)
{
   if( len==0 )
      return 1;

   // Send AT command to UART to start sending the UDP packet.
   //printf("call: %s\n", cmd);
   char atcmd[32];
   strcpy(atcmd, "AT+CIPSEND=");
   (void)itoa_not_zero(len, &(atcmd[11]));
   strcat(atcmd, "\r\n");
   uart_tx(atcmd);
   *buffer = 0;

   // Read response from UART. 
   // There should be: "OK\n\r>\n\r".
   uint8_t err = uart_read_response(">");
   if(err>0)
      return err;

    // Send data to UART. Check the response in the next frame.
    uart_raw_data_tx(data, len);
    return 0;
}

// Note: buffer contains the whole received line.
uint8_t uart_read_response(char* expected)
{
   // Read response from UART. 
   while(true)
   {
      //if(strncmp(buffer, "SEND OK", 7))

      // There should come: "OK\n\r>\n\r"
      if(strncmp(buffer, expected, strlen(expected))==0)
      {
         printf("Found expected:%s\n", expected);
         break;  // Ok!
      }
      *buffer = 0;
      unsigned char ret = uart_rx_readline(buffer, sizeof(buffer)-1);
      if(ret==URR_INCOMPLETE)
      {
         puts("--> Error: incomplete\n");  
         return 2;
      }       
      else if(*buffer == 0)
      {
         puts("--> Error: timeout\n");  
         return 3;
      }       
   }

   return 0;
}


uint8_t uart_get_received_data(char* data, uint8_t size)
{
   // Get the lenght of the data.
   uint8_t err = uart_read_response("+IPD,"); 
   if(err)
      return err+10;
   uint8_t i=0; 
   for(; i<10; i++)
   {
      if(buffer[i]==':')
      {
         buffer[i] = 0;
         break;
      }
   }
   if(i==10)
      return 1;
   uint8_t dataLen = atoi(&(buffer[5]));
   printf("dataLen=%u, size=%u\n", dataLen, size);
   if(dataLen!=size)
   {
      return 2;
   }

   // Copy data
   memcpy(data, &(buffer[i+1]), size);

   return 0;
}

// UART BAUD RATE

uint32_t video_timing[] = {
   CLK_28_0, CLK_28_1, CLK_28_2, CLK_28_3,
   CLK_28_4, CLK_28_5, CLK_28_6, CLK_28_7
};

uint32_t uart_compute_prescaler(uint32_t bps)
{
   return video_timing[ZXN_READ_REG(0x11) & 0x07] / bps;
}

void uart_set_prescaler(uint32_t prescaler)
{
   IO_153B = (IO_153B & 0x40) | 0x10 | (uint8_t)(prescaler >> 14);
   IO_143B = 0x80 | (uint8_t)(prescaler >> 7);
   IO_143B = (uint8_t)(prescaler) & 0x7f;
}

// UART TX

void uart_tx(unsigned char *s) __z88dk_fastcall
{
   while (*s)
   {
      while (IO_133B & 0x02)
      {
         //user_break();
      }
      
      IO_133B = *s++;
   }
}

void uart_raw_data_tx(unsigned char *s, uint8_t len)
{
   for( uint8_t i=len; i!=0; i--)
   {
      while (IO_133B & 0x02)
      {
         //user_break();
      }
      
      IO_133B = *s++;
   }
}


// UART RX

unsigned char uart_rx_readline(unsigned char *s, unsigned int len)
{
   static uint16_t timeout_ms;
   
   unsigned char c;

   // s is not modified and not zero terminated if no chars are read at all
   // buffer size should be len+1 bytes and len != 0

   do
   {
      timeout_ms = esp_response_time_ms;
      
      while (!(IO_133B & 0x01))
      {
         if (!(timeout_ms--)) return URR_TIMEOUT;
         
         z80_delay_ms(1*8);   // 8x for 28MHz
         //user_break();
      }
      
      c = IO_143B;
      
      *s++ = c;
      *s = 0;

      if (c == '\n') 
      {
         //puts("--> return found!\n"); 
         return URR_OK;
      }
   }
   while (--len);

   return URR_INCOMPLETE;
}

bool uart_available_rx(void)
{
   return (IO_133B & 0x01);
}

unsigned char uart_rx_readline_last(unsigned char *s, unsigned int len)
{
   if (uart_rx_readline(s, len) == URR_TIMEOUT) return URR_TIMEOUT;

   while (uart_rx_readline(s, len) != URR_TIMEOUT) ;
   
   return URR_OK;
}
