#define SCREEN_W 256
#define SCREEN_H 192

//#define SCREENSHOT_MODE  // enable to hide ip addresses from the main view

// Degub printing.
//#define DEBUG_TEXT_ENABLED
#ifdef DEBUG_TEXT_ENABLED 
   #define PRINT_DEBUG_TEXT
   #define PRINT_UART_RX_DEBUG_TEXT
#endif

//#define AT_ECHO_ON  // Set echo on.THIS TIMEOUTS FOR SOME REASON!

#define CLOUD_SPRITE_Y 40
#define UDP_LOCAL_PORT "333"
#define UART_TIMEOUT_MS 1000*5

#define RESET_WIFI  // Reset the WIFI module before starting the connection.

#define BUFFER_MAX_SIZE2 512
#define VIDEO_SCREEN_REFRESH_RATE 50 // support only 50 hz refresh rate for now.
#define PROGRAM_CONFIG_FILE "NxtUdpTest.cfg"
 
#define PROG_FAILED prog_failed(__FILE__, __LINE__, 0)
#define PROG_FAILED1(err) prog_failed(__FILE__, __LINE__, err)

// Add a programmatic breakpoint for the CSpect emulator. Note that CSpect should be started with the 
// "-brk" parameter to enable the breakpoint feature.
//
// NOTE: If you get this kind of info warning, it is a bug in the compiler. That is still ok:
//       - "info 218: z80instructionSize() failed to parse line node, assuming 999 bytes
//          'defb   0xfd, 0x00 '"
#define CSPECT_BREAK() \
   __asm\
   defb 0xfd, 0x00\
   __endasm;

#define CSPECT_BREAK_IF(cond) \
if(cond) {\
   __asm\
   defb 0xfd, 0x00\
   __endasm;\
}

extern uint8_t numClonedPackets;
extern uint8_t frameCount8Bit;
extern char serverAddress[16];  // aaa.bbb.ccc.ddd
extern char serverPort[8];  // 1234567
extern char localIpAddress[16];
int16_t GetUsedStack(void);
void prog_failed(char* sourceFile, int32_t lineNum, uint8_t err);
void DrawStatusTextAndPageFlip(char* text);


