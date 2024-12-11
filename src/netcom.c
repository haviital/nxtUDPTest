#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "netcom.h"
#include "esp.h"
#include "uart.h"

void NetComInit(void)
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
    //uart_send_at_cmd_and_print(STRING_ESP_TX_AT_GMR);

#if 0 

     // Print the wifi init status
    //uart_send_at_cmd_and_print("AT+CWINIT?\r\n");

    // Print the wifi mode
    uart_send_at_cmd_and_print("AT+CWMODE?\r\n");

    // Print the wifi state
    uart_send_at_cmd_and_print("AT+CWSTATE?\r\n");

    // Set parametes for the AP list.
    // - The first parameter is 1, meaning that the result of the command AT+CWLAP will be ordered according to RSSI;
    // - The second parameter is 31, namely 0x1F, meaning that the corresponding bits of <print mask> are set to 1. All parameters will be shown in the result of AT+CWLAP.
    uart_send_at_cmd_and_print("AT+CWLAPOPT=1,31\r\n");

    // List wifi access points
    esp_response_time_ms = 66 + ESP_FW_RESPONSE_TIME_MS * 100*10;   // two bit periods at 300bps
    uart_send_at_cmd_and_print("AT+CWLAP\r\n");

    //
    //uart_send_at_cmd_and_print("AT+CWLAP=\"5GCPE_5DFEC5\"\r\n");

   //
    //uart_send_at_cmd_and_print("AT+CWLAP=\"Koti_9751\"\r\n");
#endif

   DrawStatusTextAndPageFlip("Connecting to Wifi");

    // Disable echo
    uart_send_at_cmd_and_print("ATE0\r\n");

    // Connect to wifi AP.
    char atcmd[256];
    sprintf(atcmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", g_wifiSsid, g_wifiPassword);
    esp_response_time_ms = 66 + ESP_FW_RESPONSE_TIME_MS * 100*10;   // longer timeout
    uart_send_at_cmd_and_print(atcmd); 

    DrawStatusTextAndPageFlip("Connected to Wifi");

    // Get my IP address
    // AT+CIFSR
    //uart_send_at_cmd_and_print("AT+CIFSR\r\n");

    // Enable single connection mode
    //AT+CIPMUX=0
    //esp_response_time_ms = 66 + ESP_FW_RESPONSE_TIME_MS*10;   // normal timeout
    uart_send_at_cmd_and_print("AT+CIPMUX=0\r\n");
    DrawStatusTextAndPageFlip("Create Server connection");

    // Start UDP connection.
    // 2 means UDP(?)
    // 0 means wifi passthrough
    // AT+CIPSTART="UDP","<remote_ip>",<remote_port>,<local_port>,0
    sprintf(atcmd, "AT+CIPSTART=\"UDP\",\"%s\",%s,%s,0\r\n", 
        UDP_SERVER_ADDRESS, UDP_SERVER_PORT,UDP_LOCAL_PORT );
    uart_send_at_cmd_and_print(atcmd); 
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
