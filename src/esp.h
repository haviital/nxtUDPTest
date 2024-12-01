#ifndef ESP_H
#define ESP_H

#include <stdint.h>

// MESSAGES

extern unsigned char STRING_ESP_TX_AT[];
extern unsigned char STRING_ESP_TX_AT_GMR[];

extern unsigned char STRING_ESP_RX_OK[];
    
extern unsigned char buffer[256];

//

#define ESP_FW_RESPONSE_TIME_MS  5
#define ET_OK  1

extern uint32_t esp_bps;   // detected esp baud rate
extern uint16_t esp_response_time_ms;   // timeout in milliseconds

extern unsigned char esp_response_ok(void);
extern unsigned char esp_detect_bps(void);

#endif
