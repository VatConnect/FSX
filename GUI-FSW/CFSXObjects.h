
#pragma once

#include "Packets.h"
#include "CStateInterpolater.h"
#include "CPacketReceiver.h"
#include "CPacketSender.h"
#include "CFSXModelResolver.h"
#include "CTime.h"
#include "GUI-FSW.h"
#include "CFSXGUI.h"
#include "SimConnectFSW.h"


//STL string class we use
#include <string>
typedef std::basic_string<char> String;

//Structure we keep on each object we're displaying in FSX
typedef struct SimObjectStruct
{
	SimObjectStruct() : lFSXObjectID(-1), lOurID(-1), bOnGround(false), dGroundElevFt(0.0){};

	long			   lFSXObjectID;      //FSX assigned object number. -1 = not assigned yet (not added to FSX yet)
	long			   lOurID;            //Unique ID# we assign 
	String			   sCallsign;         //Object's callsign as reported by object, needs to be unique
	UpdateObjectPacket LastUpdatePacket;  //Copy of last state packet we received, to detect change in gear, light etc states 
	CStateInterpolater State;             //Class that smooths position/orientation because FSX needs frequent updates while we get sparse updates
	bool			   bOnGround;         //True to clamp to ground
	bool			   bLikelyLanding;    //True if we think a/c is landing: dGroundElevFt, dAltAGLFt is valid and we should clamp to that minimum altitude
	double			   dGearHeightFt;     //Height of the spawned object's gear (diff between object's "zero" height and offset of highest landing gear 
	double			   dGroundElevFt;     //if bLikelyLanding, approx ground height reported by FSX at last update. 
	double			   dAltAGLFt;         //valid if bLikelyLanding true (i.e. we're polling for ground height), does not include landing gear
	bool			   bIsJet;            //True if the aircraft is jet (for deriving flap and gear states)
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

//This must exactly correspond to MSLPosOrientStruct because we actually send a MSLPosOrientStruct with AltFtMSL changed
#define POS_AGL_STRUCT_ID 2
typedef struct AGLPosOrientStruct
{
	double LatDegN;
	double LonDegE;
	double AltFtAGL;
	double PitchDegDown;
	double HdgDegTrue;
	double RollDegLeft;
} AGLPosOrientStruct;

#define USER_STATE_STRUCT_ID 3
typedef struct UserStateStruct
{
	double LatDegN;
	double LonDegE;
	double AltFtMSL;
	double PitchDegDown;
	double HdgDegTrue;
	double RollDegLeft;
	double GroundSpeedKts;
	DWORD  XpndrCode;
	DWORD  Com1Freq;
	DWORD  EngineRPM;
	DWORD  bOnGround;
	DWORD  bStrobeLightsOn;
	DWORD  bLandingLightsOn;
	DWORD  bTaxiLightsOn;
	DWORD  bBeaconLightsOn;
	DWORD  bNavLightsOn;
	DWORD  bLogoLightsOn;
	DWORD  bGearHandleDown;
	double FlapsPct;
} UserStateStruct;

#define AGL_STRUCT_ID 4
typedef struct AGLStruct
{
	double dAGLAltitude;
} AGLStruct;

//ID of data requests we send to FSX. FSX returns the request ID in the callback.
typedef enum eFSXRequestID
{
	REQ_USERSTATE,
	REQ_OBJAGL
} eFSXRequestID;


typedef class CFSXObjects
{

public:
	CFSXObjects();
	~CFSXObjects();

	//Initialize and connect to FSX. Returns 1 if succeeded, 0 if failed.
	int Initialize(CPacketSender *pSender, HANDLE hSimConnect, DispatchProc pfnSimconnectDispatchProc, CFSXGUI *pGUI);

	//Call this continuously to check and process messages to and from the server, GUI and FSX. Returns 1 success, 0 fail, -1 if FSX has shut down 
	int Update();

	//Call after disconnect or to shutdown
	int RemoveAllObjects();

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
	void OnFSXObjectAGLReceived(void *pAGLStruct);        //Request for AGL altitude data on other object was received

	////////////////
	//Messages received from server (from packets.h) returns 1 if processed, 0 if not
	int	ProcessPacket(void *pPacket);

protected:

	//Add an object to our list, and spawn in FSX. Returns once object has been added to FSX (max 3 seconds).
	int AddObject(const String &sCallsign, const String &sFSXType, double dGearHeightFt, bool bIsJet,
		MSLPosOrientStruct *pPosOrient, long lGroundSpeedKts, long lOnGround = 0);
	
	//Send the latest object states to FSX
	int UpdateAllObjects();

	//Get the user's current state from FSX. 
	int GetUserState(UserStateStruct *pState);
	
	//Remove the given object from our list and FSX
	int RemoveObject(const String &sCallsign);

	//Return the index in the m_apObjects array for the given callsign, -1 if not found
	int	GetIndex(const String &sCallsign); 

	//Send object event to FSX
	int SendFSXEvent(long FSXObjectID, eFSXEvent Event, int Param);

	//Get given object's altitude in AGL
	int GetObjectAGL(long FSXObjectID, double *pdAltAGLFt);

	///////////////
	//Process specific packets from server 
	int OnServerConnectSuccess(ConnectSuccessPacket *p);   //Request to connect succeeded, server interface is up and running
	int OnServerConnectFail(ConnectFailPacket *p);         //Request to connect failed and server interface has shut down
	int OnServerAddObject(AddObjectPacket *p);             //Server has added this object to our scene
	int OnServerUpdateObject(UpdateObjectPacket *p);       //Server reporting update for this object
	int OnServerRemoveObject(RemoveObjectPacket *p);       //Server has removed this object from our scene
	int OnServerReqUserState(ReqUserStatePacket *p);       //Server interface requests the user's current state
	int OnServerLogoffSuccess(LogoffSuccessPacket *p);     //Disconnect request has succeeded and server interface has shut down
	int OnServerLostConnection(LostConnectionPacket *p);   //Server interface has lost connection to server 
	
	///////////////
	//Member data
	CPacketSender*	m_pSender;          //Sender of packets to server  
	DispatchProc    m_pfnSimconnectDispatchProc; //pointer to our SimConnect dispatch procedure

	CFSXModelResolver m_ModelResolver;  //Singleton used to get FSX model names 
	CFSXGUI*        m_pGUI;

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

	//Similar data but for GetObjectAGL()
	double			m_dObjAGL;
	bool			m_bObjAGLSet;

} CFSXObjects;
