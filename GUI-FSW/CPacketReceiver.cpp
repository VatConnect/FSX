//THIS IS A SHARED FILE
//BE CAREFUL ABOUT CHANGING!


#include "stdafx.h"

#include "CPacketReceiver.h"

CPacketReceiver::CPacketReceiver() : m_lNextRecvBufferSpot(0), m_Socket(NULL), m_bSocketValid(false), m_bWSAStartedUp(false)
{
}

CPacketReceiver::~CPacketReceiver()
{
	//Make sure to shut down (won't hurt to do it again even if already done)
	Shutdown();
}

//Initialize to listen for packets on given port. 1 if success, 0 if fail.
int CPacketReceiver::Initialize(long PortNum) 
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
	
	//Create socket
	m_Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_Socket == SOCKET_ERROR)
	{
		//Log("Failed creating receive socket!\n");
		return 0;
	}

	BOOL value = TRUE;
	if (setsockopt(m_Socket, SOL_SOCKET, SO_DONTLINGER, (const char*)&value, sizeof(BOOL)) < 0)
	{
		//Log(L"Failed setting socket option to DONTLINGER\n");
	}

	int iRecvBufSize = SOCKET_RECV_BUFFER_SIZE;
	if (setsockopt(m_Socket, SOL_SOCKET, SO_RCVBUF, (char *)&iRecvBufSize, sizeof(int)) < 0)
	{
		//Log(L"Failed to set receive buffer size");
	}

	//Set to non-blocking (otherwise we'd hang waiting for the next packet) 
	u_long DontBlock = true;
	if(ioctlsocket(m_Socket, FIONBIO, &DontBlock ) != 0 ) 
	{
		CloseSocket();
		//Log("Failed setting receive socket to non-blocking\n");
		return 0;
	}

	//bind it to the port
	int rc;
	struct sockaddr_in serveraddr;
	UINT16 serveraddrlen = sizeof(serveraddr);
	memset(&serveraddr, 0x00, serveraddrlen);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons((unsigned short)PortNum);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	rc = bind(m_Socket, (struct sockaddr *)&serveraddr, serveraddrlen); 
	if (rc < 0) 
	{
		//Log("Couldn't bind to UDP receive port");
		CloseSocket();
		return 0;
	}
	m_bSocketValid = true;
	
	return 1;
}

//Cleanly shutdown
int CPacketReceiver::Shutdown()
{
	CloseSocket();

	if (m_bWSAStartedUp)
	{
		WSACleanup();
		m_bWSAStartedUp = false;
	}

	return 1;
}

//Return the next packet in the buffer, if any. Returns 1 if a packet was copied into caller's
//buffer, 0 if no packets pending. A packet is validated to be one of the defined packets
//in Packets.h (the type is defined, and the magic number in the header is correct) 
int	CPacketReceiver::GetNextPacket(void *pBuffer)
{
	if (!m_bSocketValid)
		return 0;

	//Copy anything pending in the network buffer into ours
	int BytesReceived = recv(m_Socket, &m_RecvBuffer[m_lNextRecvBufferSpot], (RECV_BUFFER_SIZE - (int)m_lNextRecvBufferSpot), 0); 
	if (BytesReceived == SOCKET_ERROR) 
	{
		int ErrNo = WSAGetLastError();
		if (ErrNo != WSAEWOULDBLOCK)
		{
			//Log("Error receiving packets: WSAGetLastError# %i", ErrNo);
			return 0;
		}
		//If "would block" but nothing in buffer, return
		else if (m_lNextRecvBufferSpot == 0)
			return 0;

		//Nothing received but we have still stuff in the buffer... fall through
		BytesReceived = 0;
	}
	
	//Update next open spot in the buffer
	m_lNextRecvBufferSpot += BytesReceived;

	//Make sure it's a packet from Packets.h
	if (m_lNextRecvBufferSpot < sizeof(PacketHeader)) 
	    return 0;
	PacketHeader *pHeader = (PacketHeader *)(&m_RecvBuffer[0]);
	if (pHeader->MagicNumber != PACKET_MAGIC_NUM || pHeader->Len > LARGEST_PACKET_SIZE)
	{
		//Somebody sent us garbage data. This could theoretically happen if some other app is receiving data on
		//our port and by coincidence we connected before they did (??) but more likely it's bad code on the 
		//sender's side (maybe compiled with an older version of Packets.h?) We don't know where the garbage 
		//ends and any legit packets begin so throw it all out.
		m_lNextRecvBufferSpot = 0;
		OutputDebugString(L"Garbage received, discarded\n!");
		return 0;
	}
	
	//Note we do allow through undefined packet types (pHeader->Type >= ID_MAX_PACKET) and it's up to the caller
	//to ignore it. This is so older versions of code can handle newer versions cleanly (by just ignoring it instead
	//of us throwing out potentially good packets after it).

	//Next make sure we have the full packet
	if (m_lNextRecvBufferSpot < pHeader->Len)
		return 0;

	//Copy it into caller's buffer and move rest of our receive buffer down. 
	memcpy(pBuffer, &m_RecvBuffer[0], pHeader->Len);
	if (m_lNextRecvBufferSpot > pHeader->Len)
	{
		ULONG Length = pHeader->Len;
		memmove(&m_RecvBuffer[0], &m_RecvBuffer[Length], (size_t)(m_lNextRecvBufferSpot - Length));
		m_lNextRecvBufferSpot -= Length;
	}
	else
		m_lNextRecvBufferSpot = 0;

	return 1;
}



///////////////////
//Protected
int CPacketReceiver::CloseSocket()
{
	if (m_Socket)
	{
		shutdown(m_Socket, 0);
		closesocket(m_Socket);
		m_Socket = NULL;
		m_bSocketValid = false;
	}
	return 1;
}
