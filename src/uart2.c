#include "uart2.h"

#include <arch/zxn.h>
#include <arch/zxn/sysvar.h>
#include <input.h>
#include <intrinsic.h>
#include <z80.h>
#include "TextTileMap.h"

uint8_t uart_debug_print = 0;

// Work buffer.
#define UART_SMALL_BUFFER_SIZE 128
static char uart_small_buffer[UART_SMALL_BUFFER_SIZE];

// Uart log buffer.
uint16_t uart_log_ring_buffer_pos = 0;
char uart_log_ring_buffer[UART_LOG_RING_BUFFER_SIZE];
bool uart_log_ring_buffer_wrapped = false;

// Ring buffer methods
void add_to_log_ring_buffer(char c)
{
   uart_log_ring_buffer[uart_log_ring_buffer_pos++] = c;
   if(uart_log_ring_buffer_pos>=UART_LOG_RING_BUFFER_SIZE)
   {
      uart_log_ring_buffer_pos = 0;
      uart_log_ring_buffer_wrapped = true;
   }
}
void add_to_log_ring_buffer_with_color(char c, uint8_t color)
{
   log_buffer_set_ink_color(color);
   add_to_log_ring_buffer(c);
   log_buffer_set_ink_color(INK_YELLOW);
}
void add_str_to_log_ring_buffer(char* s)
{
   while(*s)
      add_to_log_ring_buffer(*s++);
}
void add_str_to_log_ring_buffer_with_color(char* s, uint8_t color)
{
   log_buffer_set_ink_color(color);
   add_str_to_log_ring_buffer(s);
   log_buffer_set_ink_color(INK_YELLOW);
}
void log_buffer_set_ink_color(uint8_t c)
{
   add_to_log_ring_buffer((char)0x10); // Set ink color...
   add_to_log_ring_buffer((char)(0x30 + c));  // ...to => (char)(0x30 + c)
}
void print_log_buffer(void)
{
   uint16_t pos = uart_log_ring_buffer_pos;
   uint16_t pastEndIndex = UART_LOG_RING_BUFFER_SIZE;
   if(!uart_log_ring_buffer_wrapped)
   {
      pos = 0;
      pastEndIndex = uart_log_ring_buffer_pos;
   }
   for(uint16_t i=0; i<pastEndIndex; i++)
   {
      char c = uart_log_ring_buffer[pos++];

      // Handle non-visible characters.
      if(c=='\r') 
         c='r';
      else if(c=='\n') 
         c='n';
      else if(c==0x10) // Preserve ink color control char.
         ;
      else if(c<32)
         c = '.';

      printf("%c",c);

      // Check wrapping.
      if(pos>=UART_LOG_RING_BUFFER_SIZE)
         pos = 0;
   }
}

// USER BREAK
unsigned char err_break[] = "D BREAK - no repea" "\xf4";

// Check for the user break (caps shift + space).
void user_break(void)
{
   if (in_key_pressed(IN_KEY_SCANCODE_SPACE | 0x8000))  // CAPS+SPACE
      exit((int)err_break);

}

// Make LR and CR visible.
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

// Send string to UART.
void uart_tx2(unsigned char *s)
{
   add_to_log_ring_buffer((char)0x10); // Set ink color...
   add_to_log_ring_buffer((char)0x34);  // ...to GREEN => (char)(0x30 + 0x04)

   uint16_t timeout_ms = UART_TIMEOUT_MS;
   while (*s)
   {
      // Wait until the previuos data has been sent.
      while (IO_133B & 0x02)  // Is TX buffer full?
      {
         if (!(timeout_ms--)) 
         {
            printf("Timeout error! ");
            PROG_FAILED;
        }
         z80_delay_ms(1*8);   // 8x for 28MHz

         user_break();
      }
      
      IO_133B = *s;
      add_to_log_ring_buffer(*s);
      s++;
   }

   add_to_log_ring_buffer((char)0x10); // Set ink color...
   add_to_log_ring_buffer((char)0x36);  // ...to YELLOW => (char)(0x30 + 0x6)
}

// Send data to UART.
uint8_t uart_raw_tx2(unsigned char *s, uint16_t size)
{
   // Print to log buffer.
   add_to_log_ring_buffer((char)0x10); // Set ink color...
   add_to_log_ring_buffer((char)0x34);  // ...to GREEN => (char)(0x30 + 0x04)

   add_str_to_log_ring_buffer(" <TX RAW DATA:");
   char text[8];
   ltoa(size, text, 10);
   add_str_to_log_ring_buffer(text);
   add_str_to_log_ring_buffer("B> ");

   add_to_log_ring_buffer((char)0x10); // Set ink color...
   add_to_log_ring_buffer((char)0x36);  // ...to YELLOW => (char)(0x30 + 0x6)

   uint16_t timeout_ms = UART_TIMEOUT_MS;
   while (size)
   {
      // Wait until the previuos data has been sent.
      while (IO_133B & 0x02)  // Is TX buffer full?
      {
         if (!(timeout_ms--)) 
         {
            //printf("Timeout error! ");
            //PROG_FAILED;
            return 1; // Timeout
         }
         z80_delay_ms(1*8);   // 8x for 28MHz

         user_break();
      }
      
      IO_133B = *s;
      //add_to_log_ring_buffer(*s);
      s++;

      size--;
   }

   return 0;
}

// Check if there is incoming data in UART.
bool uart_available_rx2(void)
{
   return (IO_133B & 0x01); // Is there data in RX port?
}

// Check and wait for a while if there is incoming data in UART.
bool uart_available_rx_wait_once(uint16_t waitTimeMs)
{
   if(IO_133B & 0x01) // Is there data in RX port?
      return true;

   z80_delay_ms(waitTimeMs*8);
   return (IO_133B & 0x01); // Is there data in RX port?
 }

// Print the char and make non-printing characters visible.
void uart_pretty_print(char c)
{
   if(c>31)
      TextTileMapPutc(c);
   else if(c==10)
      TextTileMapPuts("n");
   else if(c==13)
      TextTileMapPuts("r");
   else
   {
      char text[6];
      char tmpStr[4];
      strcpy(text, "<");
      itoa((int8_t)c, tmpStr, 10);
      strcat(text, tmpStr); 
      strcat(text, ">");
      TextTileMapPuts(text);
   }
}

// Wait until there is one byte from uart.
uint8_t uart_rx_char_timeout(unsigned char* ch, uint16_t timeout_ms)
{
   // Wait until there is a byte in rx.
   while (!(IO_133B & 0x01)) // Is there data in RX port?
   {
      if (timeout_ms--==0) 
         return 1; // Timeout

      z80_delay_ms(1*8);   // 8x for 28MHz

      user_break(); 
   }
   unsigned char byte2 = IO_143B;
   *ch = byte2;
   add_to_log_ring_buffer(byte2);
   return 0;
}

// Wait until there is one byte from uart.
unsigned char uart_rx_char2(void)
{
   char ch=0;
   uint8_t err = uart_rx_char_timeout(&ch, UART_TIMEOUT_MS);
   if(err) 
   {
      //printf("timeout error (%lu ms)!\n", (uint16_t)UART_TIMEOUT_MS);
      PROG_FAILED;
   }
   
   return ch;
}

// Flush the receive buffer.
void uart_flush_rx2(void)
{
   // flush read buffer2
   uint16_t timeout_ms = UART_TIMEOUT_MS;
   unsigned char c;
   while(1)
   {
      while (IO_133B & 0x01)
      {
         c = IO_143B;
   
         add_to_log_ring_buffer(c);

         if (!(timeout_ms--)) 
         {
            printf("Timeout error! ");
            PROG_FAILED;
         }
         z80_delay_ms(1*8);   // 8x for 28MHz

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

// Read chars from UART until the expected string is found. 
uint8_t uart_read_expected2(char* expected)
{
   uart_small_buffer[0] = 0; // clear
   char* bufferReadToPtr = uart_small_buffer;
   char* stringStartPtr = uart_small_buffer;
   uint16_t readLen = 0;

   while(readLen<UART_SMALL_BUFFER_SIZE)
   {
 
      // read byte from uart
      *bufferReadToPtr = uart_rx_char2();
      readLen++;

      if(readLen>=strlen(expected))
      {
         if(strncmp(stringStartPtr, expected, strlen(expected))==0)
            return 0;  // Found!
         stringStartPtr++;
      }

      bufferReadToPtr++;

   }  // response bytes comparison loop

   return 1; // Not found. Tiemout.
}

// Read chars from UART until on of the expected strings is found. 
// Returned foundStringOrdinal contains 0 or 1 depending on which string was found.
uint8_t uart_read_expected_many2(char* expected1, char* expected2, /*OUT*/uint8_t* foundStringOrdinal)
{
   uart_small_buffer[0] = 0; // clear
   char* bufferReadToPtr = uart_small_buffer;
   char* stringStartPtr1 = uart_small_buffer;
   char* stringStartPtr2 = uart_small_buffer;
   uint16_t readLen = 0;

   while(readLen<UART_SMALL_BUFFER_SIZE)
   {
      // read byte from uart
      *bufferReadToPtr = uart_rx_char2();
      readLen++;

      if(uart_debug_print)
         uart_pretty_print(*bufferReadToPtr);

      //char text[128];
      if(readLen>=strlen(expected1))
      {
         if(strncmp(stringStartPtr1, expected1, strlen(expected1))==0)
         {
            *foundStringOrdinal = 0;
            return 0;  // Found!
         }
         stringStartPtr1++;
      }

      if(readLen>=strlen(expected2))
      {
         if(strncmp(stringStartPtr2, expected2, strlen(expected2))==0)
         {
            *foundStringOrdinal = 1;
            return 0;  // Found!
         }
         stringStartPtr2++;
      }
      bufferReadToPtr++;
   }  // response bytes comparison loop

   return 1; // Not found.   
}

// Read until the char is found and return the chars before that.
// Returns a string parameter with an ending '\0'.
// If the maxlen is exceeded, returns 1 (=not found).
// The maxLen parameter should contain the ending null.
uint8_t uart_read_and_get_until_char(char untilChar, char* receivedData, uint8_t maxLen)
{
   if(uart_debug_print)
   {
      //   TextTileMapPuts("uart_read_and_get_until_char ");
      TextTileMapPuts(" READ_UNTIL>>");
      TextTileMapPutc(untilChar);
      TextTileMapPuts("<< ");
   }

   uint8_t i=0;
   while(i<maxLen)
   {
      // read byte from uart
      char byte = uart_rx_char2();
      if(uart_debug_print)
         uart_pretty_print(byte);

      if(byte==untilChar)
      {
         if(uart_debug_print) TextTileMapPuts(" FOUND>>");
         receivedData[i] = 0;  // ending null
         return 0;  // Found the bookmark char.
      }

      receivedData[i] = byte; // Store the byte read.
      i++;
   }

   return 1; // Not found.
}







