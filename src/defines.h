#define SCREEN_X 256
#define SCREEN_Y 192

//#define NO_GFX // Do not draw UI gfx. It allows to see debug prints.
//#define UART_PRINT_TO_SCREEN2 // this one. 
//#define UART_SPECIAL_DEBUG_PRINT_ENABLED 1
#define UART_SPECIAL_DEBUG_PRINT_ENABLED 0
// // #define PRINT_TO_BUFFER2  // Either this or UART_PRINT_TO_SCREEN2
//#define AT_ECHO_ON  // Set echo on.

#define CLOUD_SPRITE_Y 40

#define UDP_SERVER_PORT "4444"
#define UDP_SERVER_ADDRESS "192.168.100.135"
#define UDP_LOCAL_PORT "333"


#define UART_TIMEOUT_MS 1000*5

#define SERVER_IP_ADDRESS_2 "192.168.100.135"
#define SERVER_PORT_2 "8000"


#define BUFFER_MAX_SIZE2 512

#define PROG_FAILED prog_failed(__FILE__, __LINE__, 0)
#define PROG_FAILED1(err) prog_failed(__FILE__, __LINE__, err)

extern uint8_t numClonedPackets;
extern uint8_t frameCount8Bit;

void FlipBorderColor(bool reset);
int16_t GetUsedStack(void);
void prog_failed(char* sourceFile, int32_t lineNum, uint8_t err);
bool CheckMemoryGuards(void);


