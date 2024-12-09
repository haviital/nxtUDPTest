#include <stdint.h>

#define UDP_SERVER_PORT "4444"
#define UDP_SERVER_ADDRESS "192.168.100.133"
#define UDP_LOCAL_PORT "333"

typedef struct NopResponse_ 
{
    uint8_t cmd;
    uint8_t flags;
} NopResponse;

extern char g_wifiSsid[64];
extern char g_wifiPassword[128];

void NetComInit(void);
