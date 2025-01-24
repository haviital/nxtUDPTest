#include <stdint.h>
#include "defines.h"

// *** The "PING" message
#define MSG_ID_NOP 0
typedef struct NopResponse_ 
{
    uint8_t cmd;
    uint8_t flags;
} NopResponse;

// NOTE: The small token (4 chars) works as a security measure as the test will be stopped at the 
// first invalid packet. If invalid packets are tolerated, the token must be longer so that it is
// not possible to guess it by brute force i.e. trying all possible combinations.

// *** The Loopback request packet
#define MSG_ID_TESTLOOPBACK 18
#define MSG_TESTLOOPBACK_RANDOM_DATA_SIZE 32
#define MSG_TESTLOOPBACK_REQUEST_STRUCT_SIZE (3+MSG_TESTLOOPBACK_RANDOM_DATA_SIZE)
#define MSG_TESTLOOPBACK_RESPONSE_STRUCT_SIZE (2+MSG_TESTLOOPBACK_RANDOM_DATA_SIZE)
typedef struct TestLoopBackRequest_ 
{
    uint8_t cmd;  // The command must be the first itrem in all packets. Always MSG_ID_TESTLOOPBACK.
    uint8_t loopPacketCount; // The number of packets the server sends back.
    uint8_t packetSize;  // The size of the payload data
    uint8_t packetData[MSG_TESTLOOPBACK_RANDOM_DATA_SIZE];  // The payload data (i.e. the actual game data).
} TestLoopBackRequest;

// *** The Loopback response packet.
typedef struct TestLoopBackResponse_ 
{
    uint8_t cmd;  // The command must be the first itrem in all packets. Always MSG_ID_TESTLOOPBACK.
    uint8_t packetSize;  // The size of the payload data
    uint8_t packetData[MSG_TESTLOOPBACK_RANDOM_DATA_SIZE];  // The payload data (i.e. the actual game data).
} TestLoopBackResponse;

extern uint16_t totalSendPacketCount;
extern uint16_t totalReceivedPacketCount;
extern uint16_t sendPacketCountPerSecond;
extern uint16_t recvPacketCountPerSecond;

void NetComInit(void);
uint8_t SendMessage(uint8_t msgId);
uint8_t ReceiveMessage(uint8_t msgId, uint16_t* receivedPacketCount);
uint8_t GetStationIp(/*OUT*/char* text, uint8_t maxTextSizeWithNull );


