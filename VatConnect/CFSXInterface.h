
#pragma once

#include "Packets.h"
#include "CStateInterpolater.h"
#include "CPacketReceiver.h"
#include "CPacketSender.h"
#include "CFSXModelResolver.h"
#include "CTime.h"
#pragma warning(push, 1)
#include "SimConnect.h"
#pragma warning(pop)
#pragma comment(lib, "SimConnect.lib")

//STL string class we use
#include <string>
typedef std::basic_string<char> String;

//Structure we keep on each object we're displaying in FSX
typedef struct SimObjectStruct
{
	SimObjectStruct() : lFSXObjectID(-1), lOurID(-1){};

	long			   lFSXObjectID;      //FSX assigned object number. -1 = not assigned yet (not added to FSX yet)
	long			   lOurID;            //Unique ID# we assign 
	String			   sCallsign;         //Object's callsign as reported by object, needs to be unique
	UpdateObjectPacket LastUpdatePacket;  //Copy of last state packet we received, to detect change in gear, light etc states 
	CStateInterpolater State;             //Class that smooths position/orientation because FSX needs frequent updates while we get sparse updates
} SimObjectStruct;

//Structures we pass to and from FSX. These must correspond to the definitions we send in Initialize()
#define POS_MSL_STRUCT_ID 1
typedef struct MSLPosOrientStruct
{
	double LatDegN;
	double LonDegE;
	double AltFtMSL;
	double PitchDegDown;
	double HdgDegTrue;
	double RollDegLeft;    //looking out of object
} MSLPosOrientStruct;

#define USER_STATE_STRUCT_ID 2
typedef struct UserStateStruct
{
	double LatDegN;
	double LonDegE;
	double AltFtMSL;
	double PitchDegDown;
	double HdgDegTrue;
	double RollDegLeft;
	double GroundSpeedKts;
	DWORD  bEngineOn;
	DWORD  bStrobeLightsOn;
	DWORD  bLandingLightsOn;
	DWORD  bTaxiLightsOn;
	DWORD  bBeaconLightsOn;
	DWORD  bNavLightsOn;
	DWORD  bLogoLightsOn;
	DWORD  bGearHandleDown;
	double FlapsPct;
} UserStateStruct;

//Simulator events for simconnect
typedef enum eFSXEvent 
{
	//We subscribe to these FSX events
	EVENT_SIM_RUNNING,
	EVENT_ADDED_AIRCRAFT,
	EVENT_REMOVED_AIRCRAFT,
	EVENT_FRAME,
 
	//We send these events to FSX (in SendFSXEvent)
	EVENT_FREEZELAT,
	EVENT_FREEZEALT,
	EVENT_FREEZEATT,
    EVENT_ENGINESON, 
	EVENT_ENGINESOFF,
	EVENT_GEARSET,             
	EVENT_FLAPSUP, 
	EVENT_FLAPS1,
	EVENT_FLAPS2,
	EVENT_FLAPS3,
	EVENT_FLAPSFULL,
	EVENT_LANDLIGHTSSET,        //data field 1 or 0 (on or off)
	EVENT_STROBESSET,           //data field 1 or 0
	EVENT_NAVLIGHTSTOGGLE, 
	EVENT_TAXILIGHTSTOGGLE,
	EVENT_BEACONLIGHTSTOGGLE,
	EVENT_LOGOLIGHTSTOGGLE,
	EVENT_TOGGLEJETWAY,

	EVENT_HIGHEST
} eFSXEvent;

//ID of data requests we send to FSX. FSX returns the request ID in the callback.
typedef enum eFSXRequestID
{
	REQ_USERSTATE
} eFSXRequestID;


typedef class CFSXInterface
{

public:
	CFSXInterface();
	~CFSXInterface();

	//Initialize and connect to FSX. Returns 1 if succeeded, 0 if failed.
	int Initialize();

	//Call this continuously to check and process messages to and from the server, GUI and FSX. Returns 1 success, 0 fail, -1 if FSX has shut down 
	int Update();

	//Clean shutdown, disconnect from FSX and cleanup. bFSXExit true if FSX exiting (so we don't bother to remove objects), or if false
	//we assume FSX still running and ask it to remove all objects
	int Shutdown(bool bFSXShuttingDown);

	/////////////////
	//Events received from FSX, called from the callback functions so needs to be public 
	void OnFSXSimRunning();					   //FSX has gone into normal flight mode (i.e. user's aircraft running) 
	void OnFSXSimStopped();                    //User's aircraft has been paused, whether by user or FSX is doing something else (menus, reloading, etc)
	void OnFSXFrame();                         //Notification that FSX is ready for the next frame (ready to begin drawing it)
	void OnFSXRemovedObject(DWORD FSXObjectID); //FSX has removed given FSX object number
	void OnFSXSpawnedObject(DWORD FSXObjectID);  //FSX has visibly spawned the given FSX object number  
 	void OnFSXAddedObject(DWORD OurObjectID, DWORD FSXObjectID);  //Requested object has been added and assigned an FSXObject number 
	void OnFSXExit();                          //User has exited FSX
	void OnFSXUserStateReceived(void *pUserStateStruct);  //Request for data on user object was received

protected:

	//Add an object to our list, and spawn in FSX. Returns once object has been added to FSX (max 3 seconds).
	int AddObject(const String &sCallsign, const String &sFSXType, MSLPosOrientStruct *pPosOrient, long lGroundSpeedKts, long lOnGround = 0);
	
	//Send the latest object states to FSX
	int UpdateAllObjects();

	//Get the user's current state from FSX. 
	int GetUserState(UserStateStruct *pState);
	
	//Remove the given object from our list and FSX
	int RemoveObject(const String &sCallsign);

	//Return the index in the m_apObjects array for the given callsign, -1 if not found
	int	GetIndex(const String &sCallsign); 

	//Indicate the server interface has shutdown (could happen in several different situations)
	void IndicateServerInterfaceShutdown();

	//Send object event to FSX
	int SendFSXEvent(long FSXObjectID, eFSXEvent Event, int Param);

	//Process the received packet (from packet.h) on our listening port, could be from anyone
	int	ProcessPacket(void *pPacket);  

	////////////////
	//Process packets sent from in-game GUI 
	int OnUserReqConnect(); 
	int OnUserReqDisconnect();

	///////////////
	//Process packets sent from server 
	//
	int OnServerConnectSuccess(ConnectSuccessPacket *p);   //Request to connect succeeded, server interface is up and running
	int OnServerConnectFail(ConnectFailPacket *p);         //Request to connect failed and server interface has shut down
	int OnServerAddObject(AddObjectPacket *p);             //Server has added this object to our scene
	int OnServerUpdateObject(UpdateObjectPacket *p);       //Server reporting update for this object
	int OnServerRemoveObject(RemoveObjectPacket *p);       //Server has removed this object from our scene
	int OnServerReqUserState(ReqUserStatePacket *p);       //Server interface requests the user's current state
	int OnServerLogoffSuccess(LogoffSuccessPacket *p);     //Disconnect request has succeeded and server interface has shut down
	int OnServerLostConnection(LostConnectionPacket *p);   //Server interface has lost connection to server and has shut down
	
	///////////////
	//Member data
	CPacketReceiver	m_Receiver;         //Listener for packets sent to us
	CPacketSender	m_ServerSender;     //Sender of packets to server  
	CFSXModelResolver m_ModelResolver;  //Singleton used to get FSX model names 
	CSimpleArray<SimObjectStruct *> m_apObjects; //Our list of currently-displayed objects
	CTime	m_Time;						//Our time
	HANDLE	m_hSimConnect;				//Handle to the SimConnect session
	bool    m_bFSXIn3DView;				//True when FSX is running the simulation (false when in menus, reloading, etc)
	bool    m_bQuit;					//True when FSX has told us to exit (because it's exiting...) 
	bool	m_bInitialized;             //True once initialized, false once shut down
	long	m_lNextObjID;               //Our ID we assign to the object (FSX needs this to add the object)

	//Data passed between GetUserState() and FSX callback OnFSXUserStateReceived. When received, it's set
	//here with the m_bUserStateSet flag set to true, so GetUserState() knows it's been received.  
	UserStateStruct m_UserState;
	bool		    m_bUserStateSet;

	//Information on the Server Interface process we launch
	bool			m_bServerInterfaceRunning;  //True if process has been started (note if it crashes, we won't know unless we poll for it)
	STARTUPINFO		m_ServerProcStartupInfo;
	PROCESS_INFORMATION m_ServerProcInfo;
	bool			m_bConnectionResultReceived;  //Set to true after server interface has launched, initialized, and processed our connection request

} CFSXInterface;
