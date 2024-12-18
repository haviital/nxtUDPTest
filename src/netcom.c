#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "netcom.h"
#include "esp.h"
#include "uart.h"
#include "uart2.h"

void NetComInit(void)
{
    DrawStatusTextAndPageFlip("Init");
    uart_flush_rx2();

    DrawStatusTextAndPageFlip("Connecting to Wifi");

    // Disable echo
    uart_tx2("ATE1\r\n");
    if(uart_read_expected2("OK") != 0)
        uart_failed(__FILE__, __LINE__);

    // Connect to wifi AP.
    char atcmd[256];
    sprintf(atcmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", g_wifiSsid, g_wifiPassword);
    uart_tx2(atcmd);
    if(uart_read_expected2("OK") != 0)
        uart_failed(__FILE__, __LINE__);

    DrawStatusTextAndPageFlip("Connected to Wifi");

    // Enable single connection mode
    //AT+CIPMUX=0
    uart_tx2("AT+CIPMUX=0\r\n");
    if(uart_read_expected2("OK") != 0)
        uart_failed(__FILE__, __LINE__);
   
    DrawStatusTextAndPageFlip("Create Server connection");
    // Start UDP connection.
    // 2 means UDP(?)
    // 0 means wifi passthrough
    // AT+CIPSTART="UDP","<remote_ip>",<remote_port>,<local_port>,0
    // Could also come: "ALREADY CONNECTED\r\nERROR\n\r"
    sprintf(atcmd, "AT+CIPSTART=\"UDP\",\"%s\",%s,%s,0\r\n", 
        UDP_SERVER_ADDRESS, UDP_SERVER_PORT,UDP_LOCAL_PORT );
    uart_tx2(atcmd);
    if(uart_read_expected_many2("OK", "ERROR") != 0)
        uart_failed(__FILE__, __LINE__);

    DrawStatusTextAndPageFlip("Create Server connection DONE");

#if 0
    // *** Test!

    unsigned char close[20] = "AT+CIPCLOSE";
    unsigned char ipstart_cmd[60] = "AT+CIPSTART=\"TCP\",\"";

    strcat(ipstart_cmd, SERVER_IP_ADDRESS_2);
    strcat(ipstart_cmd, "\",");
    strcat(ipstart_cmd, SERVER_PORT_2);

    uint8_t counter = 10;

    #ifdef PRINT_TO_BUFFER2
    testBuffer2[0] = 0;
    #endif

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

        char text[128];
        sprintf(text, "Test round: %d", 11 - counter);  
        DrawStatusTextAndPageFlip(text);
  
        // Connected ok. Do it again.
        if(--counter==0)
            break;

   }  // repeated connect loop
#endif

 }

void NetComInit_old(void)
{
    // Reset ESP
    // ZXN_NEXTREG(0x02, 0x80);
    // z80_delay_ms(100*8);       // 100ms, about 8x longer for 28MHz
    // XN_NEXTREG(0x02, 0);
    // z80_delay_ms(8000U*8U);      // 8s, about 8x longer for 28MHz

    //printf("Init\r\n");
    DrawStatusTextAndPageFlip("Init");

    esp_response_time_ms = 66 + ESP_FW_RESPONSE_TIME_MS;   // two bit periods at 300bps
    uart_rx_readline_last(buffer, sizeof(buffer)-1);   // clear Rx

    //esp_response_time_ms = 66 + ESP_FW_RESPONSE_TIME_MS;   // two bit periods at 300bps
    esp_response_time_ms = 66 + ESP_FW_RESPONSE_TIME_MS * 100;   // two bit periods at 300bps
   
    //printf("ESP AT+GMR follows...\n");
    //uart_rx_readline_last(buffer, sizeof(buffer)-1);   // clear Rx

    // Send AT command to UART and print response.
    //uart_send_at_cmd(STRING_ESP_TX_AT_GMR);

#if 0 

     // Print the wifi init status
    //uart_send_at_cmd("AT+CWINIT?\r\n");

    // Print the wifi mode
    uart_send_at_cmd("AT+CWMODE?\r\n");

    // Print the wifi state
    uart_send_at_cmd("AT+CWSTATE?\r\n");

    // Set parametes for the AP list.
    // - The first parameter is 1, meaning that the result of the command AT+CWLAP will be ordered according to RSSI;
    // - The second parameter is 31, namely 0x1F, meaning that the corresponding bits of <print mask> are set to 1. All parameters will be shown in the result of AT+CWLAP.
    uart_send_at_cmd("AT+CWLAPOPT=1,31\r\n");

    // List wifi access points
    esp_response_time_ms = 66 + ESP_FW_RESPONSE_TIME_MS * 100*10;   // two bit periods at 300bps
    uart_send_at_cmd("AT+CWLAP\r\n");

    //
    //uart_send_at_cmd("AT+CWLAP=\"5GCPE_5DFEC5\"\r\n");

   //
    //uart_send_at_cmd("AT+CWLAP=\"Koti_9751\"\r\n");
#endif

   DrawStatusTextAndPageFlip("Connecting to Wifi");

    // Disable echo
    uart_send_at_cmd("ATE0\r\n");

    // Set speed.
    //uart_send_at_cmd("AT+UART_CUR=115273,8,1,0,0\r\n");
 
    // Connect to wifi AP.
    char atcmd[256];
    sprintf(atcmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", g_wifiSsid, g_wifiPassword);
    esp_response_time_ms = 66 + ESP_FW_RESPONSE_TIME_MS * 100*10;   // longer timeout
    uart_send_at_cmd(atcmd); 

    DrawStatusTextAndPageFlip("Connected to Wifi");

    // Get my IP address
    // AT+CIFSR
    //uart_send_at_cmd("AT+CIFSR\r\n");

    // Enable single connection mode
    //AT+CIPMUX=0
    //esp_response_time_ms = 66 + ESP_FW_RESPONSE_TIME_MS*10;   // normal timeout
    uart_send_at_cmd("AT+CIPMUX=0\r\n");
    DrawStatusTextAndPageFlip("Create Server connection");

    // Start UDP connection.
    // 2 means UDP(?)
    // 0 means wifi passthrough
    // AT+CIPSTART="UDP","<remote_ip>",<remote_port>,<local_port>,0
    // Could also come: "ALREADY CONNECTED\r\nERROR\n\r"
    sprintf(atcmd, "AT+CIPSTART=\"UDP\",\"%s\",%s,%s,0\r\n", 
        UDP_SERVER_ADDRESS, UDP_SERVER_PORT,UDP_LOCAL_PORT );
    uart_send_at_cmd_custom_response(atcmd, "OK\r\n", "ERROR\r\n"); 
}


/*
void main_esp_detect_bps(void)
{
   printf("Detecting ESP baud rate\n");
   
   if (!esp_detect_bps())
   {
      esp_bps = 115200UL;
      printf("\n  Failed, selecting default");
   }

   printf("\n  Setting uart to %lu\n", esp_bps);
   uart_set_prescaler(uart_compute_prescaler(esp_bps));
}
*/
