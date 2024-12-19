#include <stdint.h>
#include "defines.h"

typedef struct NopResponse_ 
{
    uint8_t cmd;
    uint8_t flags;
} NopResponse;

extern char g_wifiSsid[64];
extern char g_wifiPassword[128];

void NetComInit(void);
