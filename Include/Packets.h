//THIS IS A SHARED FILE
//BE CAREFUL WITH MODIFICATIONS!
//
#pragma once

//What ports the server interface and flight sim addon are listening on
#define SERVER_PROXY_LISTEN_PORT  15200
#define CLIENT_LISTEN_PORT  15201

//Make structures byte aligned -- this is important so receivers can rely on byte offsets to get the data, 
//if they can't cast for some reason.  
#pragma pack (push)
#pragma pack (1)

//This is in every packet header and can be used as a basic sanity check that it's likely a valid packet,  
//especially in conjunction with a valid packet type and the expected length for that packet type. It can 
//be any unique 32 bit integer. Changing this will break backward compatibility.
#define PACKET_MAGIC_NUM 234567         

//This can be used by receivers to know how much buffer space to allocate. It can be larger than the largest 
//packet, just make sure it's not smaller.
#define LARGEST_PACKET_SIZE 4096

//Packet identifier, so receiver knows which struct to cast it back into. Note it's possible to receive a 
//packet type that is >= MAX_PACKET_ID, (newer code), and the receiver should just ignore it. 
typedef enum ePacketType
{
	//Messages to server
	REQ_CONNECTION_PACKET,
	USER_STATE_UPDATE_PACKET,
	TRANSMIT_KEYDOWN_PACKET,
	TRANSMIT_KEYUP_PACKET,
	REQ_DISCONNECT_PACKET,
	SHUTDOWN_PACKET,

	//Messages from server
	PROXY_READY_PACKET,
	CONNECT_SUCCESS_PACKET,
	CONNECT_FAIL_PACKET,
	ADD_OBJ_PACKET,
	REMOVE_OBJ_PACKET,
	UPDATE_OBJ_PACKET,
	REQ_USER_STATE_PACKET,
	LOGOFF_SUCCESS_PACKET,
	LOST_CONNECTION_PACKET,
	LOGIN_INFO_PACKET,


	MAX_PACKET_ID
} ePacketType;

typedef class PacketHeader 
{
	public:
	unsigned long		Len;			//The length of the total packet in bytes, including this header
	unsigned long		MagicNumber;	//constant to confirm it's a valid packet 
	ePacketType		    Type;			
} PacketHeader;

//The header all packets need to be derived from. The constructor initializes everything.
template<class PacketStructName, ePacketType PacketType>
struct PacketInit : public PacketHeader
{
	PacketInit()
	{
		MagicNumber = PACKET_MAGIC_NUM;
		Len = sizeof(PacketStructName);
		Type = PacketType;
	}
};

////////////////////////////////////////////////////////////////////////
//
//
// Packet definitions
//
//
//

//To add new packet types, add an ID to the bottom of the ePacketType enums, above MAX_PACKET_ID,
//and define it below using the others as a guide. (Don't use "int", use "long" or DWORD instead because
//size of int could be different on sender and receiver).
//
//Make sure to update LARGEST_PACKET_SIZE above if packet is larger
//
//Note string lengths are hardcoded -- packet sender responsible to ensure no overflow, so be careful about
//making lengths smaller because senders' code will have to be changed too. 

/////////////////////////////
//Messages sent to server interface

//User requesting connection to server. 
typedef struct ReqConnectPacket : public PacketInit<ReqConnectPacket, REQ_CONNECTION_PACKET>
{
	char szServerName[64];
	char szUserName[64];
	char szUserID[32];
	char szPassword[32];
	DWORD bIsObserver;      //True if observer (and callsign and ACType not filled in), false if pilot
	char szCallsign[16];
	char szACType[16];
} ReqConnectPacket;

//User requests disconnection from the server
typedef struct ReqDisconnectPacket : public PacketInit<ReqDisconnectPacket, REQ_DISCONNECT_PACKET>
{
	DWORD Unused;
} ReqDisconnectPacket;

//Latest user state, sent in response to ReqUserStatePacket. Booleans 1 if true, 0 if false.
typedef struct UserStateUpdatePacket : public PacketInit<UserStateUpdatePacket, USER_STATE_UPDATE_PACKET>
{
	double UserTimeSecs;
	double LatDegN;
	double LonDegE;
	double AltFtMSL;
	double PitchDegUp;              
	double HdgDegTrue;
	double RollDegRight;            //positive going right looking out from object
	double GroundSpeedKts;
	DWORD  bStrobeLightsOn;
	DWORD  bLandingLightsOn;
	DWORD  bTaxiLightsOn;
	DWORD  bBeaconLightsOn;
	DWORD  bNavLightsOn;
	DWORD  bLogoLightsOn;
	DWORD  bGearDown;               //Really means position of gear handle
	DWORD  bOnGround;               //True if object says it's on the ground (according to the object)
	DWORD  FlapsDeg;                //how far trailing edge flaps are extended, in degrees 
} UserStateUpdatePacket;

//User has depressed the "voice transmit" button
typedef struct XmitKeydownPacket : public PacketInit<XmitKeydownPacket, TRANSMIT_KEYDOWN_PACKET>
{
	DWORD Unused;
} XmitKeydownPacket;

//User has released the "voice transmit" button
typedef struct XmitKeyupPacket : public PacketInit<XmitKeyupPacket, TRANSMIT_KEYUP_PACKET>
{
	DWORD Unused;
} XmitKeyupPacket;

typedef struct ShutdownPacket : public PacketInit<ShutdownPacket, SHUTDOWN_PACKET>
{
	DWORD Unused;
} ShutdownPacket;

////////////////////////
//Messages sent from server interface

//Local proxy EXE (ServerInterface) has initialized and is ready to receive messages, called after
//creation and before anybody should try to send other packets
typedef struct ProxyReadyPacket : public PacketInit<ProxyReadyPacket, PROXY_READY_PACKET>
{
	DWORD Unused;
} ProxyReadyPacket;

//Saved login information in response to ReqLoginInfoPacket (if none, just don't send this)
typedef struct LoginInfoPacket : public PacketInit<LoginInfoPacket, LOGIN_INFO_PACKET>
{
	char szServerName[64];
	char szUserName[64];
	char szUserID[16];
	char szPassword[32];
	char szCallsign[16];
	char szACType[8];
	char szACEquip[4];
} LoginInfoPacket;

//User has successfully been connected to the network, and server interface is initialized and running
typedef struct ConnectSuccessPacket : public PacketInit<ConnectSuccessPacket, CONNECT_SUCCESS_PACKET>
{
	char szMessage[1024];       
} ConnectSuccessPacket;

//User's connection request has been rejected. Note this doesn't mean the same as connection lost, it could mean bad
//user name, etc. 
typedef struct ConnectFailPacket : public PacketInit<ConnectFailPacket, CONNECT_FAIL_PACKET>
{
	char szReason[512];
} ConnectFailPacket;

//Notification that server has added this object to our "scene" and we will be receiving updates 
typedef struct AddObjectPacket : public PacketInit<AddObjectPacket, ADD_OBJ_PACKET>
{
	char   szCallsign[32];    //must be guaranteed unique
	char   szICAOType[32];    //ICAO type, e.g. B738. This is as reported by the other user, and may or may not be valid
	double LatDegN;         //Initial position and orientation. We should get light, flap etc updates in first update
	double LonDegE;    
	double AltFtMSL;
	double PitchDegUp;              
	double HdgDegTrue;
	double RollDegRight;    //positive going right looking out from object
	double GroundSpeedKts;
} AddObjectPacket;

//Notification server has removed this object from client's "scene"
typedef struct RemoveObjectPacket : public PacketInit<RemoveObjectPacket, REMOVE_OBJ_PACKET>
{
	char szCallsign[32];    
} RemoveObjectPacket;

//Update on a previously-added aircraft. Booleans are 1 if true, 0 if false
typedef struct UpdateObjectPacket : public PacketInit<UpdateObjectPacket, UPDATE_OBJ_PACKET>
{
	char  szCallsign[32];  
	double LatDegN;
	double LonDegE;
	double AltFtMSL;
	double PitchDegUp;              
	double HdgDegTrue;
	double RollDegRight;            //positive going right looking out from object
	double GroundSpeedKts;
	DWORD  bExtendedDataValid;      //1 if the following extended state data is valid. If 0, below state data is unknown 
	double ObjectTimeSecs;         //object's reported time in seconds 
	DWORD  bEnginesOn;
	DWORD  bStrobeLightsOn;
	DWORD  bLandingLightsOn;
	DWORD  bTaxiLightsOn;
	DWORD  bBeaconLightsOn;
	DWORD  bNavLightsOn;
	DWORD  bLogoLightsOn;
	DWORD  bGearDown;               //Really means position of gear handle
	DWORD  bOnGround;				//True if object says it's on the ground (according to the object)
	DWORD  FlapsDeg;                //how far trailing edge flaps are extended, in degrees
} UpdateObjectPacket;

//Notification that server interface is requesting latest user state (client should send UserStateUpdatePacket in response)
typedef struct ReqUserStatePacket : public PacketInit<ReqUserStatePacket, REQ_USER_STATE_PACKET>
{
	DWORD Unused;
} ReqUserStatePacket;

//Notification that user's requested logoff succeeded, and user is now disconnected 
typedef struct LogoffSuccessPacket : public PacketInit<LogoffSuccessPacket, LOGOFF_SUCCESS_PACKET>
{
	char szMessage[256];
} LogoffSuccessPacket;

//Notification that server connection has been lost (i.e. not due to user requested logoff)
typedef struct LostConnectionPacket : public PacketInit<LostConnectionPacket, LOST_CONNECTION_PACKET>
{
	char szReason[256];  
} LostConnectionPacket;

////////////////
//Messages sent from in-game GUI


////////////////
//Messages sent from either


//Be careful not to delete this! Or anybody who includes this will compile byte-aligned which is bad!!
#pragma pack (pop)

