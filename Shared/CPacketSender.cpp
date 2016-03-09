//THIS IS A SHARED FILE
//BE CAREFUL ABOUT CHANGING!


#include "stdafx.h"
#include "CPacketSender.h"

CPacketSender::CPacketSender() : m_Socket(NULL), m_bSocketValid(false), m_bWSAStartedUp(false)
{
}

CPacketSender::~CPacketSender()
{
	//Make sure we've shut down (won't hurt to do it again even if already done)
	Shutdown();
}

//Initialize to send packets to given port (Loopback by default). 1 if success, 0 if fail.
//To re-initialize to a different destination, caller needs to first call Shutdown().
int CPacketSender::Initialize(long DestPortNum, const char *pDestIP) 
{
	//Already initialized?
	if (m_bSocketValid)
		return 0;

	//Initialize WSA startup if it wasn't already. Calls to WSACleanup needs to be matched to each WSAStartup  
	//so we make sure to only do it once.
	if (!m_bWSAStartedUp)
	{
		WORD versionRequested= MAKEWORD( 2, 0 ); 
		WSADATA wsaData;
		if (WSAStartup( versionRequested, &wsaData ) != 0)
		{
			//Log("WSAStartup failed, unable to create net connections -- check app permissions?\n");
			return 0;
		}
		m_bWSAStartedUp = true;
	}

	//Create an outgoing socket
	m_Socket = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( m_Socket == SOCKET_ERROR ) 
	{
		//Log("Failed creating send socket\n");
		return 0;
	}

	//Permit multicasts
	BOOL value= TRUE;
	if (setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST, (const char*) &value, sizeof(BOOL)) < 0) 
	{
		//Log("Failed setting socket option to broadcast\n");
		//non-fatal, continue
	}
	
	//Set to non-blocking
	u_long DontBlock = true;
	if(ioctlsocket(m_Socket, FIONBIO, &DontBlock) != 0) 
	{
		//Log("Failed setting socket to non-blocking\n");

		//This doesn't have to be fatal, but it could make
		//Send() hang for awhile, and callers expect it 
		//to return right away. So we consider it fatal. 
		CloseSocket();
		return 0;
	}

	//Set default destination address to given IP
	SOCKADDR_IN	socketAddr;
	socketAddr.sin_family = AF_INET;
	socketAddr.sin_port = htons((unsigned short)DestPortNum);
	socketAddr.sin_addr.s_addr = inet_addr(pDestIP); 
	if (connect( m_Socket, (SOCKADDR*) &socketAddr, sizeof(socketAddr)) < 0) 
	{
		//int error= WSAGetLastError();
		//Log("Could not connect outgoing socket to port: %d - error: %d\n", DestPortNum, error );
		CloseSocket();
		return 0;
	} 

	m_bSocketValid = true;
	
	return 1;
}

//Send packet from Packets.h 
int CPacketSender::Send(void *pPacket)
{
	//Make sure we've been initialized, and the packet is valid
	if (!m_bSocketValid || ((PacketHeader *)pPacket)->MagicNumber != PACKET_MAGIC_NUM)
		return 0;
	
	int Sent = send(m_Socket, (char *)pPacket, (int)((PacketHeader *)pPacket)->Len, 0);
	if (Sent == SOCKET_ERROR)
	{
		int ErrNo = WSAGetLastError();

		//Error 10035 (WSAWouldBlock) isn't really an error, just that the system couldn't send it
		//immediately (but will later, typically fractions of milliseconds). 
		if (ErrNo == 10035) 
			return 1;  

		//if (ErrNo == WSAENOTCONN)
		//  Log("Lost connection")
		//else
		//	Log("WSAGetLastError = %i", ErrNO);
		return 0;
	}
	else if (Sent < (int)((PacketHeader*)pPacket)->Len)   
	{
		//Log("Socket output buffer full??");
		return 0;
	}
	return 1;
}

//Cleanly shutdown
int CPacketSender::Shutdown()
{
	CloseSocket();

	if (m_bWSAStartedUp)
	{
		WSACleanup();
		m_bWSAStartedUp = false;
	}

	return 1;
}
///////////////////
//Protected
int CPacketSender::CloseSocket()
{
	if (m_Socket)
	{
		shutdown(m_Socket, 1);
		closesocket(m_Socket);
		m_Socket = NULL;
		m_bSocketValid = false;
	}
	return 1;
}
