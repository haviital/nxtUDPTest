// tcpping
// Johan Engdahl

// zcc +zxn -v -startup=30 -clib=sdcc_iy -SO3 --max-allocs-per-node200000 tcpping.c -o tcpping -subtype=dot -Cz"--clean" -create-app

#include <arch/zxn.h>
#include <arch/zxn/sysvar.h>
#include <input.h>
#include <intrinsic.h>
#include <z80.h>
#include "uart2.h"

#define SERVER_IP_ADDRESS_2 "192.168.100.134"
#define SERVER_PORT_2 "8000"


#pragma printf = "%lu %s %u %c"
#pragma output CLIB_EXIT_STACK_SIZE = 1



// UART

__sfr __banked __at 0x153b IO_153B;   // until it is added to headers

/*
uint32_t video_timing[] = {
   CLK_28_0, CLK_28_1, CLK_28_2, CLK_28_3,
   CLK_28_4, CLK_28_5, CLK_28_6, CLK_28_7
};

void uart_set_bps(uint32_t bps)
{
   static uint32_t prescalar;
   
   prescalar = video_timing[(ZXN_READ_REG(0x11) & 0x07)] / bps;
   
   IO_153B = (IO_153B & 0x40) | 0x10 | (uint8_t)(prescalar >> 14);
   IO_143B = 0x80 | (uint8_t)(prescalar >> 7);
   IO_143B = (uint8_t)(prescalar) & 0x7f;
}
*/

// FRAMES

uint32_t before, after, beforeTest, afterTest;

// MAIN

unsigned char old_uart;
unsigned char old_cpu_speed;

void cleanup(void)
{
   IO_153B = old_uart;
   ZXN_NEXTREGA(0x07, old_cpu_speed);
}

unsigned char rst[20] = "AT+RST";
unsigned char close[20] = "AT+CIPCLOSE";
unsigned char ipstart_cmd[60] = "AT+CIPSTART=\"TCP\",\"";

int TEST_main(void)
{
   static unsigned char byte;
   static unsigned char lastchar;
   
   static uint32_t temp;
     
   strcat(ipstart_cmd, SERVER_IP_ADDRESS_2);
   strcat(ipstart_cmd, "\",");
   strcat(ipstart_cmd, SERVER_PORT_2);

   lastchar = 0;
   
   uint8_t counter = 10;

   #ifdef PRINT_TO_BUFFER2
   testBuffer2[0] = 0;
   #endif

   uart_flush_rx2();

   // Do a connection open and close 10 times
   printf("Start testing! round: ");
   memcpy(&beforeTest, SYSVAR_FRAMES, 3);  // inlines as ldir
   while (1)
   {
      // Open connection
      uart_tx2(ipstart_cmd);
      uart_tx2("\r\n");
      
      if(uart_read_expected2("CONNECT") == 0)
      {
         // *** Connected ok
         printf("%u ", counter);

         // Close connetion
         uart_tx2(close);
         uart_tx2("\r\n");

         // Note: you MUST read this before calling open again. Otherwise the connection 
         //       might still be not yet closed and the open fails.
         if(uart_read_expected2("CLOSED") == 0)
         {
            // *** Connection closed.
         }
         else
         {
            printf("Did not found 'CLOSED'\n");
            for(;;);  // Loop forever
         }
      }
      else
      {
         printf("Did not found 'CONNECTED'.\n");
         for(;;);  // Loop forever
      }

      // Connected ok. Do it again.
      if(--counter==0)
         break;

   }  // repeated connect loop

   memcpy(&afterTest, SYSVAR_FRAMES, 3);  // inlines as ldir
   printf("\nTEST done! Testing 10 times took %lu frames.",afterTest - beforeTest);
   
   return 0;
}
