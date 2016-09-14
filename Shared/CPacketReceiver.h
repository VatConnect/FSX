//THIS IS A SHARED FILE
//BE CAREFUL ABOUT CHANGING!


//////////////////////////////////////////////////////
//
// This class is used to receive UDP packets (defined in Packets.h) by listening
// on the desired port number. Packets are checked that they are one of the defined
// packets (correct type and size). Multiple instances can exist, listening
// on different ports. The caller must poll for packets with GetNextPacket. Polling
// should be frequent enough the receive buffer doesn't overflow, or packets will
// get dropped. 
//
// Sample use:
//
// Initialize...
// CPacketReceiver Receiver;
// Receiver.Initialize(12345);   //listen on port 12345. 
//
// later..
// 
// static char PacketBuffer[LARGEST_PACKET_SIZE] 
//
// while (Receiver.GetNextPacket(&PacketBuffer))   //could be more than one in the buffer
// {
//		PacketHeader *pHeader = (PacketHeader *)PacketBuffer;
//		if (pHeader->Type == SAMPLE_PACKET)
//		{
//			SamplePacket* pSamplePacket = PacketBuffer;
//          double dLat = pSamplePacket->dLatitude;
//          etc
//		}
//}
//
// Cleanup...
//
// Receiver.Shutdown();
//
//include these in your project if this class isn't compiling (e.g. because you're using "WIN32_LEAN_AND_MEAN" definition) 
//#include <winsock2.h> 
//#include <iphlpapi.h>        

#pragma once

#include <stdio.h>
#pragma comment(lib,"ws2_32")

#include "Packets.h"

#define RECV_BUFFER_SIZE 131072          //Our memory buffer read from the socket. Arbitrary but not too small to drop packets, or too large to waste memory. 
#define SOCKET_RECV_BUFFER_SIZE 131072  //Size we request for the socket's receive buffer, arbitrary but should be large enough for worst case

typedef class CPacketReceiver
{
public:
	CPacketReceiver();
	~CPacketReceiver();

	//Initialize to start listening on the given port. Be sure to call Shutdown when done.
	int Initialize(long PortNumber);

	//Return next packet into the given buffer (must be long enough for largest packet, i.e. LARGEST_PACKET_SIZE in Packets.h)
	//Return 1 if a packet is put into the buffer, 0 if no packets pending (doesn't mean an error, just none in the queue)
	int GetNextPacket(void *pBuffer);

	//Shut down
	int Shutdown();

protected:
	int CloseSocket();

	char	m_RecvBuffer[RECV_BUFFER_SIZE];   //FIFO buffer. When a packet is retrieved, the rest are moved down
	ULONG	m_lNextRecvBufferSpot;            //offset to next open spot in the buffer
	SOCKET	m_Socket;                         
	bool    m_bSocketValid;                   
	bool	m_bWSAStartedUp;                  //Keep track of WSAStartup() calls so we only call it and shut it down once

} CPacketReceiver;






