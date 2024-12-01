#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "esp.h"
//#include "main.h"
#include "uart.h"

unsigned char esp_test(void);

unsigned char STRING_ESP_TX_AT[] = "AT\r\n";
unsigned char STRING_ESP_TX_AT_GMR[] = "AT+GMR\r\n";

unsigned char STRING_ESP_TX_AT_BPS_Q1[] = "AT+UART_CUR?\r\n";
unsigned char STRING_ESP_RX_AT_BPS_R1[] = "+UART_CUR:%lu";

unsigned char STRING_ESP_TX_AT_BPS_Q2[] = "AT+UART?\r\n";
unsigned char STRING_ESP_RX_AT_BPS_R2[] = "+UART:%lu";

unsigned char STRING_ESP_RX_OK[] = "OK\r\n";

uint32_t common_bps[] = {
   115200UL, 9600UL, 2000000UL, 1000000UL,
   1152000UL, 57600UL, 19200UL, 31250UL, 74880UL
};

typedef struct {
   uint32_t start;
   uint32_t end;
} range_t;

range_t common_bps_range[] = {
   { 1000000UL, 2000000UL },
   { 115200UL, 1000000UL },
   { 300UL, 19200UL },
   { 19200UL, 115200UL }
};

uint32_t esp_bps;
uint16_t esp_response_time_ms;

// BUFFER

unsigned char buffer[256];

unsigned char esp_response_ok(void)
{
   unsigned char ret;
   
   // URR_TIMEOUT, URR_OK = 0, URR_INCOMPLETE, ET_OK = 1
   return (ret = uart_rx_readline_last(buffer, sizeof(buffer)-1)) ? (ret) : (!strcmp(buffer, STRING_ESP_RX_OK));
}

uint32_t esp_prescaler, esp_prescaler_endpoint;

uint32_t esp_bsearch(void)
{
   static uint32_t ok, last;
   uint32_t next;
   
   ok = esp_prescaler;
   next = (ok + esp_prescaler_endpoint)/2;

   do   // do at least once because we don't like to see empty text for high and low end of range
   {
      uart_set_prescaler(next);
      esp_bps = uart_compute_prescaler(next);   // necessary for computing timeout value
      
      if (esp_test() == ET_OK)
      {
         // works
         
         ok = next;
      }
      else
      {
         // fails
         
         esp_prescaler_endpoint = next;
      }
      
      last = next;
      next = (ok + esp_prescaler_endpoint)/2;
   }
   while (last != next);

   return ok;
}

void esp_binary_search_bps(void)
{
   static uint32_t uart_prescaler_lo, uart_prescaler_hi;
   
   esp_prescaler = uart_compute_prescaler(esp_bps);
   
   // find low end of working range

   printf("    High end        ");   // bps is inverse of prescaler
   
   esp_prescaler_endpoint = (esp_prescaler * 4UL) / 5UL;   // ~20% below found
   uart_prescaler_lo = esp_bsearch();
   
   // find high end of working range

   printf("\n    Low end         ");   // bps is inverse of prescaler
   
   esp_prescaler_endpoint = (esp_prescaler * 5UL) / 4UL;   // ~20% above found
   uart_prescaler_hi = esp_bsearch();
   
   // computing bps from prescaler is the same function as computing prescaler from bps with prescaler arg
   
   esp_bps = uart_compute_prescaler((uart_prescaler_lo + uart_prescaler_hi)/2);
}

unsigned char esp_bps_query(unsigned char *send, unsigned char *respond)
{
   static uint32_t temp;
   
   // clear Rx buffer
   
   uart_rx_readline_last(buffer, sizeof(buffer)-1);

   // transmit query
   
   uart_tx(send);
   
   // parse responses
   
   do
   {
      *buffer = 0;
      uart_rx_readline(buffer, sizeof(buffer)-1);
      
      if (sscanf(buffer, respond, &temp) == 1)
      {
         esp_bps = temp;
         return 1;
      }
   }
   while (*buffer);
   
   return 0;
}

void esp_finetune_bps(void)
{
   static uint32_t temp;

   printf("\n  Fine tune working baud rate\n");
   
   if ((esp_bps_query(STRING_ESP_TX_AT_BPS_Q1, STRING_ESP_RX_AT_BPS_R1)) || 
       (esp_bps_query(STRING_ESP_TX_AT_BPS_Q2, STRING_ESP_RX_AT_BPS_R2)))
   {
      printf("  ESP reports %lu", esp_bps);
   }
   else
   {
      // esp firmware does not support bps query

      esp_binary_search_bps();
      printf("\n  Binary search found %lu", esp_bps);
   }
}

unsigned char esp_test(void)
{
   printf("\x08\x08\x08\x08\x08\x08\x08" "%7lu", esp_bps);
   
   uart_tx("\r\n");
   uart_tx(STRING_ESP_TX_AT);

   esp_response_time_ms = (uint16_t)(20000UL / esp_bps) + ESP_FW_RESPONSE_TIME_MS;  // two bit periods
   
   // URR_TIMEOUT, URR_OK = 0, URR_INCOMPLETE, ET_OK = 1
   return esp_response_ok();
}

unsigned char esp_detect_bps(void)
{
   static unsigned char i;
   
   printf("  Trying        ");
   
   // try passed in baud rate
   
   if (esp_bps)
   {
      uart_set_prescaler(uart_compute_prescaler(esp_bps));
      
      if (esp_test() == ET_OK)
      {
         esp_finetune_bps();
         return 1;
      }
   }
   
   // try common bps
   
   for (i = 0; i != sizeof(common_bps)/sizeof(*common_bps); ++i)
   {
      esp_bps = common_bps[i];
      
      uart_set_prescaler(uart_compute_prescaler(esp_bps));
      
      if (esp_test() == ET_OK)
      {
         esp_finetune_bps();
         return 1;
      }
   }
   
   // try ranges
   
   for (i = 0; i != sizeof(common_bps_range)/sizeof(*common_bps_range); ++i)
   {
      esp_bps = common_bps_range[i].end;   // start here because it's faster
      
      do
      {
         uart_set_prescaler(uart_compute_prescaler(esp_bps));
         
         if (esp_test() == ET_OK)
         {
            esp_finetune_bps();
            return 1;
         }
      
         esp_bps = (esp_bps * 80UL) / 81UL;   // 1.25% step size decrease
      }
      while (esp_bps >= common_bps_range[i].start);
   }
   
   return 0;
}
