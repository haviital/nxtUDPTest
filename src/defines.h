#define SCREEN_X 256
#define SCREEN_Y 192

#define NO_GFX // Do not draw UI gfx. It allows to see debug prints.
//#define UART_PRINT_TO_SCREEN2 
//#define UART_SPECIAL_DEBUG_PRINT_ENABLED 1
#define UART_SPECIAL_DEBUG_PRINT_ENABLED 0
// // #define PRINT_TO_BUFFER2  // Either this or UART_PRINT_TO_SCREEN2
//#define AT_ECHO_ON  // Set echo on.THIS TIMEOUTS FOR SOME REASON!

#define CLOUD_SPRITE_Y 40
#define UDP_LOCAL_PORT "333"
#define UART_TIMEOUT_MS 1000*5

#define RESET_WIFI  // Reset the WIFI module before starting the connection.

#define BUFFER_MAX_SIZE2 512

#define VIDEO_SCREEN_REFRESH_RATE 50 // support only 50 hz refresh rate for now.

#define PROG_FAILED prog_failed(__FILE__, __LINE__, 0)
#define PROG_FAILED1(err) prog_failed(__FILE__, __LINE__, err)

// Add a programmatic breakpoint for the CSpect emulator. Note that CSpect should be started with the 
// "-brk" parameter to enable the breakpoint feature.
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
void FlipBorderColor(bool reset);
int16_t GetUsedStack(void);
void prog_failed(char* sourceFile, int32_t lineNum, uint8_t err);
void DrawStatusTextAndPageFlip(char* text);


