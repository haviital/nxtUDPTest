#include "uart2.h"

#include <arch/zxn.h>
#include <arch/zxn/sysvar.h>
#include <input.h>
#include <intrinsic.h>
#include <z80.h>
#include "TextTileMap.h"

#ifdef PRINT_TO_BUFFER2
char testBuffer2[2048];
#endif

uint8_t uart_debug_print = 0;

// Work buffer.
static char buffer2[BUFFER_MAX_SIZE2];

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
   #if UART_SPECIAL_DEBUG_PRINT_ENABLED
   printf("uart_tx2: %s\n", s);
   #endif
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
      
      IO_133B = *s++;
   }
}

// Send data to UART.
uint8_t uart_raw_tx2(unsigned char *s, uint16_t size)
{
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
      
      IO_133B = *s++;
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
   //printf(" uart_rx_char2 ");

   // Wait until there is a byte in rx.
   while (!(IO_133B & 0x01)) // Is there data in RX port?
   {
      if (timeout_ms--==0) 
         return 1; // Timeout

      z80_delay_ms(1*8);   // 8x for 28MHz

      user_break(); 
   }
   unsigned char byte2 = IO_143B;
   #ifdef UART_PRINT_TO_SCREEN2
   uart_pretty_print(byte2);
   #endif
   *ch = byte2;
   return 0;
}

// Wait until there is one byte from uart.
unsigned char uart_rx_char2(void)
{
   char ch=0;
   uint8_t err = uart_rx_char_timeout(&ch, UART_TIMEOUT_MS);
   if(err) 
   {
      printf("timeout error (%lu ms)!\n", (uint16_t)UART_TIMEOUT_MS);
      PROG_FAILED;
   }
   
   return ch;
}

// Flush the receive buffer.
void uart_flush_rx2(void)
{
   // flush read buffer2
   //printf("Flushing: \n");
   uint16_t timeout_ms = UART_TIMEOUT_MS;
   unsigned char c;
   while(1)
   {
      while (IO_133B & 0x01)
      {
        c = IO_143B;
        #ifdef UART_PRINT_TO_SCREEN2
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
uint8_t uart_read_until_expected(char* expected)
{
   uint16_t expectedStringPos = 0;
   const uint16_t expectedLen = strlen(expected);
   const uint16_t timeout_ms = 10;
 
   #ifdef PRINT_UART_RX_DEBUG_TEXT
   TextTileMapPuts("###TX EXPECTED:");
   TextTileMapPuts(expected);
   TextTileMapPutc(' ');
   TextTileMapClearToEol();
   #endif

   // First read out all the garbage before the expected string.
   char ch=0;
   uint8_t err = 0;
   while(true)
   {
      err = uart_rx_char_timeout(&ch, timeout_ms);
      if(err==1) 
         break; // Timeout error.
      else if( err != 0) 
         PROG_FAILED1(err);
         
      #ifdef PRINT_UART_RX_DEBUG_TEXT
      uart_pretty_print(ch);
      #endif

      // Check if the char is expected.
      if(ch==expected[expectedStringPos])
      {
         // Advance the expected string position.
         if(++expectedStringPos >= expectedLen)
            break;  // All checked!
      }
      else
      {
         // Did not find the char belonging to expected string.
         // Start again!
         expectedStringPos = 0;

         // Check if the char is expected.
         if(ch==expected[expectedStringPos])
         {
            // Advance the expected string position.
            if(++expectedStringPos >= expectedLen)
               break;  // All checked!
         }
      }
   }

   return err; // Not found.
}

// Read chars from UART until the expected string is found. 
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

// Read chars from UART until on of the expected strings is found. 
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

      if(uart_debug_print)
         uart_pretty_print(*bufferReadToPtr);
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

// Read until the char is found and return the chars (not ending to \0) before that.
// maxLen should contain the ending null.
uint8_t uart_read_until_char(char untilChar, char* receivedData, uint8_t maxLen)
{
   if(uart_debug_print)
      TextTileMapPuts("uart_read_until_char ");
   uint8_t i=0;
   while(i<maxLen-1)
   {
      // read byte from uart
      char byte = uart_rx_char2();
      if(uart_debug_print)
         uart_pretty_print(byte);

      if(byte==untilChar)
      {
         receivedData[i] = 0;  // ending null
         return 0;  // Found the bookmark char.
      }

      receivedData[i] = byte; // Store the byte read.
      i++;
   }

   return 1; // Not found.
}

// Send data to UDP via UART.
uint8_t uart_send_data_packet2(unsigned char *data, uint8_t len)
{
   if( len==0 )
      return 1;

   // Send AT command to UART to start sending the UDP packet.
   char atcmd[32];
   strcpy(atcmd, "AT+CIPSEND=");
   itoa(len, &(atcmd[11]), 10);
   strcat(atcmd, "\r\n");
   #if UART_SPECIAL_DEBUG_PRINT_ENABLED
   printf("call: %s\n", atcmd);
   #endif
   uart_tx2(atcmd);
   //*buffer = 0;

   // Read response from UART. 
   // There should be: "OK\n\r>\n\r".
   uint8_t err = uart_read_expected2(">");
   if(err>0)
      return err+20;

    // Send data to UART. Check the response in the next frame.
    err = uart_raw_tx2(data, len);
    if(err>0)
      return err+10;

   return 0;
}

// Receive data from UDP via UART.
uint8_t uart_receive_data_packet_if_any(char* receivedData, uint8_t size)
{
   // The received data has the following format:
   // "+IPD,6,192.168.1.100,12345:123456"
   // Where:
   // - 6: the data lenght in bytes
   // - 192.168.1.100: the sender ip address
   // - 12345: the sender ip port
   // - 123456: Actual data

   uart_debug_print = 1;  //!!HV
   screencolour = 18;// pink //16 grey // 12 cyan
   TextTileMapGoto(15, 0);

   // Read UART until the string was found or there is timeout.
   uint8_t err = uart_read_until_expected("+IPD,"); 
   if(err)
      return err;  // Note: 1 is a timeout. Not an error.

   // Read the packet details from UART. The format is: "6,192.168.1.100,12345:" 
   // Get the number of bytes as a string.
   // Read until ':'. Get the amount of data bytes back (as a string).
   //const uint8_t MAX_STR_SIZE = 19; //3+1+15+1+6+1;
   #define MAX_STR_SIZE 27//19
   char receivedFromUart[MAX_STR_SIZE]; // includes the ending null. 
   err = uart_read_until_char(':', receivedFromUart, MAX_STR_SIZE);
   
   //for(;;); //!!HV
   
   if(err)
      return 20 + err;

   uart_debug_print = 0;  //!!HV

   //#ifdef PRINT_UART_RX_DEBUG_TEXT
   //uart_pretty_print(ch);
   //#endif

   // Check the minimum lenght.
   uint16_t serverAddressLen = strlen(serverAddress);
   uint16_t serverPortLen = strlen(serverPort);
   uint16_t len = strlen(receivedFromUart);
   if( len < 1 + 1 + serverAddressLen + 1 + serverPortLen + 1 )
      PROG_FAILED;

   // get data lenght.
   char tmp[32];
   char* posPtr = strchr(receivedFromUart, ',');
   if(posPtr==NULL)  
      PROG_FAILED;
   len = posPtr-receivedFromUart;
   strncpy(tmp, receivedFromUart,len);
   tmp[len] = '\0';

   // Convert the string to a number.
   uint8_t dataLen = atoi(tmp);
   if(dataLen!=size)
      PROG_FAILED;

   // Check ip address.
   posPtr++; // ','
   if( strncmp(posPtr, serverAddress, serverAddressLen ) != 0)
      PROG_FAILED;
   posPtr += serverAddressLen;

   // Check the port.
   posPtr++; // ','
   if( strncmp(posPtr, serverPort, serverPortLen ) != 0)
      PROG_FAILED;
      
   // Read actual data.
   for(uint8_t i=0; i<dataLen; i++)
      receivedData[i] = uart_rx_char2();

   return 0;
}





