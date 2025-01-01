#include <stdint.h>
#include "defines.h"

#define MSG_ID_NOP 0
typedef struct NopResponse_ 
{
    uint8_t cmd;
    uint8_t flags;
} NopResponse;



#define MSG_ID_TESTLOOPBACK 18
#define MSG_TESTLOOPBACK_RANDOM_DATA_SIZE 16
#define MSG_TESTLOOPBACK_REQUEST_STRUCT_SIZE 19+MSG_TESTLOOPBACK_RANDOM_DATA_SIZE
#define MSG_TESTLOOPBACK_RESPONSE_STRUCT_SIZE 2+MSG_TESTLOOPBACK_RANDOM_DATA_SIZE
typedef struct TestLoopBackRequest_ 
{
    uint8_t cmd;
    uint8_t testGuid[16];
    uint8_t loopPacketCount;
    uint8_t packetSize;
    uint8_t packetData[MSG_TESTLOOPBACK_RANDOM_DATA_SIZE];
} TestLoopBackRequest;

typedef struct TestLoopBackResponse_ 
{
    uint8_t cmd;
    uint8_t packetSize;
    uint8_t packetData[MSG_TESTLOOPBACK_RANDOM_DATA_SIZE];
} TestLoopBackResponse;

extern char g_wifiSsid[64];
extern char g_wifiPassword[128];

extern uint16_t totalSendPacketCount;
extern uint16_t totalReceivedPacketCount;
extern uint16_t sendPacketCountPerSecond;
extern uint16_t recvPacketCountPerSecond;

void NetComInit(void);
//uint8_t SendOrReceiveData(uint8_t msgId, uint16_t* receivedPacketCount);
uint8_t SendMessage(uint8_t msgId);
uint8_t ReceiveMessage(uint8_t msgId, uint16_t* receivedPacketCount);


