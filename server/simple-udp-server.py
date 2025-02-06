##############################
# This is a simple loopback UDP server which receives a packet and sends it
# back to the client. The client specify in a packet header how many repeated sends there are.  
# Hannu Viitala, 2025

import socket
from socket import getaddrinfo, gethostname
import ipaddress
import argparse
import time
import os
import re
import uuid

##############################
#
def get_ip():
    ip_list = []
    for ip in getaddrinfo(gethostname(), None, 2, 1, 0):
        try:
            if len( ip ) == 5:
                ipa = ip[4][0]
                ipao = ipaddress.ip_address( ipa )
                if not( ipao.is_loopback or ipao.is_link_local ):
                    ip_list.append( str( ipa ) )
        except:
            pass
    return ip_list

##############################
#
def receive_udp_packet(sock, testLoopBackGuidBytes, clientIp):
    recvData, address = sock.recvfrom(4096)
    if(clientIp != None and address[0]!=clientIp ):
        print(f"*** ERROR! Not authorized client ip tries to connect: {address}")
        cmd = b'\xff'  # error
        return cmd, cmd, cmd, cmd, cmd

    print(f"Received {len(recvData)} bytes from {address}")
    recvDataList = list(recvData)
    # print(f"bytes array: {recvDataList}")

    # Handle the command from the client.
    
    # This is the C struct the client sends:
    # {
    #     uint8_t cmd;
    #     uint8_t loopPacketCount;
    #     uint8_t packetSize;
    #     uint8_t packetData[MSG_TESTLOOPBACK_RANDOM_DATA_SIZE];
    # } 

    cmd = recvData[0:1]
    loopPacketCount = recvData[1:2]
    packetSize = recvData[2:3]
    packetData = recvData[3:]

    #len_ = len(packetData)
    #print(f"cmd={cmd}, loopPacketCount={loopPacketCount}, packetSize={packetSize}, data size={len_}, address={address}")

    return cmd, loopPacketCount, packetSize, packetData, address

##############################
#
def send_udp_packets( sock, address, cmd, packetSize, packetData, loopPacketCount ):

    # This is the C struct the client expects to receive:
    # {
    #     uint8_t cmd;
    #     uint8_t packetSize;
    #     uint8_t packetData[MSG_TESTLOOPBACK_RANDOM_DATA_SIZE];
    # } 

    # print(f"send_udp_packets")

    sendDataBytes = cmd + packetSize + packetData

    count = int.from_bytes(loopPacketCount, byteorder='big')
    #print(f"loopback count={count}")
    sendDataList = list(sendDataBytes)
    sendDataLen = len(sendDataBytes)
    #print(f"bytes array: size={sendDataLen}, data={sendDataList}")
    for i in range(0, count):
        sent = sock.sendto(sendDataBytes, address)
        print(f"Sent {sent} bytes back to {address}")

##############################
#
def udp_server(host, port, clientIp):
    
    # Create a UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_address = (host, port)
    sock.bind(server_address)

    if(clientIp == None):
        print(f"UDP server listening. Accepts connections from any ip address to port: {port}.")
    else:
        print(f"UDP server listening. Accepts connections only from {clientIp} to port: {port}.")

    # The test guid for security check. 
    guid_str = 'b679c980-0a2f-4c71-a0cf-fe9dcfef3a17'
    guid = uuid.UUID(guid_str)
    testLoopBackGuidBytes = guid.bytes

    while True:

        # Receive a data packet.
        cmd, loopPacketCount, packetSize, packetData, address = receive_udp_packet(
            sock, testLoopBackGuidBytes, clientIp )
        if cmd != b'\x12':  # MSG_ID_TESTLOOPBACK = 18
            exit

        # Send data packets
        send_udp_packets( sock, address, cmd, packetSize, packetData, loopPacketCount )

##############################
#
if __name__ == "__main__":
    
    # Read parameters
    parser = argparse.ArgumentParser(
        prog='simple-udp-server.py',
        description='This is a simple UDP server which receives a packet and sends one or more packets back to the client. The client specify in a packet header how many repeated sends there are.',
        epilog='Example: python simple-udp-server.py -c 123.456.78.9'
    )
    parser.add_argument(
            '-c','--clientip', action='store', dest='clientip',
            help="The address of the client (optional). The sender of the incoming packet will be checked." )
    args = parser.parse_args()

    if(args.clientip == None):
        print("\n*** Note ***")
        print("The -clientip parameter not used. The incoming packet ip is not checked.\nUse -clientip or -c for better security. More info with the -h parameter.")
        print("*** End  ***\n")
       
    serverPort = 4444
    ips = get_ip()
    for serverIp in sorted( ips ):
        print( f"Server: {serverIp}:{serverPort}" )

    # Start the UDP server
    udp_server(serverIp, serverPort, args.clientip)
