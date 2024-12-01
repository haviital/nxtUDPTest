#include <arch/zxn.h>
#include <stdint.h>
#include <z80.h>

#include "esp.h"
#include "uart.h"
//#include "user.h"

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

      if (c == '\n') return URR_OK;
   }
   while (--len);

   return URR_INCOMPLETE;
}

unsigned char uart_rx_readline_last(unsigned char *s, unsigned int len)
{
   if (uart_rx_readline(s, len) == URR_TIMEOUT) return URR_TIMEOUT;

   while (uart_rx_readline(s, len) != URR_TIMEOUT) ;
   
   return URR_OK;
}
