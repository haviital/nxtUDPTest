// tcpping
// Johan Engdahl

// zcc +zxn -v -startup=30 -clib=sdcc_iy -SO3 --max-allocs-per-node200000 tcpping.c -o tcpping -subtype=dot -Cz"--clean" -create-app

#include <arch/zxn.h>
#include <arch/zxn/sysvar.h>
#include <input.h>
#include <intrinsic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma printf = "%lu %s"
#pragma output CLIB_EXIT_STACK_SIZE = 1

// USER BREAK

unsigned char err_break[] = "D BREAK - no repea" "\xf4";

void user_break(void)
{
   if (in_key_pressed(IN_KEY_SCANCODE_SPACE | 0x8000))  // CAPS+SPACE
      exit((int)err_break);
}

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
void uart_tx2(unsigned char *s)
{
   while (*s)
   {
      while (IO_133B & 0x02)
         user_break();
      
      IO_133B = *s++;
   }
}

// FRAMES

uint32_t before, after;

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
unsigned char array[60] = "AT+CIPSTART=\"TCP\",\"";

int TEST_main(void)
{
   static unsigned char byte;
   static unsigned char lastchar;
   
   static uint32_t temp;
   
   unsigned char c;
   
   // restore on exit
   
   old_cpu_speed = ZXN_READ_REG(0x07) & 0x03;
   old_uart = IO_153B & 0x40;

   atexit(cleanup);
   
   ZXN_NEXTREG(0x07, 0x03);   // 28MHz
   IO_153B = 0x00;   // select esp uart
   
   // command line
/*   
   if (argc == 2)
   {
      temp = atol(argv[1]);
      
      printf("\nSetting uart to %lu bps\n\n", temp);
      //uart_set_bps(temp);
      
      exit(0);
   }
   
   if (argc != 3)
   {
      printf("\nUsage:\n"
             "  .TCPPING [IP|FQDN] [Port]\n"
             "  .TCPPING bps\n\n");
      exit(0);
   }
*/   
   // flush read buffer
   
   while (IO_133B & 0x01)
   {
      c = IO_143B;
      user_break();
   }
   
   // ping
   const char* par1 = "192.168.100.133";
   const char* par2 = "8000";
   printf("\nPinging %s at port %s\n", par1, par2);
   
   strcat(array, par1);
   strcat(array, "\",");
   strcat(array, par2);
   
   uart_tx2(array);
   uart_tx2("\r\n");
   
   intrinsic_di();
   memcpy(&before, SYSVAR_FRAMES, 3);  // inlines as ldir
   intrinsic_ei();
   
   lastchar = 0;
   
   while (1)
   {
      // read byte from uart
      
      while (!(IO_133B & 0x01))
         user_break();
      
      byte = IO_143B;
      
      //
      
      if ((lastchar == 65) && (byte == 76))  // already connected
      {
         printf("Already connected to %s\n"
                "Closing connection...\n", par1);

         uart_tx2(close);
         uart_tx2("\r\n");
         
         printf("Try again\n");
         
         for(;;);  // Loop forever
      }
      
      if ((lastchar == 67) && (byte == 79))  // connected
      {
         printf("Port %s open. Connected...\n", par2);
         
         intrinsic_di();
         memcpy(&after, SYSVAR_FRAMES, 3);  // inlines as ldir
         intrinsic_ei();
         
         printf("Response time %lu frames\n"
                "Closing connection\n", after - before);
         
         uart_tx2(close);
         uart_tx2("\r\n");
         
         for(;;);  // Loop forever
      }
   
      if ((lastchar == 68) && (byte == 78))  // dns fail
      {
         printf("DNS lookup failed...\n");
         
         uart_tx2(close);
         uart_tx2("\r\n");
         
         printf("Try again\n");
         
         for(;;);  // Loop forever
      }
      
      if ((lastchar == 69) && (byte == 82))  // dns fail (other)
      {
         printf("An error occurred...\n");
         
         uart_tx2(close);
         uart_tx2("\r\n");
         
         printf("Try again\n");
         
         exit(0);
      }
      
      lastchar = byte;
   }
}
