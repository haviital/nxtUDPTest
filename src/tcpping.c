// tcpping
// Johan Engdahl

// zcc +zxn -v -startup=30 -clib=sdcc_iy -SO3 --max-allocs-per-node200000 tcpping.c -o tcpping -subtype=dot -Cz"--clean" -create-app

#include <arch/zxn.h>
#include <arch/zxn/sysvar.h>
#include <input.h>
#include <intrinsic.h>
#include <z80.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma printf = "%lu %s %u %c"
#pragma output CLIB_EXIT_STACK_SIZE = 1

#define PRINT_TO_BUFFER
#ifdef PRINT_TO_BUFFER
static char testBuffer[2048];
#endif

#define BUFFER_MAX_SIZE 2048
static char buffer[BUFFER_MAX_SIZE];

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

// Wait until there is one byte from uart.
unsigned char uart_rx2_char(void)
{
   // Wait until there is a byte in rx.
   while (!(IO_133B & 0x01))
   {
      user_break(); 
   }
   unsigned char byte2 = IO_143B;
   // if(byte2>31)
   //    printf("%c", byte2);
   // else
   //    printf("<%u>", byte2);
   #ifdef PRINT_TO_BUFFER
   char* text_[16];
   if(byte2>31)
   {
      sprintf(text_, "%c", byte2);
      strcat(testBuffer, text_);
   }
   else
   {
      sprintf(text_, "<%u>", byte2);
      strcat(testBuffer, text_);
   }
   #endif
   return( byte2 );
}

void uart_flush_rx(void)
{
   // flush read buffer
   printf("Flushing: \n");
   unsigned char c;
   while(1)
   {
      while (IO_133B & 0x01)
      {
         c = IO_143B;
         //if(c>31)
         //   printf("%c", c);
         //else
         //   printf("<%u>", c);
         #ifdef PRINT_TO_BUFFER
         char* text_[16];
         if(c>31)
         {
            sprintf(text_, "%c", c);
            strcat(testBuffer, text_);
         }
         else
         {
            sprintf(text_, "<%u>", c);
            strcat(testBuffer, text_);
         }
         #endif
         user_break();
      }

      //
      // wait for 5+ frames
      //for (uint16_t len = 0; len < 10000; len++);
      z80_delay_ms(1*8);   // 8x for 28MHz
      
      // If still not any data to read, exit the loop.
      if(!(IO_133B & 0x01))
         break;
   }
}
uint8_t uart_read_expected(char* expected)
{
   //printf("uart_read_expected: %s", expected);
   buffer[0] = 0; // clear// BUFFER_MAX_SIZE];
   char* bufferReadToPtr = buffer;
   char* stringStartPtr = buffer;
   uint16_t readLen = 0;

   while(readLen<BUFFER_MAX_SIZE)
   {
 
      // read byte from uart
      *bufferReadToPtr = uart_rx2_char();
      readLen++;

      // if(*bufferReadToPtr>31)
      //    printf("%c", *bufferReadToPtr);
      // else
      //    printf("<%u>", *bufferReadToPtr);

      if(readLen>=strlen(expected))
      {
         if(strncmp(stringStartPtr, expected, strlen(expected))==0)
            return 0;  // Found!
         stringStartPtr++;
      }

      bufferReadToPtr++;

   }  // response bytes comparison loop

   return 1; // Not found.
}

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
   
   unsigned char c;
   
   // restore on exit
   
   //old_cpu_speed = ZXN_READ_REG(0x07) & 0x03;
   //old_uart = IO_153B & 0x40;

   //atexit(cleanup);
   
   //ZXN_NEXTREG(0x07, 0x03);   // 28MHz
   //IO_153B = 0x00;   // select esp uart
   
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
   
   
   const char* par1 = "192.168.100.133";
   const char* par2 = "8000";
   strcat(ipstart_cmd, par1);
   strcat(ipstart_cmd, "\",");
   strcat(ipstart_cmd, par2);

   lastchar = 0;
   
   uint8_t counter = 10;

   #ifdef PRINT_TO_BUFFER
   testBuffer[0] = 0;
   #endif

   uart_flush_rx();

   // Do a connection open and close 10 times
   printf("Start testing! round: ");
   memcpy(&beforeTest, SYSVAR_FRAMES, 3);  // inlines as ldir
   while (1)
   {

      // *** ping
      {
         //printf("\nPinging %s at port %s\n", par1, par2);
         //printf("open ");
         uart_tx2(ipstart_cmd);
         uart_tx2("\r\n");
         
         intrinsic_di();
         memcpy(&before, SYSVAR_FRAMES, 3);  // inlines as ldir
         intrinsic_ei();
      }

      // Read consecutive bytes in a loop 
      uint32_t testCounter=0;
      uint32_t testCounter2=0;
      while(1)
      {
         testCounter++;

         // read byte from uart
         byte = uart_rx2_char();

         // has been connected
         if ((lastchar == 67) && (byte == 79))  // "CO" ==> connected
         {
            // *** SUCCEEDED!

            //printf("Port %s open. Connected... testCounter=%lu\n", par2, testCounter);

            //printf("close ");
           
            intrinsic_di();
            memcpy(&after, SYSVAR_FRAMES, 3);  // inlines as ldir
            intrinsic_ei();
            
            // printf("Response time %lu frames\n"
            //       "Closing connection\n", after - before);
            printf("%u ", counter);

            uart_tx2(close);
            uart_tx2("\r\n");

            // Note: you MUST read this before calling open again. Otherwise the connection 
            //       might still be not yet closed and the open fails.
            if(uart_read_expected("CLOSED") == 0)
            {
               break; // Found!
            }
            // Not found
            #ifdef PRINT_TO_BUFFER
            printf("\ntestBuffer=%s",testBuffer);
            #endif
            printf("Did not found 'CLOSED'. Try again\n");
            for(;;);  // Loop forever
         }
            
         if ((lastchar == 65) && (byte == 76))  // "AL" ==> already connected
         {
            printf("Already connected to %s\n"
                  "Closing connection...\n", par1);

            uart_tx2(close);
            uart_tx2("\r\n");
            
            printf("Try again\n");
            
            for(;;);  // Loop forever
         }
         
         if ((lastchar == 68) && (byte == 78))  // "DN" ==> dns fail
         {
            printf("DNS lookup failed...\n");
            
            uart_tx2(close);
            uart_tx2("\r\n");
            
            printf("Try again\n");
            
            for(;;);  // Loop forever
         }
         
         if ((lastchar == 69) && (byte == 82))  // "ER" ==> dns fail (other)
         {
            printf("An error occurred...\n");
            
            z80_delay_ms(65000);   // 8x for 28MHz

            uart_flush_rx();

            uart_tx2(close);
            uart_tx2("\r\n");

            #ifdef PRINT_TO_BUFFER
            printf("\ntestBuffer=%s",testBuffer);
            #endif

            printf("Try again\n");
            
            for(;;);  // Loop forever
         }
         
         lastchar = byte;

      }  // response bytes comparison loop

      //
      lastchar = 0;

      // Connected ok. Do it again.
      if(--counter==0)
         break;

   }  // repeated connect loop

   memcpy(&afterTest, SYSVAR_FRAMES, 3);  // inlines as ldir
   printf("\nTEST done! Testing 10 times took %lu frames.",afterTest - beforeTest);
   
   return 0;
}
