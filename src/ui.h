#include <stdint.h>
#include "defines.h"

void DrawIpAddressDialog(uint8_t row, uint8_t col);
void InputIPAddress(uint8_t row, uint8_t col);
void printIpAddressesOnUI(void);

#define TT_COLOR_IP_DIALOG_CURSOR 144 // grey fg, blue bg
#define TT_COLOR_IP_DIALOG_MAIN_TEXT 86  // yellow fg, green bg
#define TT_COLOR_IP_DIALOG_INFO_TEXT 85  // light grey fg, green bg
#define TT_COLOR_IP_DIALOG_IP_ADDRESS_FIELD 222 // white fg on dark cyan bg
#define TT_COLOR_IP_DIALOG_IP_ADDRESS_POINTS 94 // white fg, green bg

