#include "uart2.h"

#include <arch/zxn.h>
#include <arch/zxn/sysvar.h>
#include <input.h>
#include <intrinsic.h>
#include <z80.h>

#ifdef PRINT_TO_BUFFER2
char testBuffer2[2048];
#endif

static char buffer2[BUFFER_MAX_SIZE2];

// USER BREAK

unsigned char err_break[] = "D BREAK - no repea" "\xf4";

void user_break(void)
{
   if (in_key_pressed(IN_KEY_SCANCODE_SPACE | 0x8000))  // CAPS+SPACE
      exit((int)err_break);
}

char* replaceCrAndLn2(char* str, char* newStr)
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

void myitoa_not_zero(uint8_t num, char* str) 
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

void myitoa(uint8_t num, char* str) 
{
   if(num==0)
      strcpy(str, "0");
   else 
      myitoa_not_zero(num, str);   
}

uint8_t myatoi(char* str) 
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



void uart_tx2(unsigned char *s)
{
   while (*s)
   {
      // Wait until the previuos data has been sent.
      while (IO_133B & 0x02)  // Is TX buffer full?
         user_break();
      
      IO_133B = *s++;
   }
}

void uart_raw_tx2(unsigned char *s, uint16_t size)
{
   while (size)
   {
      // Wait until the previuos data has been sent.
      while (IO_133B & 0x02)  // Is TX buffer full?
         user_break();
      
      IO_133B = *s++;
      size--;
   }
}

// Wait until there is one byte from uart.
unsigned char uart_rx_char2(void)
{
   //printf(" uart_rx_char2 ");

   // Wait until there is a byte in rx.
   while (!(IO_133B & 0x01)) // Is there data in RX port?
   {
      user_break(); 
   }
   unsigned char byte2 = IO_143B;
   #ifdef PRINT_TO_SCREEN2
   if(byte2>31)
      printf("%c", byte2);
   else
      printf("<%u>", byte2);
   #elif defined(PRINT_TO_BUFFER2)
   char* text_[16];
   if(byte2>31)
   {
      sprintf(text_, "%c", byte2);
      strcat(testBuffer2, text_);
   }
   else
   {
      sprintf(text_, "<%u>", byte2);
      strcat(testBuffer2, text_);
   }
   #endif
   return( byte2 );
}

void uart_flush_rx2(void)
{
   // flush read buffer2
   //printf("Flushing: \n");
   unsigned char c;
   while(1)
   {
      while (IO_133B & 0x01)
      {
        c = IO_143B;
        #ifdef PRINT_TO_SCREEN2
        if(c>31)
           printf("%c", c);
        else
           printf("<%u>", c);
        #elif defined(PRINT_TO_BUFFER2)
         char* text_[16];
         if(c>31)
         {
            sprintf(text_, "%c", c);
            strcat(testBuffer2, text_);
         }
         else
         {
            sprintf(text_, "<%u>", c);
            strcat(testBuffer2, text_);
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

uint8_t uart_read_expected2(char* expected)
{
   //printf("uart_read_expected2: %s", expected);
   buffer2[0] = 0; // clear// BUFFER_MAX_SIZE2];
   char* bufferReadToPtr = buffer2;
   char* stringStartPtr = buffer2;
   uint16_t readLen = 0;

   while(readLen<BUFFER_MAX_SIZE2)
   {
 
      // read byte from uart
      *bufferReadToPtr = uart_rx_char2();
      readLen++;

      // if(*bufferReadToPtr>31)
      //     printf("%c", *bufferReadToPtr);
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

uint8_t uart_read_expected_many2(char* expected1, char* expected2)
{
   // printf("uart_read_expected_many2: #1=%s(%u) or #2=%s(%u)\n", 
   //    expected1, strlen(expected1), expected2, strlen(expected2));

   buffer2[0] = 0; // clear// BUFFER_MAX_SIZE2];
   char* bufferReadToPtr = buffer2;
   char* stringStartPtr1 = buffer2;
    char* stringStartPtr2 = buffer2;
   uint16_t readLen = 0;

   while(readLen<BUFFER_MAX_SIZE2)
   {
 
      // read byte from uart
      *bufferReadToPtr = uart_rx_char2();
      readLen++;

      // if(*bufferReadToPtr>31)
      //     printf("%c", *bufferReadToPtr);
      // else
      //    printf("<%u>", *bufferReadToPtr);

      //printf(" %u: ", readLen);

      //char text[128];
      if(readLen>=strlen(expected1))
      {
         // TEST
         // char* pastEndPtr=stringStartPtr1 + strlen(expected1);
         // if(pastEndPtr>bufferReadToPtr+1)
         //    pastEndPtr = bufferReadToPtr+1;
         // uint8_t len = pastEndPtr - stringStartPtr1;
         // memcpy(text, stringStartPtr1, len);
         // text[len] = 0;
         // replaceCrAndLn2(text, text);
         // printf("#1(%s) ", text);

         if(strncmp(stringStartPtr1, expected1, strlen(expected1))==0)
            return 0;  // Found!
         stringStartPtr1++;
      }

      if(readLen>=strlen(expected2))
      {
         // TEST
         // char* pastEndPtr=stringStartPtr2 + strlen(expected2);
         // if(pastEndPtr>bufferReadToPtr+1)
         //    pastEndPtr = bufferReadToPtr+1;
         // uint8_t len = pastEndPtr - stringStartPtr2;
         // memcpy(text, stringStartPtr2, len);
         // text[len] = 0;
         // replaceCrAndLn2(text, text);
         // printf("#2(%s) ", text);

         if(strncmp(stringStartPtr2, expected2, strlen(expected2))==0)
            return 0;  // Found!
         stringStartPtr2++;
      }

      bufferReadToPtr++;

      //printf("\n");

   }  // response bytes comparison loop

   return 1; // Not found.   
}

uint8_t uart_send_data_packet2(unsigned char *data, uint8_t len)
{
   if( len==0 )
      return 1;

   // Send AT command to UART to start sending the UDP packet.
   char atcmd[32];
   strcpy(atcmd, "AT+CIPSEND=");
   (void)myitoa_not_zero(len, &(atcmd[11]));
   strcat(atcmd, "\r\n");
   if(UART_DEBUG_PRINT_ENABLED) printf("call: %s\n", atcmd);
   uart_tx2(atcmd);
   //*buffer = 0;

   // Read response from UART. 
   // There should be: "OK\n\r>\n\r".
   uint8_t err = uart_read_expected2(">");
   if(err>0)
      return err;

    // Send data to UART. Check the response in the next frame.
    uart_raw_tx2(data, len);
    return 0;
}

void prog_failed(char* sourceFile, int32_t lineNum)
{
   //printf("uart_failed()\n");
   char text[128];
   sprintf(text, "FAILED in file: %s (%lu)", sourceFile, lineNum);
   printf(text);
   for(;;);
}


