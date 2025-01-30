#include <stdint.h>
#include "defines.h"

void DrawIpAddressDialog(uint8_t row, uint8_t col);
void InputIPAddress(uint8_t row, uint8_t col);
void printIpAddressesOnUI(void);

// 16*BG + FG
#define TT_COLOR_IP_DIALOG_CURSOR 16*TT_COLOR_WHITE + TT_COLOR_BLUE
#define TT_COLOR_IP_DIALOG_MAIN_TEXT 16*TT_COLOR_BLUE + TT_COLOR_WHITE
#define TT_COLOR_IP_DIALOG_INFO_TEXT 16*TT_COLOR_BLUE + TT_COLOR_WHITE
#define TT_COLOR_IP_DIALOG_IP_ADDRESS_FIELD 16*TT_COLOR_CYAN + TT_COLOR_WHITE
#define TT_COLOR_IP_DIALOG_IP_ADDRESS_POINTS 16*TT_COLOR_BLUE + TT_COLOR_WHITE

