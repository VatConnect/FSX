//THIS IS A SHARED FILE
//BE CAREFUL ABOUT CHANGING!


//////////////////////////////////////////////////////
//
// This class is used to send a UDP packet defined in Packets.h to anybody
// listening on the desired port number (and optional different IP#, but the intent
// of this is for inter-process communication on the same machine). Multiple instances
// can exist. Note packets are sent out blindly -- the receiver may or may not be connected,
// and if sent over a WAN it's possible they could be dropped before reaching the receiver.
//
//
// Sample use:
//
// Initialize...
// CPacketSender Sender;
// Sender.Initialize(12345); //set up to send to anybody listening on port 12345 on this machine
//
// later..
// 
// SamplePacket P;
// P.dLatitude = 42.023932945
// P.dLongitude = -117.123432
// Sender.Send(P);
//
// Cleanup...
//
// Sender.Shutdown();
//

#pragma once

#include <stdio.h>   
#pragma comment(lib,"ws2_32")

#include "Packets.h"

#define SOCKET_SEND_BUFFER_SIZE 65535

typedef class CPacketSender
{
public:
	CPacketSender();
	~CPacketSender();

	//Initialize to send to given port (loopback by default, but another IP can be used). 
	//1 if success, 0 fail. 
	int Initialize(long SendToPortNumber, const char *pDestIP="localhost");
	
	//Send a packet defined in Packets.h. 1 success, 0 fail. 
	int Send(void *pPacket);
	
	//Shut down connection. 1 success, 0 fail
	int Shutdown();

protected:
	int CloseSocket();

	SOCKET	m_Socket;                         //The socket we send and receive on
	bool    m_bSocketValid;                   //Socket is usable (always for UDP, only once connected with TCP)
	bool	m_bWSAStartedUp;                  //Remembered so we don't call WSAStartup more than once
	
} CPacketSender;









