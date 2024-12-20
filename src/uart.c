#include <arch/zxn.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <z80.h>

#include "esp.h"
#include "uart.h"
//#include "user.h"

#if 0

void uart_send_at_cmd(unsigned char *cmd)
{
   uart_send_at_cmd_custom_response( cmd, "OK\r\n", NULL);
}

void uart_send_at_cmd_custom_response(unsigned char *cmd, char* expectedResponse, char* orExpectedResponse_CanBeNull)
{
   // Send AT command to UART.
   if(UART_DEBUG_PRINT_ENABLED) printf("call: %s\n", cmd);
   uart_tx(cmd);
   *buffer = 0;

   // Read response from UART. Print to to screen.
   uart_read_response(expectedResponse, orExpectedResponse_CanBeNull);
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
        if(UART_DEBUG_PRINT_ENABLED) puts(buffer);  
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
   char atcmd[32];
   strcpy(atcmd, "AT+CIPSEND=");
   (void)myitoa_not_zero(len, &(atcmd[11]));
   strcat(atcmd, "\r\n");
   if(UART_DEBUG_PRINT_ENABLED) printf("call: %s\n", atcmd);
   uart_tx(atcmd);
   *buffer = 0;

   // Read response from UART. 
   // There should be: "OK\n\r>\n\r".
   uint8_t err = uart_read_response(">", NULL);
   if(err>0)
      return err;

    // Send data to UART. Check the response in the next frame.
    uart_raw_data_tx(data, len);
    return 0;
}

char* replaceCrAndLn(char* str, char* newStr)
{
   uint8_t len = strlen(str);
   uint8_t i=0;
   for(; i<len; i++)
   {
      if(str[i]=='\r') 
         newStr[i]='r';
      else if(str[i]=='\n') 
         newStr[i]='n';
      else 
         newStr[i] = str[i];
   }
   newStr[i]=0; // ending 0
   return newStr;
}

int mystrncmp(char* str1, char* str2, int len, bool debug)
{
   for(uint8_t i=0; i<len; i++ )
   {
      if(debug)
         printf(" %c(%u)--%c(%u) ", str1[i], str1[i], str2[i], str2[i]);
      if(str1[i] != str2[i])
         return (str1[i] - str2[i]);
   }
   
   return 0;
}

// Note: buffer contains the whole received line.
uint8_t uart_read_response(char* expected, char* orExpected_CanBeNull)
{
   // Read response from UART. 
   char outTxt[128];
   if(UART_DEBUG_PRINT_ENABLED) printf("Expected to read: %s\n", replaceCrAndLn(expected, outTxt));
   if(UART_DEBUG_PRINT_ENABLED && orExpected_CanBeNull !=NULL) printf("or: %s\n", replaceCrAndLn(orExpected_CanBeNull, outTxt));
    while(true)
   {
      //if(strncmp(buffer, "SEND OK", 7))

      // There should come: "OK\n\r>\n\r"
      *buffer = 0;
      unsigned char ret = uart_rx_readline(buffer, sizeof(buffer)-1);
      //if(mystrncmp(buffer, expected, strlen(expected), false)==0)
      if(strncmp(buffer, expected, strlen(expected))==0)
      {
         if(UART_DEBUG_PRINT_ENABLED) printf("*** Found expected:%s\n", replaceCrAndLn(expected, outTxt));
         break;  // Ok!
      }
      else if( orExpected_CanBeNull!=NULL && 
               strncmp(buffer, orExpected_CanBeNull, strlen(orExpected_CanBeNull))==0)
      {
         if(UART_DEBUG_PRINT_ENABLED) printf("*** Found expected:%s\n", replaceCrAndLn(orExpected_CanBeNull, outTxt));
         break;  // Ok!
      }
      else if(ret==URR_INCOMPLETE)
      {
         puts("--> Error: incomplete\n");  
         return 2;
      }       
      else if(ret==URR_TIMEOUT)
      {
         puts("--> Error: timeout in reading line\n");  
         return 2;
      }       
      else if(*buffer == 0)
      {
         puts("--> Error: timeout in reading line (empty)\n");  
         return 3;
      }       
      else 
      {
         if(UART_DEBUG_PRINT_ENABLED) printf("Incorrect resp found ('%s'). Read again.\n", buffer);
         //int ret = mystrncmp(buffer, expected, strlen(expected), true);
         //printf("Incorrect resp found(%d): B=%s(%u) <> E=%s(%u). Read again.\n", 
         //   ret, replaceCrAndLn(buffer), strlen(buffer), replaceCrAndLn(expected), strlen(expected));  
      }       
   }

   return 0;
}


uint8_t uart_get_received_data(char* data, uint8_t size)
{
   // Get the lenght of the data.
   uint8_t err = uart_read_response("+IPD,", NULL); 
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
   uint8_t dataLen = myatoi(&(buffer[5]));
   //printf("dataLen=%u, size=%u\n", dataLen, size);
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
#endif