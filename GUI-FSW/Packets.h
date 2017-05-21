//THIS IS A SHARED FILE
//BE CAREFUL WITH MODIFICATIONS!
//
//Also note string size constants are also in code that checks for overwrites, so if you change something, 
//search all the code that references it (e.g. strcpy_s uses a constant because the compiler doesn't realize
//these structures are byte-aligned)
//
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
//packet, just make sure it's not smaller. In practice though packets should be < 1500 to avoid fragmentation.
#define LARGEST_PACKET_SIZE 4096

//Packet identifier, so receiver knows which struct to cast it back into. Note it's possible to receive a 
//packet type that is >= MAX_PACKET_ID, (newer code), and the receiver should just ignore it. 
typedef enum ePacketType
{
	//Messages to server
	SET_CLIENT_INFO_PACKET,
	REQ_LOGIN_INFO_PACKET,
	REQ_CONNECTION_PACKET,
	USER_STATE_UPDATE_PACKET,
	TRANSMIT_KEYDOWN_PACKET,
	TRANSMIT_KEYUP_PACKET,
	REQ_DISCONNECT_PACKET,
	REQ_METAR_PACKET,
	SHUTDOWN_PACKET,
	FLIGHT_PLAN_PACKET,
	SET_RADIO_FREQ_PACKET,

	//Messages to or from either 
	TEXT_MESSAGE_PACKET,

	//Messages from server
	PROXY_READY_PACKET,
	PROXY_MESSAGE_PACKET,
	CONNECT_SUCCESS_PACKET,
	CONNECT_FAIL_PACKET,
	ADD_OBJ_PACKET,
	REMOVE_OBJ_PACKET,
	UPDATE_OBJ_PACKET,
	REQ_USER_STATE_PACKET,
	LOGOFF_SUCCESS_PACKET,
	LOST_CONNECTION_PACKET,
	LOGIN_INFO_PACKET,
	ADD_CONTROLLER_PACKET,
	REMOVE_CONTROLLER_PACKET,
	ADD_SERVER_PACKET,
	METAR_PACKET,

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

//GUI declaring its name and client key with the proxy. Not needed for ServerSim but for actual
//network connection needs to be set prior to sending any other packets
typedef struct SetClientInfoPacket : public PacketInit<SetClientInfoPacket, SET_CLIENT_INFO_PACKET>
{
	char szClientName[32];        //e.g. "VatConnect"
	DWORD VersionNumberMajor;     //e.g. 1
	DWORD VersionNumberMinor;     //e.g 0
	char szFlightSimName[16];     //FS2004, FSX, XPLANE, P3D, DTG, OTHER  (in caps)
	DWORD VatsimClientID;         //assigned by VATSIM
	char szVatsimClientKey[36];   //assigned by VATSIM -- do not share or make public!
} SetClientInfoPacket;

typedef struct ReqLoginInfoPacket : public PacketInit<ReqLoginInfoPacket, REQ_LOGIN_INFO_PACKET>
{
	DWORD Unused;
}ReqLoginInfoPacket;

//User requesting connection to server. 
typedef struct ReqConnectPacket : public PacketInit<ReqConnectPacket, REQ_CONNECTION_PACKET>
{
	char szServerName[64];  //Name as sent in AddServer packet
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

//User requesting METAR weather for given station
typedef struct ReqMetarPacket : public PacketInit<ReqMetarPacket, REQ_METAR_PACKET>
{
	char szStationName[8];
} ReqMetarPacket;

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
	DWORD  TransponderCode;        
	DWORD  TransponderMode;         //0=off, 1=mode C, 2 = Identing
	DWORD  TransmitFreq;            //e.g. 122800 = 122.8
	DWORD  bEnginesOn;              
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

typedef struct FlightPlanPacket : public PacketInit<FlightPlanPacket, FLIGHT_PLAN_PACKET>
{
	char szCallsign[16];         //7 char ICAO callsign
	char szACType[8];            //ICAO Aircraft type e.g. B738
	char szACEquip[4];           //single char e.g. I
	char szDepTime[8];           //time zulu, e.g 0120 
	char szETE[8];               //estimated time enroute, e.g. 0100 (1 hour)
	char szTAS[8];               //True airspeed e.g. 440
	char szAltitude[8];          //Altitude in feet e.g. 33000
	char szRoute[512];           //Route including ICAO departure and arrival airport, e.g KLAX DAG CRL LENDY6 KJFK
	char szRemarks[64];          //Misc. remarks
	DWORD bIsVFR;                //0 if IFR, else VFR
} FlightPlanPacket;

////////////////////////////
// Messages sent to or from server interface

typedef struct TextMessagePacket : public PacketInit<TextMessagePacket, TEXT_MESSAGE_PACKET>
{
	char szMessage[512];         //
	char szSender[32];           //callsign it's from (e.g. DAL123, BOS_APP..  special keywords SERVER, ACARS)
	DWORD bIsPrivateMessage;     //1 if true, 0 if false (regular radio message)
	char szRecipient[32];        //if outgoing private message, callsign sending it to (special keyword SUPE) 
} TextMessagePacket;

////////////////////////
//Messages sent from server interface

//Local proxy EXE (ServerInterface) has initialized and is ready to receive messages, called after
//creation and before anybody should try to send other packets
typedef struct ProxyReadyPacket : public PacketInit<ProxyReadyPacket, PROXY_READY_PACKET>
{
	DWORD Unused;
} ProxyReadyPacket;

//Saved login information in response to ReqLoginInfoPacket (if none, just don't send this).
//Note: changing this will mess up anyone's previously-saved login information. 
typedef struct LoginInfoPacket : public PacketInit<LoginInfoPacket, LOGIN_INFO_PACKET>
{
	char szServerName[64];
	char szUserName[64];
	char szUserID[16];
	char szPassword[32];
	char szCallsign[16];      //Callsign and ACType may be null if last login was as observer
	char szACType[8];         
	char szACEquip[4];        //e.g. I 
} LoginInfoPacket;

//User has successfully been connected to the network, and server interface is initialized and running
typedef struct ConnectSuccessPacket : public PacketInit<ConnectSuccessPacket, CONNECT_SUCCESS_PACKET>
{
	char szMessage[512];       
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

//Notification new controller is in range
typedef struct AddControllerPacket : public PacketInit<AddControllerPacket, ADD_CONTROLLER_PACKET>
{
	char szPosName[64];               //e.g. JFK_TWR
	char szControllerNameRating[64];  //Name and rating e.g. John Smith (S1)
	char szFreq[8];                   //ASCII e.g. 128.9 
	double dLatDegN;
	double dLonDegE;
	char szMessage[512];
} AddControllerPacket;

//Notification to remove controller (out of range or logged off)
typedef struct RemoveControllerPacket : public PacketInit<RemoveControllerPacket, REMOVE_CONTROLLER_PACKET>
{
	char szPosName[64]; 
} RemoveControllerPacket;

//Add given server to the available-servers list
typedef struct AddServerPacket : public PacketInit<AddServerPacket, ADD_SERVER_PACKET>
{
	char szServerName[64];
	char szServerLocation[64];
} AddServerPacket;

//Status or other remark from server proxy
typedef struct ProxyMessagePacket : public PacketInit<ProxyMessagePacket, PROXY_MESSAGE_PACKET>
{
	char szProxyMessage[128];
} ProxyMessagePacket;

//METAR in response to request for it
typedef struct MetarPacket : public PacketInit<MetarPacket, METAR_PACKET>
{
	char szMetar[256];
} MetarPacket;


//Be careful not to delete this! Or anybody who includes this will compile byte-aligned which is bad!!
#pragma pack (pop)

