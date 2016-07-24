#include "stdafx.h"
#include "CFSXInterface.h"

//Forward definitions for C callback functions
void LogException(DWORD ExceptionEnum);
void CALLBACK SimconnectDispatch(SIMCONNECT_RECV* pData, DWORD cbData, void *pContext);


//////////////////////////////////////////
//C functions for SimConnect callbacks 

//Function for debugging SimConnect exceptions
void LogException(DWORD ExceptionEnum)
{
	SIMCONNECT_EXCEPTION e = (SIMCONNECT_EXCEPTION)ExceptionEnum;
	switch(e)
	{
	case SIMCONNECT_EXCEPTION_NONE:
		break;
	case SIMCONNECT_EXCEPTION_ERROR:
		//Log("generic error");
		break;
	case SIMCONNECT_EXCEPTION_SIZE_MISMATCH:
		//Log("size mismatch");
		break;
	case SIMCONNECT_EXCEPTION_UNRECOGNIZED_ID:
		//Log("unrecognized ID");
		break;
	case SIMCONNECT_EXCEPTION_UNOPENED:
		//Log("unopened");
		break;
	case SIMCONNECT_EXCEPTION_VERSION_MISMATCH:
		//Log("version mismatch");
		break;
	case SIMCONNECT_EXCEPTION_TOO_MANY_GROUPS:
		//Log("too many groups");
		break;
	case SIMCONNECT_EXCEPTION_NAME_UNRECOGNIZED:
		//Log("name unrecognized");
		break;
	case SIMCONNECT_EXCEPTION_TOO_MANY_EVENT_NAMES:
		//Log("too many event names");
		break;
	case SIMCONNECT_EXCEPTION_EVENT_ID_DUPLICATE:
		//Log("EventID duplicate");
		break;
	case SIMCONNECT_EXCEPTION_TOO_MANY_MAPS:
		//Log("too many maps");
		break;
	case SIMCONNECT_EXCEPTION_TOO_MANY_OBJECTS:
		//Log("too many objects");
		break;
	case SIMCONNECT_EXCEPTION_TOO_MANY_REQUESTS:
		//Log("too many requests");
		break;
	case SIMCONNECT_EXCEPTION_WEATHER_INVALID_PORT:
		//Log("weather invalid port");
		break;
	case SIMCONNECT_EXCEPTION_WEATHER_INVALID_METAR:
		//Log("invalid METAR");
		break;
	case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_GET_OBSERVATION:
		//Log("unable to get weather observation");
		break;
	case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_CREATE_STATION:
		//Log("unable to create weather station");
		break;
	case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_REMOVE_STATION:
		//Log("unable to remove weather station");
		break;
	case SIMCONNECT_EXCEPTION_INVALID_DATA_TYPE:
		//Log("invalid data type");
		break;
	case SIMCONNECT_EXCEPTION_INVALID_DATA_SIZE:
		//Log("invalid data size");
		break;
	case SIMCONNECT_EXCEPTION_DATA_ERROR:
		//Log("data error");
		break;
	case SIMCONNECT_EXCEPTION_INVALID_ARRAY:
		//Log("invalid array");
		break;
	case SIMCONNECT_EXCEPTION_CREATE_OBJECT_FAILED:
		//Log("create object failed");
		break;
	case SIMCONNECT_EXCEPTION_LOAD_FLIGHTPLAN_FAILED:
		//Log("load flightplan failed");
		break;
	case SIMCONNECT_EXCEPTION_OPERATION_INVALID_FOR_OBJECT_TYPE:
		//Log("operation invalid for object type");
		break;
	case SIMCONNECT_EXCEPTION_ILLEGAL_OPERATION:
		//Log("illegal operation");
		break;
	case SIMCONNECT_EXCEPTION_ALREADY_SUBSCRIBED:
		//Logprintf("already subscribed");
		break;
	case SIMCONNECT_EXCEPTION_INVALID_ENUM:
		//Log("invalid enum");
		break;
	case SIMCONNECT_EXCEPTION_DEFINITION_ERROR:
		//Log("definition error");
		break;
	case SIMCONNECT_EXCEPTION_DUPLICATE_ID:
		//Log("duplicate ID");
		break;
	case SIMCONNECT_EXCEPTION_DATUM_ID:
		//Log("datum ID");
		break;
	case SIMCONNECT_EXCEPTION_OUT_OF_BOUNDS:
		//Log("out of bounds");
		break;
	case SIMCONNECT_EXCEPTION_ALREADY_CREATED:
		//Log("already created");
		break;
	case SIMCONNECT_EXCEPTION_OBJECT_OUTSIDE_REALITY_BUBBLE:
		//Log("object outside reality bubble");
		break;
	case SIMCONNECT_EXCEPTION_OBJECT_CONTAINER:
		//Log("object container");
		break;
	case SIMCONNECT_EXCEPTION_OBJECT_AI:
		//Log("object AI");
		break;
	case SIMCONNECT_EXCEPTION_OBJECT_ATC:
		//Log("object ATC");
		break;
	case SIMCONNECT_EXCEPTION_OBJECT_SCHEDULE:
		//Log("object schedule");
		break;
	default:
		break;
		//Log("unknown");
	}
	return;
}

//Callback from SimConnect when it has an event message for us. pContext is pointer we passed
//in to our FSXInterface instance.
void CALLBACK SimconnectDispatch(SIMCONNECT_RECV* pData, DWORD cbData, void *pContext)
{   
	CFSXInterface* pInstance = (CFSXInterface *)pContext;

    switch(pData->dwID)
    {
 
		case SIMCONNECT_RECV_ID_EVENT_FRAME:
			pInstance->OnFSXFrame();
			break;

        case SIMCONNECT_RECV_ID_EVENT_OBJECT_ADDREMOVE:
        {
            SIMCONNECT_RECV_EVENT_OBJECT_ADDREMOVE *evt = (SIMCONNECT_RECV_EVENT_OBJECT_ADDREMOVE*)pData;
 			
			if (evt->uEventID == EVENT_ADDED_AIRCRAFT)
				pInstance->OnFSXSpawnedObject(evt->dwData);  
			else if (evt->uEventID == EVENT_REMOVED_AIRCRAFT)
				pInstance->OnFSXRemovedObject(evt->dwData); 
            
			break;
        }

        case SIMCONNECT_RECV_ID_ASSIGNED_OBJECT_ID:
        {
            SIMCONNECT_RECV_ASSIGNED_OBJECT_ID *pObjData = (SIMCONNECT_RECV_ASSIGNED_OBJECT_ID*)pData;
    		pInstance->OnFSXAddedObject(pObjData->dwRequestID, pObjData->dwObjectID);
            break;
        }

        case SIMCONNECT_RECV_ID_EVENT:
        {
            SIMCONNECT_RECV_EVENT *evt = (SIMCONNECT_RECV_EVENT*)pData;

            switch(evt->uEventID)
            {
               case EVENT_SIM_RUNNING:
					if (evt->dwData > 0)
						pInstance->OnFSXSimRunning();
					else
						pInstance->OnFSXSimStopped();
					break;
	        }
            break;
        }
		case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
		{
			SIMCONNECT_RECV_SIMOBJECT_DATA *pObjData = (SIMCONNECT_RECV_SIMOBJECT_DATA*) pData;

			//Currently we only request user state
			switch(pObjData->dwRequestID)
			{
				case REQ_USERSTATE:
					pInstance->OnFSXUserStateReceived(&pObjData->dwData);
					break;
			}
			break;
		}

		case SIMCONNECT_RECV_ID_QUIT:
        {
			pInstance->OnFSXExit();
            break;
        }
		case SIMCONNECT_RECV_ID_EXCEPTION:
		{
			SIMCONNECT_RECV_EXCEPTION *pObjData = (SIMCONNECT_RECV_EXCEPTION*)pData;
			
			//Dont log "generic" error because it seems to happen a lot and is ignorable
			if (pObjData->dwException != SIMCONNECT_EXCEPTION_ERROR)
			{
				//Log("Simconnect Exception: ");
				LogException(pObjData->dwException);
			}
			break;
		}
		

    }
}
//////////////////////////////////////////
//CFSXInterface

CFSXInterface::CFSXInterface() : m_hSimConnect(NULL), m_bFSXIn3DView(false), m_bQuit(false), m_bInitialized(false), 
	m_lNextObjID(1), m_bUserStateSet(false), m_bServerInterfaceRunning(false)
{
}

CFSXInterface::~CFSXInterface()
{
	//Cleanly shutdown if it wasn't called already
	if (m_bInitialized)
		Shutdown(false);
}

int CFSXInterface::Initialize()
{

	//Initialize message sender/receiver
	if (!m_Receiver.Initialize(CLIENT_LISTEN_PORT))
	{
		//Log("Failed to open UDP receive socket, check app permissions?");
		return 0;
	}
	if (!m_ServerSender.Initialize(SERVER_INTERFACE_LISTEN_PORT))
	{
		Shutdown(false);
		//Log("Failed to open sending UDP socket, check app permissions?");
		return 0;
	}

	//Initialize SimConnect
	HRESULT hr = S_OK;
	//DEBUG
	/*
	if (SUCCEEDED(SimConnect_Open(&m_hSimConnect, "VatConnect", NULL, 0, 0, 0)))
    {
        //Log("VPC Connected to FSX\n");   
        
		//Note S_OK is 0, so below is quick way to check that all results were S_OK

        //Subscribe to these FSX events
        hr += SimConnect_SubscribeToSystemEvent(m_hSimConnect, EVENT_SIM_RUNNING, "Sim");
        hr += SimConnect_SubscribeToSystemEvent(m_hSimConnect, EVENT_FRAME, "Frame"); 
        hr += SimConnect_SubscribeToSystemEvent(m_hSimConnect, EVENT_ADDED_AIRCRAFT, "ObjectAdded");
        hr += SimConnect_SubscribeToSystemEvent(m_hSimConnect, EVENT_REMOVED_AIRCRAFT, "ObjectRemoved");

		//Define the events we send to FSX
		hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EVENT_FREEZELAT, "FREEZE_LATITUDE_LONGITUDE_SET");
		hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EVENT_FREEZEALT, "FREEZE_ALTITUDE_SET");
		hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EVENT_FREEZEATT, "FREEZE_ATTITUDE_SET");
		hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EVENT_ENGINESON, "ENGINE_AUTO_START");
		hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EVENT_ENGINESOFF, "ENGINE_AUTO_SHUTDOWN");
		hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EVENT_GEARSET, "GEAR_SET");
		hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EVENT_FLAPSUP, "FLAPS_UP");
		hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EVENT_FLAPS1, "FLAPS_1");
		hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EVENT_FLAPS2, "FLAPS_2");
		hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EVENT_FLAPS3, "FLAPS_3");
		hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EVENT_FLAPSFULL, "FLAPS_DOWN");
		hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EVENT_LANDLIGHTSSET, "LANDING_LIGHTS_SET");
		hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EVENT_STROBESSET, "STROBES_SET");
		hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EVENT_NAVLIGHTSTOGGLE, "TOGGLE_NAV_LIGHTS"); 
		hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EVENT_TAXILIGHTSTOGGLE, "TOGGLE_TAXI_LIGHTS");
		hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EVENT_BEACONLIGHTSTOGGLE, "TOGGLE_BEACON_LIGHTS");
		hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EVENT_LOGOLIGHTSTOGGLE, "TOGGLE_LOGO_LIGHTS");
		hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EVENT_TOGGLEJETWAY, "TOGGLE_JETWAY");

		//Define the object state structure we send to FSX for networked aircraft (must correspond to MSLPosOrientStruct)
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, POS_MSL_STRUCT_ID, "plane latitude", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, POS_MSL_STRUCT_ID, "plane longitude", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, POS_MSL_STRUCT_ID, "plane altitude", "feet", SIMCONNECT_DATATYPE_FLOAT64);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, POS_MSL_STRUCT_ID, "plane pitch degrees", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, POS_MSL_STRUCT_ID, "plane heading degrees true", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, POS_MSL_STRUCT_ID, "plane bank degrees", "degrees", SIMCONNECT_DATATYPE_FLOAT64);

		//Define the user aircraft state structure we get from FSX (must correspond to UserStateStruct)
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, USER_STATE_STRUCT_ID, "plane latitude", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, USER_STATE_STRUCT_ID, "plane longitude", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, USER_STATE_STRUCT_ID, "plane altitude", "feet", SIMCONNECT_DATATYPE_FLOAT64);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, USER_STATE_STRUCT_ID, "plane pitch degrees", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, USER_STATE_STRUCT_ID, "plane heading degrees true", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, USER_STATE_STRUCT_ID, "plane bank degrees", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, USER_STATE_STRUCT_ID, "ground velocity", "knots", SIMCONNECT_DATATYPE_FLOAT64);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, USER_STATE_STRUCT_ID, "light strobe", "bool", SIMCONNECT_DATATYPE_INT32);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, USER_STATE_STRUCT_ID, "light landing", "bool", SIMCONNECT_DATATYPE_INT32);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, USER_STATE_STRUCT_ID, "light taxi", "bool", SIMCONNECT_DATATYPE_INT32);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, USER_STATE_STRUCT_ID, "light beacon", "bool", SIMCONNECT_DATATYPE_INT32);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, USER_STATE_STRUCT_ID, "light nav", "bool", SIMCONNECT_DATATYPE_INT32);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, USER_STATE_STRUCT_ID, "light logo", "bool", SIMCONNECT_DATATYPE_INT32);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, USER_STATE_STRUCT_ID, "gear handle position", "bool", SIMCONNECT_DATATYPE_INT32);
		hr += SimConnect_AddToDataDefinition(m_hSimConnect, USER_STATE_STRUCT_ID, "trailing edge flaps left percent", "percent", SIMCONNECT_DATATYPE_FLOAT64);

		if (hr != S_OK)
		{
			//Log("One or more initializers to SimConnect failed!");
			//return 0; //is this really fatal?
		}
	}
	else
	{
		//Log("Failed to connect to FSX!");
		Shutdown(false);
		return 0;
	}
	*/
	m_ModelResolver.Initialize();
	m_bInitialized = true;
	return 1;
}

//Main update, should be called continuously. Returns 1 if successful, 0 if failed, -1 if FSX is shutting down (i.e. user exited)
int CFSXInterface::Update()
{
	if (!m_bInitialized)
		return 0;

	//Process all pending packets from both the server and FSX. If a bunch of packets are pending, keep checking FSX too.
	bool bPacketsPending = false;
	static char PacketBuffer[LARGEST_PACKET_SIZE];

	do
	{
		//Process pending FSX messages, if any. This calls back to our "OnFSX..." methods if there are any, before returning.
		SimConnect_CallDispatch(m_hSimConnect, SimconnectDispatch, this);

		//If user exited FSX, CallDispatch calls back OnFSXExit which shuts us down (sends disconnect to server, closes ports etc)
		//and sets this flag.
		if (m_bQuit)
			return -1;

		//Process next pending packet from the server (if any)
		if (m_Receiver.GetNextPacket(&PacketBuffer))
		{
			ProcessPacket(&PacketBuffer);
			bPacketsPending = true;
		}
		else
			bPacketsPending = false;

	} while (bPacketsPending);
	
	return 1;
}

//Cleanly shut down -- disconnect from SimConnect and close our messaging ports. If bFSXShutdown true, we assume
//this is being called because FSX is exiting (OnFSXExit) and don't bother to remove all objects. If false,
//it's a programmatic shutdown maybe for debugging or user has closed our GUI window so we remove all objects from
//FSX. 
int CFSXInterface::Shutdown(bool bFSXShutdown)
{
	int Size = m_apObjects.GetSize();
	if (Size > 0 && !bFSXShutdown)
	{
		String CS;
		for (int i = 0; i < Size; i++)
		{
			CS = m_apObjects[0]->sCallsign;
			RemoveObject(CS);
		}
	}
	SimConnect_Close(m_hSimConnect);
	
	//Delete our internal array of objects 
	Size = m_apObjects.GetSize();
	for (int i = 0; i < Size; i++)
	{
		delete m_apObjects[i]; 
	}
	m_apObjects.RemoveAll();

	//If user didn't cleanly disconnect before exiting, do it here to shut down server interface EXE
	if (m_bServerInterfaceRunning) 
	{
		ReqDisconnectPacket P;
		m_ServerSender.Send(&P);
		IndicateServerInterfaceShutdown();
	}

	//Close messaging ports
	m_ServerSender.Shutdown();
	m_Receiver.Shutdown();
	m_bInitialized = false;

	return 1;
}

//Given the object's unique callsign, valid FSX aircraft type (i.e. we know it's installed), and initial position, add to the FSX scene. 
//Return once it's been added to the scene, max 3.0 second wait. If it wasn't added in the timeout period, returns 0, else returns 1. Note
//FSX sends two events, one when the object was successfully added to the scene (OnFSXAddedObject), and another when it's been 
//actually spawned (OnFSXSpawnedObject). We just wait for it to be added to the scene. Only first 12 characters in the callsign will show up
//in FSX although it's okay for it to be longer.
int CFSXInterface::AddObject(const String &sCallsign, const String &sFSXType, MSLPosOrientStruct *pPosOrient, long lGroundSpeedKts, long lOnGround)
{
	//See if object already added
	int i = GetIndex(sCallsign);
	if (i >= 0)
		return 1; 
	
	//Create our version of the object
	SimObjectStruct *p = new SimObjectStruct; 
	if (!p)
		return 0;
	p->sCallsign = sCallsign;
	p->lFSXObjectID = -1;
	p->lOurID = m_lNextObjID++;

	//Initialize state interpolater
	p->State.UpdateState(-9999.0, pPosOrient->LatDegN, pPosOrient->LonDegE, pPosOrient->AltFtMSL, pPosOrient->HdgDegTrue,
		-pPosOrient->PitchDegDown, -pPosOrient->RollDegLeft, (double)lGroundSpeedKts);
	
	//Set "last update" state to current.
	p->LastUpdatePacket.LatDegN = pPosOrient->LatDegN;
	p->LastUpdatePacket.LonDegE = pPosOrient->LonDegE;
	p->LastUpdatePacket.AltFtMSL = pPosOrient->AltFtMSL;
	p->LastUpdatePacket.GroundSpeedKts = (double)lGroundSpeedKts;
	p->LastUpdatePacket.HdgDegTrue = pPosOrient->HdgDegTrue;
	p->LastUpdatePacket.PitchDegUp = -pPosOrient->PitchDegDown;
	p->LastUpdatePacket.RollDegRight = -pPosOrient->RollDegLeft;
	p->LastUpdatePacket.bExtendedDataValid = false;   //this gets set to object's initial state in OnFSXAddedObject()

	m_apObjects.Add(p);
	
	//Ask FSX to add it -- note FSX pitch and roll are opposite to ours
	SIMCONNECT_DATA_INITPOSITION InitPos;
	InitPos.Latitude = pPosOrient->LatDegN; 
	InitPos.Longitude = pPosOrient->LonDegE; 
	InitPos.Altitude = pPosOrient->AltFtMSL;
	InitPos.Pitch = pPosOrient->PitchDegDown;
	InitPos.Bank = pPosOrient->RollDegLeft;
	InitPos.Heading = pPosOrient->HdgDegTrue;
	InitPos.OnGround = lOnGround; 
	InitPos.Airspeed = lGroundSpeedKts;  
	
	//Clamp callsign to max 12 characters per SDK documentation
	char cCallsign[13];
	if (sCallsign.size() > 12)
	{
		//There's probably an easier way to do this! 
		for (i = 0; i < 12; i++)
			cCallsign[i] = sCallsign.at(i);
		cCallsign[i] = 0;
	}
	else
	{
		strcpy_s(cCallsign, 13, sCallsign.c_str());
		cCallsign[sCallsign.size()] = 0;
	}

	//Create as AI aircraft. When we get the "added" callback, we disable the AI then.
	SimConnect_AICreateNonATCAircraft(m_hSimConnect, sFSXType.c_str(), cCallsign, InitPos, p->lOurID);

	//Wait for it to be added (up to timeout). As long as it's found it should be added immediately. We wait
	//because our a/c states could get messed up (flaps, lights, etc) if they change before FSX has added it 
	//to the scene. If we timeout it's either an error (object not found) or maybe the user is in the 
	//process of exiting? Caller could maybe ignore the 0 return code. 
	double dStart = m_Time.GetTimeSeconds();
	do
	{
		//Update processing of FS messages -- calls back to OnFSXAddedObject once added
		SimConnect_CallDispatch(m_hSimConnect, SimconnectDispatch, this);
		Sleep(0);
	} while (p->lFSXObjectID == -1 && (m_Time.GetTimeSeconds() - dStart) < 3.0);    

	//timed out
	if (p->lFSXObjectID == -1)
		return 0;

	return 1;
}

//Go through our list of objects and send the latest states to FSX. This is typically done in response to the FSX
//frame event. Return 1 if all succeeded, 0 if any failed
int CFSXInterface::UpdateAllObjects()
{
	static MSLPosOrientStruct P;
	SimObjectStruct *pObj;
	HRESULT hr = S_OK;
	int Num = m_apObjects.GetSize();
	for (int i = 0; i < Num; i++)
	{
		//Get current state 
		pObj = m_apObjects[i];
        if (!pObj)
			continue;
		pObj->State.GetStateNow(&P.LatDegN, &P.LonDegE, &P.AltFtMSL, &P.HdgDegTrue, &P.PitchDegDown, &P.RollDegLeft);
		
		//Negate pitch and roll (FSX opposite to ours)
		P.PitchDegDown *= -1.0;
		P.RollDegLeft *= -1.0;

		//Send to FSX -- S_OK success result is zero, so this is quick way to check if any failed
		if (pObj->lFSXObjectID != -1)
			hr += SimConnect_SetDataOnSimObject(m_hSimConnect, POS_MSL_STRUCT_ID, pObj->lFSXObjectID, NULL, 0, sizeof(MSLPosOrientStruct), &P);
	}
	if (hr != S_OK)
		return 0;
	return 1;
}

//Get the user's current state. Returns 1 if set, 0 if not set because of timeout or error with simconnect
int CFSXInterface::GetUserState(UserStateStruct *pState)
{
	//Send request to FSX
	m_bUserStateSet = false;
	HRESULT hr = SimConnect_RequestDataOnSimObject(m_hSimConnect, REQ_USERSTATE, USER_STATE_STRUCT_ID, SIMCONNECT_OBJECT_ID_USER,
		SIMCONNECT_PERIOD_ONCE);
	if (FAILED(hr))
		return 0;

	//Wait for the callback -- it'll call our OnFSXUserStateReceived() which copies it into member data and sets the m_bUserStateSet flag.
	//Timeout after 0.9 seconds which should be way more than needed. Length is arbitrary but it's in case server interface is requesting 1 second 
	//updates and FSX is crashed or in menus, we don't want to get backed up "Request User State" packets. 
	double dStart = m_Time.GetTimeSeconds();
	do
	{
		//Update processing of FS messages -- callback happens through this function
		SimConnect_CallDispatch(m_hSimConnect, SimconnectDispatch, this);
		Sleep(0);
	} while (!m_bUserStateSet && (m_Time.GetTimeSeconds() - dStart) < 0.9);    

	if (!m_bUserStateSet)
		return 0;
	memcpy(pState, &m_UserState, sizeof(UserStateStruct));
	return 1;
}

//Remove the given object from our list and FSX, returns 1 if removed and 0 if not found.
int CFSXInterface::RemoveObject(const String &sCallsign)
{
	int i = GetIndex(sCallsign);
	if (i < 0)
		return 0;
	SimObjectStruct *pObj = m_apObjects[i];
	
	//Remove from FSX
	SimConnect_AIRemoveObject(m_hSimConnect, pObj->lFSXObjectID, pObj->lOurID);

	//Remove from our list
	m_apObjects.RemoveAt(i);
	delete pObj; 

	return 1;
}

//Return the index in our array of objects (m_apObjects) for the given callsign, or -1 if not found
int CFSXInterface::GetIndex(const String &sCallsign)
{
	int Size = m_apObjects.GetSize();
	for (int i = 0; i < Size; i++)
	{
		if (m_apObjects[i]->sCallsign == sCallsign)
			return i;
	}
	return -1;
}

//Process a packet received from the server or GUI, as defined in Packets.h
int CFSXInterface::ProcessPacket(void *pPacket)
{
	switch(((PacketHeader *)pPacket)->Type)
	{
	case CONNECT_SUCCESS_PACKET:
		return OnServerConnectSuccess((ConnectSuccessPacket *)pPacket);
	case ADD_OBJ_PACKET:
		return OnServerAddObject((AddObjectPacket *)pPacket);
	case REMOVE_OBJ_PACKET:
		return OnServerRemoveObject((RemoveObjectPacket *)pPacket);
	case UPDATE_OBJ_PACKET:
		return OnServerUpdateObject((UpdateObjectPacket *)pPacket);
	case REQ_USER_STATE_PACKET:
		return OnServerReqUserState((ReqUserStatePacket *)pPacket);
	case LOGOFF_SUCCESS_PACKET:
		return OnServerLogoffSuccess((LogoffSuccessPacket *)pPacket);
	case LOST_CONNECTION_PACKET:
		return OnServerLostConnection((LostConnectionPacket *)pPacket);
	}
	return 1;
}

////////////////////////
//Packet handlers for messages from server interface
//

//Request to connect succeeded, and server interface is up and running
int CFSXInterface::OnServerConnectSuccess(ConnectSuccessPacket *p)
{
	m_bConnectionResultReceived = true;
	//TODO send to GUI

	return 1;
}

//Request to connect failed and server interface has shut down
int CFSXInterface::OnServerConnectFail(ConnectFailPacket *p)
{
	m_bConnectionResultReceived = true;
	IndicateServerInterfaceShutdown();
	//TODO send to GUI
	return 1;
}

//Server has added this new object to our scene
int CFSXInterface::OnServerAddObject(AddObjectPacket *p)  
{
	if (!m_bServerInterfaceRunning)
		return 0;

	//Find our best installed FSX model for the requested type and livery
	char cFSXModel[256];
	cFSXModel[0] = '*'; cFSXModel[1] = 0;  //just in case
	m_ModelResolver.GetBestModelForCallsignAndType(p->szCallsign, p->szICAOType, cFSXModel);

	//Add it to our list and into FSX
	String sFSXType = cFSXModel;
	MSLPosOrientStruct Pos;
	Pos.LatDegN = p->LatDegN;
	Pos.LonDegE = p->LonDegE;
	Pos.AltFtMSL = p->AltFtMSL;
	Pos.HdgDegTrue = p->HdgDegTrue;
	Pos.PitchDegDown = -p->PitchDegUp;
	Pos.RollDegLeft = -p->RollDegRight;
	if (AddObject(p->szCallsign, sFSXType, &Pos, (long)p->GroundSpeedKts))
		return 1;
	return 0;
}
//Server is sending update for this object previously added. 1 if ok, 0 if not found.
//Note we may change the passed-in packet data.
int CFSXInterface::OnServerUpdateObject(UpdateObjectPacket *pPacket)
{
	//Find it in our object list
	SimObjectStruct *p = NULL;

	for (int i = 0; !p && i < m_apObjects.GetSize(); i++)
	{
		if (m_apObjects[i]->sCallsign == pPacket->szCallsign)
			p = m_apObjects[i];
	}
	//Return if not found. We might also consider adding it ourselves if we could find the AC type somehow.
	if (!p)
		return 0; 

	//Update the position and orientation in the state interpolater
	p->State.UpdateState(-9999.0, pPacket->LatDegN, pPacket->LonDegE, pPacket->AltFtMSL, pPacket->HdgDegTrue,
		pPacket->PitchDegUp, pPacket->RollDegRight, pPacket->GroundSpeedKts);

	//If no extended data available (gear, flaps, etc) create a state that is appropriate for speed and altitude 
	if (!pPacket->bExtendedDataValid)
	{
		pPacket->bEnginesOn = 1;
		pPacket->bNavLightsOn = 1;
		pPacket->bBeaconLightsOn = 1;
		pPacket->bLogoLightsOn = 1;
		if (pPacket->GroundSpeedKts > 25)
			pPacket->bStrobeLightsOn = 1;
		else
			pPacket->bStrobeLightsOn = 0;
		
		//Determine landing/taxi lights, flaps and gear
		if (pPacket->AltFtMSL < 10500.0)
		{
			if (pPacket->GroundSpeedKts > 25)
			{
				pPacket->bLandingLightsOn = 1;
				pPacket->bTaxiLightsOn = 0;
			}
			else
			{
				pPacket->bLandingLightsOn = 0;
				pPacket->bTaxiLightsOn = 1;
			}
			//Determine flaps and gear
			pPacket->bGearDown = 1;
			//If apparently climbing or accelerating...
			if ((pPacket->GroundSpeedKts - p->LastUpdatePacket.GroundSpeedKts) > 2 ||
				(pPacket->AltFtMSL - p->LastUpdatePacket.AltFtMSL) > 3.0)
			{
				if (pPacket->GroundSpeedKts < 165)
					pPacket->FlapsDeg = 8;
				else if (pPacket->GroundSpeedKts < 200)
				{
					pPacket->FlapsDeg = 5;
					pPacket->bGearDown = 0;
				}
				else
				{
					pPacket->FlapsDeg = 0;
					pPacket->bGearDown = 0;
				}
			}
			//if level, leave unchanged because we don't know if in climbing or descent mode
			else if (abs(pPacket->AltFtMSL - p->LastUpdatePacket.AltFtMSL) < 1.5)
			{
				pPacket->FlapsDeg = p->LastUpdatePacket.FlapsDeg;
				pPacket->bGearDown = p->LastUpdatePacket.bGearDown;
			}
			//Descending
			else
			{
				if (pPacket->GroundSpeedKts < 155)
					pPacket->FlapsDeg = 40;
				else if (pPacket->GroundSpeedKts < 165)
					pPacket->FlapsDeg = 25;
				else if (pPacket->GroundSpeedKts < 175)
					pPacket->FlapsDeg = 10;
				else if (pPacket->GroundSpeedKts < 200)
				{
					pPacket->FlapsDeg = 5;
					pPacket->bGearDown = 0;
				}
				else 
				{
					pPacket->FlapsDeg = 0;
					pPacket->bGearDown = 0;
				}
			}
		}
		//Alt > 10500
		else
		{
			pPacket->bLandingLightsOn = 0;
			pPacket->bTaxiLightsOn = 0;
			pPacket->FlapsDeg = 0;
			pPacket->bGearDown = 0;
		}	
		
		pPacket->bExtendedDataValid = true;
	}
	
	//See what gear/flaps etc states have changed since last update and send the appropriate FSX event
	UpdateObjectPacket *l = &p->LastUpdatePacket;   
	long ID = p->lFSXObjectID;
	if (l->bEnginesOn != pPacket->bEnginesOn)
	{
		if (!pPacket->bEnginesOn)
			SendFSXEvent(ID, EVENT_ENGINESOFF, 0);
		else
			SendFSXEvent(ID, EVENT_ENGINESON, 0);
	}
	if (l->bGearDown != pPacket->bGearDown)
		SendFSXEvent(ID, EVENT_GEARSET, pPacket->bGearDown? 1 : 0);
	if (l->bStrobeLightsOn != pPacket->bStrobeLightsOn)
		SendFSXEvent(ID, EVENT_STROBESSET, pPacket->bStrobeLightsOn? 1 : 0);
	if (l->bLandingLightsOn != pPacket->bLandingLightsOn)
		SendFSXEvent(ID, EVENT_LANDLIGHTSSET, pPacket->bLandingLightsOn? 1 : 0);
	if (l->bTaxiLightsOn != pPacket->bTaxiLightsOn)
		SendFSXEvent(ID, EVENT_TAXILIGHTSTOGGLE, 0);
	if (l->bBeaconLightsOn != pPacket->bBeaconLightsOn)
	{
		SendFSXEvent(ID, EVENT_BEACONLIGHTSTOGGLE, 0);
		SendFSXEvent(ID, EVENT_TOGGLEJETWAY, 0); //do jetway here because normally toggle beacon when shutting down or about to pushback
	}
	if (l->bNavLightsOn != pPacket->bNavLightsOn)
		SendFSXEvent(ID, EVENT_NAVLIGHTSTOGGLE, 0);
	if (l->bLogoLightsOn != pPacket->bLogoLightsOn)
		SendFSXEvent(ID, EVENT_LOGOLIGHTSTOGGLE, 0);

	//Determine new flap setting
	if (l->FlapsDeg != pPacket->FlapsDeg)
	{
		if (pPacket->FlapsDeg == 0)
			SendFSXEvent(ID, EVENT_FLAPSUP, 0);
		else if (pPacket->FlapsDeg <= 5)
			SendFSXEvent(ID, EVENT_FLAPS1, 1);
		else if (pPacket->FlapsDeg <= 15)
			SendFSXEvent(ID, EVENT_FLAPS2, 1);
		else if (pPacket->FlapsDeg <= 25)
			SendFSXEvent(ID, EVENT_FLAPS3, 1);
		else
			SendFSXEvent(ID, EVENT_FLAPSFULL, 1);
	}

	//Copy in this new state to our last
	memcpy (&p->LastUpdatePacket, pPacket, sizeof(UpdateObjectPacket));

	return 1;
}

//Server has removed this object from our scene
int CFSXInterface::OnServerRemoveObject(RemoveObjectPacket *p)
{
	if (!m_bServerInterfaceRunning)
		return 0;

	String sCallsign = p->szCallsign;
	if (RemoveObject(sCallsign))
		return 1;
	return 0;
}

//Server is asking us to send user aircraft's current state
int CFSXInterface::OnServerReqUserState(ReqUserStatePacket *p)
{
	if (!m_bServerInterfaceRunning)
		return 0;

	UserStateStruct S;
	if (!GetUserState(&S))
		return 0;

	//Fill in the user state update packet
	UserStateUpdatePacket P;
	P.UserTimeSecs = m_Time.GetTimeSeconds();
	P.LatDegN = S.LatDegN;
	P.LonDegE = S.LonDegE;
	P.AltFtMSL = S.AltFtMSL;
	P.HdgDegTrue = S.HdgDegTrue;
	P.PitchDegUp = -S.PitchDegDown;
	P.RollDegRight = -S.RollDegLeft;
	P.GroundSpeedKts = S.GroundSpeedKts;
	P.bStrobeLightsOn = S.bStrobeLightsOn;
	P.bLandingLightsOn = S.bLandingLightsOn;
	P.bTaxiLightsOn = S.bTaxiLightsOn;
	P.bBeaconLightsOn = S.bBeaconLightsOn;
	P.bNavLightsOn = S.bNavLightsOn;
	P.bLogoLightsOn = S.bLogoLightsOn;
	P.bGearDown = S.bGearHandleDown;
	P.FlapsDeg = (DWORD)(S.FlapsPct * 40.0 + 0.5);    

	//Send it
	if (m_ServerSender.Send(&P))
		return 1;
	return 0;
}

//Disconnect request has succeeded and server interface has shut down
int CFSXInterface::OnServerLogoffSuccess(LogoffSuccessPacket *p)
{
	//TODO send message to GUI that we're disconnected
	IndicateServerInterfaceShutdown();
	return 1;
}

//Server interface has lost connection to server and has shut down
int CFSXInterface::OnServerLostConnection(LostConnectionPacket *p)
{
	//TODO Send message and reason string to GUI

	IndicateServerInterfaceShutdown();
	return 1;
}


////////////////
//Message handler for messages from GUI

//User requests connection to server. Returns 1 if server responded, 0 if problem starting up server interface
int CFSXInterface::OnUserReqConnect()
{
	//Launch the server interface EXE
	if (!m_bServerInterfaceRunning)
	{
		ZeroMemory(&m_ServerProcStartupInfo, sizeof(m_ServerProcStartupInfo));
		m_ServerProcStartupInfo.cb = sizeof(m_ServerProcStartupInfo);
		ZeroMemory(&m_ServerProcInfo, sizeof(m_ServerProcInfo));
		/*DEBUG
		if (!CreateProcess(L"Vatsim Interface.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &m_ServerProcStartupInfo, &m_ServerProcInfo))
		{
			//Log("Couldn't launch Server Interface!");
			return 0;
		}*/

		m_bServerInterfaceRunning = true;
	}
	//Server could take unknown time to launch and initialize, so keep sending
	//connection request packets and polling for incoming messages until we get 
	//a connection failed or succeeded result (m_bConnectionResultReceived set to
	//true).

	//Fill out the packet fields -- DEBUG we should be getting this from the GUI passed as a parameter
	ReqConnectPacket RC;
	strcpy_s(RC.szLoginName, 32,  "00000000");
	strcpy_s(RC.szPassword, 32, "123456");

	//Keep sending to server until we get a reply -- DEBUG don't timeout from this
	//so we can run server interface in the debugger. But eventually we should
	//have a timeout.
	m_bConnectionResultReceived = false;
	int UpResult = 1;
	MSG msg;

	while (!m_bConnectionResultReceived && UpResult != -1 && !m_bQuit) //TODO && not timed out yet
	{
		m_ServerSender.Send(&RC);
		Sleep(10);

		//Process any messages
		UpResult = Update();
		
		//Process any windows messages
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT || msg.message == WM_DESTROY)
				m_bQuit = true;
			else if (msg.message != WM_NULL) 
			{
				if (msg.message != WM_SYSKEYDOWN)
					TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	return 1;
}

//User has requested disconnect -- remove all objects and notify server. Note we might
//still have server messages like update and add object in our queue -- we will ignore those.
//Server though will send "logoff successful" message we can process. Also note server is 
//responsible to shut itself down, so we flag it's not running here.
int CFSXInterface::OnUserReqDisconnect()
{
	while (m_apObjects.GetSize())
	{
		//This shrinks the array so there's always an element 0
		RemoveObject(m_apObjects[0]->sCallsign);
	}
	
	if (m_bServerInterfaceRunning)
	{
		ReqDisconnectPacket P;
		m_ServerSender.Send(&P);
		IndicateServerInterfaceShutdown();
	}
	return 1;
}

void CFSXInterface::IndicateServerInterfaceShutdown()
{
	if (m_bServerInterfaceRunning)
	{
		CloseHandle(m_ServerProcInfo.hProcess);
		CloseHandle(m_ServerProcInfo.hThread);
		m_bServerInterfaceRunning = false;
	}
	return;
}
////////////////////////
//Handler for messages from FSX (callbacks to our C functions)

//FSX has gone into normal flight mode (i.e. user's aircraft running)
void CFSXInterface::OnFSXSimRunning()
{
	//DEBUG! Automatically connect on first startup
	if (!m_bServerInterfaceRunning)
		OnUserReqConnect();


	return;
}

//User's aircraft has been paused either by user or FSX is doing something else (menus, reloading, etc)
void CFSXInterface::OnFSXSimStopped()
{
	//DEBUG!automatically disconnect on FSX pause or exit
	if (m_bServerInterfaceRunning)
		OnUserReqDisconnect();

	return;
}

//Notification FSX is ready for the next frame (ready to begin drawing it)
void CFSXInterface::OnFSXFrame()                         
{
	UpdateAllObjects();
	return;
}

//FSX has removed given FSX object number. Hopefully this is only done in response to our
//Remove Object, so we've already deleted it from our list and all we do is return. If some aircraft
//are invisible, we should check if it's still in our list and maybe respawn? //REVISIT 
void CFSXInterface::OnFSXRemovedObject(DWORD FSXObjectID)
{
	return;
}

//FSX has visibly spawned the given FSX object number 
void CFSXInterface::OnFSXSpawnedObject(DWORD FSXObjectID)
{

	return;
}

//Requested added object has been added to FSX and assigned an FSXObject number (though not necessarily visible)
void CFSXInterface::OnFSXAddedObject(DWORD OurObjectID, DWORD FSXObjectID)
{
	for (int i = 0; i < m_apObjects.GetSize(); i++)
	{
		if (m_apObjects[i]->lOurID == (long)OurObjectID)
		{
			m_apObjects[i]->lFSXObjectID = FSXObjectID;
			
			//Remove its AI and "freeze" its position so we can control it directly
			SimConnect_AIReleaseControl(m_hSimConnect, FSXObjectID, OurObjectID);
			SimConnect_TransmitClientEvent(m_hSimConnect, FSXObjectID, EVENT_FREEZELAT, 1, 
					SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);	
			SimConnect_TransmitClientEvent(m_hSimConnect, FSXObjectID, EVENT_FREEZEALT, 1, 
					SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);	
			SimConnect_TransmitClientEvent(m_hSimConnect, FSXObjectID, EVENT_FREEZEATT, 1, 
					SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);	

			//Set initial object state and set "last update received" state to the same. When
			//we get the first update, we'll set the correct state by comparing to this last update
			m_apObjects[i]->LastUpdatePacket.bExtendedDataValid = 1;
			m_apObjects[i]->LastUpdatePacket.bEnginesOn = 0;
			m_apObjects[i]->LastUpdatePacket.bStrobeLightsOn = 0;
			m_apObjects[i]->LastUpdatePacket.bLandingLightsOn = 0;
			m_apObjects[i]->LastUpdatePacket.bTaxiLightsOn = 0;
			m_apObjects[i]->LastUpdatePacket.bBeaconLightsOn = 0;
			m_apObjects[i]->LastUpdatePacket.bNavLightsOn  = 0;
			m_apObjects[i]->LastUpdatePacket.bLogoLightsOn = 0;
			SendFSXEvent(FSXObjectID, EVENT_GEARSET, 1); 
			m_apObjects[i]->LastUpdatePacket.bGearDown = 1;
			m_apObjects[i]->LastUpdatePacket.bOnGround = 0;
			m_apObjects[i]->LastUpdatePacket.FlapsDeg = 0;
			
			return;
		}
	}
	return;
}

//User has exited FSX
void CFSXInterface::OnFSXExit()
{
	Shutdown(true);
	m_bQuit = true;
	return;
}

//Request for user state from FSX has been received. Set the member data and flag it's received. GetUserState() is waiting on this.
void CFSXInterface::OnFSXUserStateReceived(void *pUserStateStruct)
{
	UserStateStruct *p = (UserStateStruct *)pUserStateStruct;
	memcpy (&m_UserState, p, sizeof(UserStateStruct));
	m_bUserStateSet = true;
	return;
}

int CFSXInterface::SendFSXEvent(long FSXObjectID, eFSXEvent Event, int Param)
{
	HRESULT hr = SimConnect_TransmitClientEvent(m_hSimConnect, FSXObjectID, Event, Param, SIMCONNECT_GROUP_PRIORITY_HIGHEST, 
		SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);	
	if (SUCCEEDED(hr))
		return 1;
	return 0;
}