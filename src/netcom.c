#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <z80.h>

#include "defines.h"
#include "netcom.h"
#include "uart2.h"

void DrawStatusTextAndPageFlip(char* text);

void NetComInit(void)
{
    //printf("NetComInit()..\n"); //!!HV
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

    // Connect to wifi AP.
    char atcmd[256];
    sprintf(atcmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", g_wifiSsid, g_wifiPassword);
    uart_tx2(atcmd);
    if(uart_read_expected2("OK") != 0)
        PROG_FAILED;

    DrawStatusTextAndPageFlip("Connected to Wifi");

    // Enable single connection mode
    //AT+CIPMUX=0
    uart_tx2("AT+CIPMUX=0\r\n");
    if(uart_read_expected2("OK") != 0)
        PROG_FAILED;
   
    DrawStatusTextAndPageFlip("Create Server connection");
    // Start UDP connection.
    // 2 means UDP(?)
    // 0 means wifi passthrough
    // AT+CIPSTART="UDP","<remote_ip>",<remote_port>,<local_port>,0
    // Could also come: "ALREADY CONNECTED\r\nERROR\n\r"
    sprintf(atcmd, "AT+CIPSTART=\"UDP\",\"%s\",%s,%s,0\r\n", 
        UDP_SERVER_ADDRESS, UDP_SERVER_PORT,UDP_LOCAL_PORT );
    uart_tx2(atcmd);
    if(uart_read_expected_many2("OK", "ERROR") != 0)
        PROG_FAILED;

    DrawStatusTextAndPageFlip("");

    //printf("..NetComInit()\n"); //!!HV
 }

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

                // Receive a packet from the server
                TestLoopBackResponse serverCommandsTestLoopBack;
                uint8_t err = uart_receive_data_packet_if_any((char*)&serverCommandsTestLoopBack, 
                    MSG_TESTLOOPBACK_RESPONSE_STRUCT_SIZE);
                if(err && err != 1) PROG_FAILED;  // Note: 1 is timeout. Not an error.

                // If the packet was found, check the result.
                if(!err)
                {
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
                            PROG_FAILED1(i);

                    // All good. Increase the received packet count.
                    (*receivedPacketCount)++;
                    totalReceivedPacketCount++;
                    recvPacketCountPerSecond++;
                }
            }
            break;
            
            default:
                PROG_FAILED;
        }

        //z80_delay_ms(1*8);   // 8x for 28MHz
    }

    return 0;
}

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
                // Server TESTLOOPBACKGUID: b679c980-0a2f-4c71-a0cf-fe9dcfef3a17
                .testGuid = 
                {
                    0xb6, 0x79, 0xc9, 0x80, 0x0a, 0x2f, 0x4c, 0x71, 
                    0xa0, 0xcf, 0xfe, 0x9d, 0xcf, 0xef, 0x3a, 0x17
                },
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

    //int32_t delta = totalSendPacketCount - totalReceivedPacketCount; 

    return 0;
}