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
def receive_udp_packet(sock, testLoopBackGuidBytes):
    recvData, address = sock.recvfrom(4096)
    print(f"Received {len(recvData)} bytes from {address}")
    recvDataList = list(recvData)
    # print(f"bytes array: {recvDataList}")

    # Handle the command from the client.
    
    # This is the C struct the client sends:
    # {
    #     uint8_t cmd;
    #     uint8_t testGuid[16];
    #     uint8_t loopPacketCount;
    #     uint8_t packetSize;
    #     uint8_t packetData[MSG_TESTLOOPBACK_RANDOM_DATA_SIZE];
    # } 

    cmd = recvData[0:1]
    clientGuidBytes = recvData[1:17]  # [17] is excluded     
    loopPacketCount = recvData[17:18]
    packetSize = recvData[18:19]
    packetData = recvData[19:]

    # Validate the client packet.
    #print(list(testLoopBackGuidBytes))
    #print(list(clientGuidBytes))
    if(clientGuidBytes != testLoopBackGuidBytes):
        print("Invalid client guid or corrupted packet")
        cmd = b'\xff'
    len_ = len(packetData)
    #print(f"cmd={cmd}, loopPacketCount={loopPacketCount}, packetSize={packetSize}, data size={len_}, address={address}")

    return cmd, clientGuidBytes, loopPacketCount, packetSize, packetData, address

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
def udp_server(host, port):
    
    # Create a UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_address = (host, port)
    sock.bind(server_address)

    print(f"UDP server listening")

    # The test guid for security check. 
    guid_str = 'b679c980-0a2f-4c71-a0cf-fe9dcfef3a17'
    guid = uuid.UUID(guid_str)
    testLoopBackGuidBytes = guid.bytes

    while True:

        # Receive a data packet.
        cmd, clientGuidBytes, loopPacketCount, packetSize, packetData, address = receive_udp_packet(
            sock, testLoopBackGuidBytes )
        if cmd == b'\xff':
            exit

        # Send data packets
        send_udp_packets( sock, address, cmd, packetSize, packetData, loopPacketCount )

##############################
#
if __name__ == "__main__":
    
    # Read parameters
    parser = argparse.ArgumentParser(
        description='Simple UDP server.' )
    parser.add_argument(
            '-b','--bind', action='store', dest='bind',
            default='0.0.0.0', help="bind address" )
    parser.add_argument(
            '-p', '--port', action='store', dest='port',
            default=4444, help="listening port" )
    args = parser.parse_args()

    ips = get_ip()
    for s in sorted( ips ):
        print( f"#  {s}:{args.port}" )

    # Start the UDP server
    udp_server(args.bind, args.port)
