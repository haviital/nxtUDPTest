#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <arch/zxn.h>
#include <z80.h>

#include "defines.h"
#include "netcom.h"
#include "uart2.h"
#include "TextTileMap.h"

void NetComCheckReceivedLoopbackData(TestLoopBackResponse serverCommandsTestLoopBack);
uint8_t NetComReceiveIPDDataPacket(char* receivedData, uint8_t size);

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

    // Close the previous connection if exists.
    char atcmd[128]; 
    sprintf(atcmd, "AT+CIPCLOSE\r\n");
    uart_tx2(atcmd);
    uint8_t foundStringOrdinal = 0;
    if(uart_read_expected_many2("OK", "ERROR", &foundStringOrdinal) != 0)
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
    // Could also come: "ALREADY CONNECTED\r\nERROR\n\r" => raport an error.
    char atcmd[128]; 
    sprintf(atcmd, "AT+CIPSTART=\"UDP\",\"%s\",%s,%s,0\r\n", 
        serverAddress, serverPort, UDP_LOCAL_PORT );
    uart_tx2(atcmd);
    if(uart_read_expected2("OK") != 0)
        PROG_FAILED;

    // Include sender address in each received packet.
    // AT+CIPDINFO=1
    sprintf(atcmd, "AT+CIPDINFO=1\r\n");
    uart_tx2(atcmd);
    if(uart_read_expected2("OK\r\n") != 0)
        PROG_FAILED;
}

uint8_t GetStationIp(/*OUT*/char* textOut, uint8_t maxTextSizeWithNull )
{
    // Send a command to read my station ip.
    // We should get this back ...'STAIP,"123.456.78.9"<crln>...OK'
    uart_tx2("AT+CIFSR\r\n");

    // Read UART until the string was found or there is timeout.
    uint8_t err = uart_read_expected2("STAIP,\""); 
    if(err)
        return err;  // Note: 1 is a timeout. Not an error.

    // Read the IP adress. The format is '123.456.78.9"'. 
    // Read until '"'. Get the the chars *before* it back, including '\0'.
    err = uart_read_and_get_until_char('"', textOut, maxTextSizeWithNull);
    if(err)
      return 20 + err;

    // Read the rest of the data.
    uart_read_expected2("OK");

    return 0;
}

// Send the packet to the server.
uint8_t NetComPrepareSendMessage(void)
{
    // *** Send the packet to the server via UART & UDP.
    // Init packet sending to the server.
    // Send AT command to UART to start sending the UDP packet.
    char atcmd[32];
    strcpy(atcmd, "AT+CIPSEND=");
    itoa(MSG_TESTLOOPBACK_REQUEST_STRUCT_SIZE, &(atcmd[11]), 10);
    strcat(atcmd, "\r\n");
    uart_tx2(atcmd);     

    return 0;
}

// Send the packet to the server.
uint8_t NetComSendMessage(void)
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

    // Send data to UART. Check the response in the next frame.
    uint8_t err = uart_raw_tx2((uint8_t*)&serverCommandsTestLoopBack, MSG_TESTLOOPBACK_REQUEST_STRUCT_SIZE);
    if(err>0)
        PROG_FAILED1( err+10 );

    totalSendPacketCount++;
    sendPacketCountPerSecond++;
    
    return 0;
}


uint8_t NetComReceiveSendPromptOrDataCmdIfAny(/*OUT*/ uint8_t* netComCmd)
{
    if(uart_available_rx2()) 
    {
        uint8_t foundStringOrdinal = 0;
        uint8_t err = uart_read_expected_many2("+IPD,", ">", /*OUT*/&foundStringOrdinal);
        if(err)
        return err;  // Note: 1 is a timeout. Not an error.    

        if(foundStringOrdinal==0)
            *netComCmd = NETCOM_CMD_IPD_RECEIVED;
        else if(foundStringOrdinal==1)
            *netComCmd = NETCOM_CMD_CIPSEND_RESPONSE;
        else
            PROG_FAILED1(foundStringOrdinal);
    }
    else
    {
        *netComCmd = NETCOM_CMD_NONE;
    }

    return 0;
}

// Receive data from UDP via UART.
uint8_t NetComReceiveSendResponseOrDataPacketIfAny( bool *hasReceivedPacket )
{
    *hasReceivedPacket = false;
    if(uart_available_rx2()) 
    {
        // Read UART until the string was found or there is timeout.
        uint8_t foundStringOrdinal = 0;
        uint8_t err = uart_read_expected_many2("+IPD,", "SEND OK\r\n", /*OUT*/&foundStringOrdinal);
        if(err)
            return err;  // Note: 1 is a timeout. Not an error.    

        if(foundStringOrdinal == 0)
        {
            err = NetComReceiveAndCheckIPDDataPacket();
            if(!err) *hasReceivedPacket = true;
        }
        else if(foundStringOrdinal == 1)
            err = 1; // Timeout (not really but is ok to use)
        return err;  
    }

    return 0;
}

// Receive the packet from the server.
uint8_t NetComReceiveAndCheckIPDDataPacket(void)
{
    /// The IPD cmd received. Ready for receiving the data. 
    TestLoopBackResponse serverCommandsTestLoopBack;
    uint8_t err =  NetComReceiveIPDDataPacket((char*)&serverCommandsTestLoopBack, 
        MSG_TESTLOOPBACK_RESPONSE_STRUCT_SIZE );
    if(err)
        PROG_FAILED;

    NetComCheckReceivedLoopbackData( serverCommandsTestLoopBack );

    return 0;
}

// Receive data from UDP via UART.
uint8_t NetComReceiveIPDDataPacket(char* receivedData, uint8_t size)
{
   // The full received data has the following format:
   // "+IPD," (already read) + "6,192.168.1.100,12345:123456"
   // Where:
   // - 6: the data lenght in bytes
   // - 192.168.1.100: the sender ip address
   // - 12345: the sender ip port
   // - 123456: Actual data

   #ifdef PRINT_UART_RX_DEBUG_TEXT
   uart_debug_print = 1; 
   screencolour = TT_COLOR_PINK;
   TextTileMapGoto(15, 0);
   #endif

   // Read the packet details from UART. The format is: "6,192.168.1.100,12345:" 
   // Get the number of bytes as a string.
   // Read until ':'. Get the amount of data bytes back (as a string).
   //const uint8_t MAX_STR_SIZE = 19; //3+1+15+1+6+1;
   #define MAX_STR_SIZE 27//19
   char receivedFromUart[MAX_STR_SIZE]; // includes the ending null. 
   uint8_t err = uart_read_and_get_until_char(':', receivedFromUart, MAX_STR_SIZE);
   if(err)
      return 20 + err;

   #ifdef PRINT_UART_RX_DEBUG_TEXT
   uart_debug_print = 0;  
   #endif

   //#ifdef PRINT_UART_RX_DEBUG_TEXT
   //uart_pretty_print(ch);
   //#endif

   // Check the minimum lenght.
   uint16_t serverAddressLen = strlen(serverAddress);
   uint16_t serverPortLen = strlen(serverPort);
   uint16_t len = strlen(receivedFromUart);
   if( len < 1 + 1 + serverAddressLen + 1 + serverPortLen + 1 )
      PROG_FAILED;

   // get data lenght.
   char tmp[32];
   char* posPtr = strchr(receivedFromUart, ',');
   if(posPtr==NULL)  
      PROG_FAILED;
   len = posPtr-receivedFromUart;
   strncpy(tmp, receivedFromUart,len);
   tmp[len] = '\0';

   // Convert the string to a number.
   uint8_t dataLen = atoi(tmp);
   if(dataLen!=size)
      PROG_FAILED1(dataLen);

   // Check ip address.
   posPtr++; // ','
   if( strncmp(posPtr, serverAddress, serverAddressLen ) != 0)
      PROG_FAILED;
   posPtr += serverAddressLen;

   // Check the port.
   posPtr++; // ','
   if( strncmp(posPtr, serverPort, serverPortLen ) != 0)
      PROG_FAILED;
      
   // Read actual data.
   for(uint8_t i=0; i<dataLen; i++)
      receivedData[i] = uart_rx_char2();

   return 0;
}

void NetComCheckReceivedLoopbackData(TestLoopBackResponse serverCommandsTestLoopBack)
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
        {
            //CSPECT_BREAK();
            PROG_FAILED1(i);
        }

    // All good. Increase the received packet count.
    //CSPECT_BREAK();                    
    totalReceivedPacketCount++;
    recvPacketCountPerSecond++;
    zx_border(INK_YELLOW);

    lastReceivedPacketAt = frames16t;
}