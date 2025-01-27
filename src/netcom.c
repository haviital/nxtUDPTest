#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <arch/zxn.h>
#include <z80.h>

#include "defines.h"
#include "netcom.h"
#include "uart2.h"
#include "TextTileMap.h"

// Init the connection to the server.
void NetComInit(void)
{
    DrawStatusTextAndPageFlip("Init");
    uart_flush_rx2();

    DrawStatusTextAndPageFlip("Connecting to Wifi");

    // Disable echo
    #ifdef AT_ECHO_ON
    uart_tx2("ATE1\r\n");
    #else
    uart_tx2("ATE0\r\n");
    #endif
    if(uart_read_expected2("OK") != 0)
        PROG_FAILED;

    #ifdef RESET_WIFI
    // Initialize the wifi module.
    // sprintf(atcmd, "AT+CWINIT=1\r\n");
    // uart_tx2(atcmd);
    // if(uart_read_expected2("OK") != 0)
    //     PROG_FAILED;
    #endif

    // Enable single connection mode
    DrawStatusTextAndPageFlip("Init connection.");
    uart_tx2("AT+CIPMUX=0\r\n");
    if(uart_read_expected2("OK") != 0)
        PROG_FAILED;
   
    #ifdef PRINT_UART_RX_DEBUG_TEXT
    uart_debug_print = 1;
    #endif
    // Get my IP address.
    uint8_t err = GetStationIp(/*OUT*/localIpAddress, 16 );
    if(err)
        PROG_FAILED1(err);
    #ifdef PRINT_UART_RX_DEBUG_TEXT
    uart_debug_print = 0;
    #endif

    DrawStatusTextAndPageFlip("");
 }

// Init the connection to the server.
void NetComConnectToServer(void)
{
    // DrawStatusTextAndPageFlip("Connect to server.");

    // Start UDP connection.
    // 2 means UDP(?)
    // 0 means wifi passthrough
    // AT+CIPSTART="UDP","<remote_ip>",<remote_port>,<local_port>,0
    // Could also come: "ALREADY CONNECTED\r\nERROR\n\r"
    char atcmd[128]; 
    sprintf(atcmd, "AT+CIPSTART=\"UDP\",\"%s\",%s,%s,0\r\n", 
        serverAddress, serverPort, UDP_LOCAL_PORT );
    uart_tx2(atcmd);
    if(uart_read_expected_many2("OK", "ERROR") != 0)
        PROG_FAILED;

    // Include sender address in each received packet.
    // AT+CIPDINFO=1
    sprintf(atcmd, "AT+CIPDINFO=1\r\n");
    uart_tx2(atcmd);
    if(uart_read_expected_many2("OK", "ERROR") != 0)
        PROG_FAILED;
}

uint8_t GetStationIp(/*OUT*/char* textOut, uint8_t maxTextSizeWithNull )
{
    // Send a command to read my station ip.
    // We should get this back ...'STAIP,"123.456.78.9"<crln>...OK'
    uart_tx2("AT+CIFSR\r\n");

    // Read UART until the string was found or there is timeout.
    uint8_t err = uart_read_until_expected("STAIP,\""); 
    if(err)
        return err;  // Note: 1 is a timeout. Not an error.

    // Read the IP adress. The format is '123.456.78.9"'. 
    // Read until '"'. Get the the chars *before* it back, including '\0'.
    err = uart_read_and_get_until_char('"', textOut, maxTextSizeWithNull);
    if(err)
      return 20 + err;

    // Read the rest of the data.
    uart_read_expected_many2("OK", "ERROR");

    return 0;
}

// Receive the packet from the server.
uint8_t ReceiveMessage(uint8_t msgId, uint16_t* receivedPacketCount)
{
    if(uart_available_rx2()) 
    {
        switch(msgId)
        {
            case MSG_ID_NOP:
            {
                // Receive UDP data if exists.
                NopResponse resp;
                uint8_t err = uart_receive_data_packet_if_any((char*)&resp, sizeof(NopResponse));
                if(err && err != 1) PROG_FAILED;  // Note: 1 is timeout. Not an error.

                // If the packet was found, check the result.
                if(!err)
                {
                    if(resp.cmd != 0 )
                        PROG_FAILED;
                    if( resp.flags != 1 )
                        PROG_FAILED;
                }

                // All good. Increase the received packet count.
                (*receivedPacketCount)++;
                totalReceivedPacketCount++;
                recvPacketCountPerSecond++;
            }
            break;
            
            case MSG_ID_TESTLOOPBACK:
            {
                // The request struct.

                //CSPECT_BREAK()

                // Receive a packet from the server
                TestLoopBackResponse serverCommandsTestLoopBack;
                uint8_t err = uart_receive_data_packet_if_any((char*)&serverCommandsTestLoopBack, 
                    MSG_TESTLOOPBACK_RESPONSE_STRUCT_SIZE);
                if(err && err != 1) PROG_FAILED1(err);  // Note: 1 is timeout. Not an error.

                // If the packet was found, check the result.
                if(!err)
                {

                    //CSPECT_BREAK();
                    zx_border(INK_GREEN);

                    if( serverCommandsTestLoopBack.cmd != MSG_ID_TESTLOOPBACK )
                        PROG_FAILED;
                    if( serverCommandsTestLoopBack.packetSize != MSG_TESTLOOPBACK_RANDOM_DATA_SIZE )
                    {
                        printf("packetSize=%u (%u)", serverCommandsTestLoopBack.packetSize, 
                            MSG_TESTLOOPBACK_RESPONSE_STRUCT_SIZE);
                        PROG_FAILED;
                    }

                    // Verify the test data.
                    for(uint8_t i=0; i<MSG_TESTLOOPBACK_RANDOM_DATA_SIZE; i++)
                        if(serverCommandsTestLoopBack.packetData[i] != i)
                        {
                            //CSPECT_BREAK();
                            PROG_FAILED1(i);
                        }

                    // All good. Increase the received packet count.
                    //CSPECT_BREAK();                    
                    (*receivedPacketCount)++;
                    totalReceivedPacketCount++;
                    recvPacketCountPerSecond++;
                    zx_border(INK_YELLOW);
                }
            }
            break;
            
            default:
                PROG_FAILED;
        }
    }

    return 0;
}

// Send the packet to the server.
uint8_t SendMessage(uint8_t msgId)
{
    // *** Send the packet to the server via UART & UDP.
    switch(msgId)
    {
        case MSG_ID_NOP:
        {
            // Send NOP to the server.
            uint8_t serverCommandsNop = 0;
            uint8_t packetLen = 1;
            uint8_t err = uart_send_data_packet2(&serverCommandsNop, packetLen);
            if(err) PROG_FAILED1(err);
        }
        break;
        
        case MSG_ID_TESTLOOPBACK:
        {
            // The request struct.
            TestLoopBackRequest serverCommandsTestLoopBack = 
            {
                .cmd = MSG_ID_TESTLOOPBACK,
                .loopPacketCount = numClonedPackets,  // Count of faked "clients"
                .packetSize = MSG_TESTLOOPBACK_RANDOM_DATA_SIZE,  // Random data size.                  
            };

            // Init the test data for verification when it comes back
            for(uint8_t i=0; i<MSG_TESTLOOPBACK_RANDOM_DATA_SIZE; i++)
                serverCommandsTestLoopBack.packetData[i] = i;

            // Send packet to the server. The server sends it back 3 times.
            uint8_t err = uart_send_data_packet2((uint8_t*)&serverCommandsTestLoopBack, 
                MSG_TESTLOOPBACK_REQUEST_STRUCT_SIZE);
            if(err) PROG_FAILED1(err);
        }
        break;
        
        default:
            PROG_FAILED;
    }

    // Read send response (from UART?).
    // The response should be: "Recv 1 bytes\n\rSEND OK\n\r"
    // TODO: Can "+IPD" interfere before SEND OK?
    // TODO: Is this slow to wait for the answer.
    uint8_t err = uart_read_expected2("SEND OK");
    if(err) PROG_FAILED; 

    totalSendPacketCount++;
    sendPacketCountPerSecond++;
    
    return 0;
}