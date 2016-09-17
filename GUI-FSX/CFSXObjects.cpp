#include "stdafx.h"
#include "CFSXObjects.h"

#define ON_GROUND_SPD_KTS 30   //for objects not reporting extended data, speed below which aircraft is clamped to the ground regardless of its reported altitude... //REVISIT
#define GND_ELEV_CHECK_ALTMSL 6000 //For all objects descending, are below this altitude, and speed less than 155 kts (i.e. likely on short final), get the ground elevation
                                   //from FSX at each update so we don't interpolate below the ground when landing

//////////////////////////////////////////
//CFSXObjects

CFSXObjects::CFSXObjects() : m_hSimConnect(NULL), m_bFSXIn3DView(false), m_bQuit(false), m_bInitialized(false), 
	m_lNextObjID(1), m_bUserStateSet(false)
{
}

CFSXObjects::~CFSXObjects()
{
	//Cleanly shutdown if it wasn't called already
	if (m_bInitialized)
		RemoveAllObjects();
}

int CFSXObjects::Initialize(CPacketSender *pSender, HANDLE hSimConnect, DispatchProc pfnSimconnectDispatchProc,
	                        CFSXGUI *pGUI)
{

	m_pSender = pSender;
	m_pfnSimconnectDispatchProc = pfnSimconnectDispatchProc;
	m_hSimConnect = hSimConnect;
	m_pGUI = pGUI;
	
	m_ModelResolver.Initialize();
	m_bInitialized = true;
	return 1;
}

//Main update, should be called continuously. Returns 1 if successful, 0 if failed, -1 if FSX is shutting down (i.e. user exited)
int CFSXObjects::Update()
{
	if (!m_bInitialized)
		return 0;

	return 1;
}

//Process a packet received from the server, as defined in Packets.h
int CFSXObjects::ProcessPacket(void *pPacket)
{
	switch (((PacketHeader *)pPacket)->Type)
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


//Remove all objects, e.g. user has disconnected or we are shutting down
int CFSXObjects::RemoveAllObjects()
{
	int Size = m_apObjects.GetSize();
	if (Size > 0)
	{
		String CS;
		for (int i = 0; i < Size; i++)
		{
			CS = m_apObjects[0]->sCallsign;
			RemoveObject(CS);
		}
	}
	
	//Delete our internal array of objects 
	Size = m_apObjects.GetSize();
	for (int i = 0; i < Size; i++)
	{
		delete m_apObjects[i]; 
	}
	m_apObjects.RemoveAll();

	m_bInitialized = false;

	return 1;
}

//Given the object's unique callsign, valid FSX aircraft type (i.e. we know it's installed), and initial position, add to the FSX scene. 
//Return once it's been added to the scene, max 3.0 second wait. If it wasn't added in the timeout period, returns 0, else returns 1. Note
//FSX sends two events, one when the object was successfully added to the scene (OnFSXAddedObject), and another when it's been 
//actually spawned (OnFSXSpawnedObject). We just wait for it to be added to the scene. Only first 12 characters in the callsign will show up
//in FSX although it's okay for it to be longer.
int CFSXObjects::AddObject(const String &sCallsign, const String &sFSXType, double dGearHeightFt,
	bool bIsJet, MSLPosOrientStruct *pPosOrient, long lGroundSpeedKts, long lOnGround)
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
	p->bOnGround = lOnGround ? true : false;
	p->bLikelyLanding = false;
	p->dGearHeightFt = dGearHeightFt;
	p->bIsJet = bIsJet;

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
	
	//Clamp callsign to max 12 characters (including terminating 0) per SDK documentation
	char cCallsign[13];
	if (sCallsign.size() > 11)
	{
		//There's probably an easier way to do this! 
		for (i = 0; i < 11; i++)
			cCallsign[i] = sCallsign.at(i);
		cCallsign[i] = 0;
	}
	else
	{
		strcpy_s(cCallsign, 12, sCallsign.c_str());
		cCallsign[sCallsign.size()] = 0;
	}

	//Create as AI aircraft. When we get the "added" callback, we disable the AI then.
	SimConnect_AICreateNonATCAircraft(m_hSimConnect, sFSXType.c_str(), cCallsign, InitPos, p->lOurID);

	//Wait for it to be added (up to timeout). As long as it's found it should be added immediately. We wait
	//because our a/c states could get messed up (flaps, lights, etc) if they change before FSX has added it 
	//to the scene. If we timeout it's either an error (object not found) or maybe the user is in the 
	//process of exiting? Caller could maybe ignore the 0 return code. 
	double dStart = m_Time.GetTimeSeconds();
	MSG msg;
	do
	{
		//Update processing of FS messages -- calls back to OnFSXAddedObject once added
		SimConnect_CallDispatch(m_hSimConnect, m_pfnSimconnectDispatchProc, this);
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			DispatchMessage(&msg);
		Sleep(0);
	} while (p->lFSXObjectID == -1 && (m_Time.GetTimeSeconds() - dStart) < 3.0);    

	//timed out?
	if (p->lFSXObjectID == -1)
	{
		WCHAR Buf[256];
		wsprintf(Buf, L"FSX failed to add %s", sCallsign);
		m_pGUI->AddErrorMessage(Buf);
		return 0;
	}
	return 1;
}

//Go through our list of objects and send the latest states to FSX. This is typically done in response to the FSX
//frame event. Return 1 if all succeeded, 0 if any failed
int CFSXObjects::UpdateAllObjects()
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

		//Clamp to ground if on or apparently below ground
		if (pObj->bLikelyLanding || pObj->bOnGround)
		{
			if (!pObj->bOnGround)
			{
				double GroundZ = pObj->dGroundElevFt + pObj->dGearHeightFt;
				if (P.AltFtMSL <= GroundZ)
				{
					P.AltFtMSL = GroundZ;
					pObj->bOnGround = true;
					pObj->dAltAGLFt = pObj->dGearHeightFt;
				}
				else
					pObj->dAltAGLFt = P.AltFtMSL - pObj->dGroundElevFt;
			}
			else
				pObj->dAltAGLFt = pObj->dGearHeightFt;
		}

		//Send to FSX -- S_OK success result is zero, so this is quick way to check if any failed
		if (pObj->lFSXObjectID != -1)
		{ 
			if (pObj->bOnGround)
			{
				P.AltFtMSL = pObj->dGearHeightFt;

				//We use our MSLPosOrientStruct above but tell FSX it's an AGL struct because they are defined the same.  
				hr += SimConnect_SetDataOnSimObject(m_hSimConnect, POS_AGL_STRUCT_ID, pObj->lFSXObjectID, NULL, 0, sizeof(AGLPosOrientStruct), &P);
			}
			else
				hr += SimConnect_SetDataOnSimObject(m_hSimConnect, POS_MSL_STRUCT_ID, pObj->lFSXObjectID, NULL, 0, sizeof(MSLPosOrientStruct), &P);
		}
	}
	if (hr != S_OK)
		return 0;
	return 1;
}

//Get the user's current state. Returns 1 if set, 0 if not set because of timeout or error with simconnect
int CFSXObjects::GetUserState(UserStateStruct *pState)
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
	MSG msg;
	do
	{
		//Update processing of FS messages -- callback happens through this function
		SimConnect_CallDispatch(m_hSimConnect, m_pfnSimconnectDispatchProc, this);
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			DispatchMessage(&msg);
		Sleep(0);
	} while (!m_bUserStateSet && (m_Time.GetTimeSeconds() - dStart) < 0.9);    

	if (!m_bUserStateSet)
		return 0;
	memcpy(pState, &m_UserState, sizeof(UserStateStruct));
	return 1;
}

//Get given FSXObjectID's above ground altitude (not including landing gear), done when we suspect it's in landing mode so we 
//can make sure not to extrapolate below ground. Don't know if this is expensive for FSX to calculate or possibly it's already cached?
//Similar to GetUserState
int CFSXObjects::GetObjectAGL(long FSXObjectID, double *pdAltAGLFt)
{
	m_bObjAGLSet = false;
	HRESULT hr = SimConnect_RequestDataOnSimObject(m_hSimConnect, REQ_OBJAGL, AGL_STRUCT_ID, FSXObjectID, SIMCONNECT_PERIOD_ONCE);
	if (FAILED(hr))
		return 0;

	//Wait for callback
	double dStart = m_Time.GetTimeSeconds();
	MSG msg;
	do
	{
		//Update processing of FS messages -- callback happens through this function
		SimConnect_CallDispatch(m_hSimConnect, m_pfnSimconnectDispatchProc, this);
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			DispatchMessage(&msg);
		Sleep(0);
	} while (!m_bObjAGLSet && (m_Time.GetTimeSeconds() - dStart) < 0.5);

	if (!m_bObjAGLSet)
		return 0;
	
	*pdAltAGLFt = m_dObjAGL;

	return 1;
	
}

//Remove the given object from our list and FSX, returns 1 if removed and 0 if not found.
int CFSXObjects::RemoveObject(const String &sCallsign)
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
int CFSXObjects::GetIndex(const String &sCallsign)
{
	int Size = m_apObjects.GetSize();
	for (int i = 0; i < Size; i++)
	{
		if (m_apObjects[i]->sCallsign == sCallsign)
			return i;
	}
	return -1;
}


////////////////////////
//Packet handlers for messages from server interface
//

//Request to connect succeeded, and server interface is up and running
int CFSXObjects::OnServerConnectSuccess(ConnectSuccessPacket *p)
{	

	return 1;
}

//Request to connect failed 
int CFSXObjects::OnServerConnectFail(ConnectFailPacket *p)
{
		
	return 1;
}

//Server has added this new object to our scene
int CFSXObjects::OnServerAddObject(AddObjectPacket *p)  
{
	
	//Find our best installed FSX model for the requested type and livery
	char cFSXModel[256];
	double dGearHeightFt;
	bool bIsJet;
	cFSXModel[0] = '*'; cFSXModel[1] = 0;  //just in case
	m_ModelResolver.GetBestModelForCallsignAndType(p->szCallsign, p->szICAOType, cFSXModel, 
		&dGearHeightFt, &bIsJet);

	//Add it to our list and into FSX
	String sFSXType = cFSXModel;
	MSLPosOrientStruct Pos;
	Pos.LatDegN = p->LatDegN;
	Pos.LonDegE = p->LonDegE;
	Pos.AltFtMSL = p->AltFtMSL;
	Pos.HdgDegTrue = p->HdgDegTrue;
	Pos.PitchDegDown = -p->PitchDegUp;
	Pos.RollDegLeft = -p->RollDegRight;
	if (AddObject(p->szCallsign, sFSXType, dGearHeightFt, bIsJet, &Pos, (long)p->GroundSpeedKts)) 
 		return 1;
	return 0;
}
//Server is sending update for this object previously added. 1 if ok, 0 if not found.
//Note we may change the passed-in packet data.
int CFSXObjects::OnServerUpdateObject(UpdateObjectPacket *pPacket)
{
	//Find it in our object list
	SimObjectStruct *p = NULL;

	for (int i = 0; !p && i < m_apObjects.GetSize(); i++)
	{
		if (m_apObjects[i]->sCallsign == pPacket->szCallsign)
			p = m_apObjects[i];
	}
	//Return if not found. We might also consider adding it ourselves if we could find the AC type somehow.
	//REVISIT
	if (!p)
	{
		m_pGUI->AddErrorMessage(L"Update received on object not in list");
		return 0;
	}

	//Update the position and orientation in the state interpolater
	p->State.UpdateState(-9999.0, pPacket->LatDegN, pPacket->LonDegE, pPacket->AltFtMSL, pPacket->HdgDegTrue,
		pPacket->PitchDegUp, pPacket->RollDegRight, pPacket->GroundSpeedKts);

	//Determine if likely landing, and if so update current ground elevation. Note we're getting the
	//last update's ground elevation compared to current update's altitude. //REVISIT
	p->bLikelyLanding = false;
	/* TODO -- enabling this causes all objects to jitter when any aircraft on final approach... 
	           could be GetObjectAGL isn't getting correct object?
	if (!p->bLikelyLanding && !p->bOnGround && pPacket->AltFtMSL < GND_ELEV_CHECK_ALTMSL &&
		((p->bIsJet && pPacket->GroundSpeedKts < 175 && p->State.GetVerticalSpeedFPM() < -500.0) ||
	     (!p->bIsJet && p->State.GetVerticalSpeedFPM() < -200.0)))
	{
		double AGL;
		if (GetObjectAGL(p->lFSXObjectID, &AGL))
		{
			p->bLikelyLanding = true;
			p->dGroundElevFt = pPacket->AltFtMSL - AGL;
			if (p->dGroundElevFt > 10000.0 || p->dGroundElevFt < -200.0)
				p->bLikelyLanding = false;
		}
		else
			p->bLikelyLanding = false;
	}
	//Still likely landing, update AGL and check if no longer likely landing
	else if (p->bLikelyLanding)
	{

		if (p->State.GetVerticalSpeedFPM() > 200.0)
			p->bLikelyLanding = false;
		else
		{
			//Update AGL altitude
			double AGL;
			GetObjectAGL(p->lFSXObjectID, &AGL);
			p->dGroundElevFt = pPacket->AltFtMSL - AGL;
			if (p->dGroundElevFt > 10000.0 || p->dGroundElevFt < -200.0)
				p->bLikelyLanding = false;
		}
	}
	*/

	//If no extended data available (gear, flaps, etc) create a state that is appropriate for speed and altitude 
	if (!pPacket->bExtendedDataValid)
	{
		double VS = p->State.GetVerticalSpeedFPM();

		//Set to on-ground if too slow to fly (i.e. must be taxiing) 
		if (pPacket->GroundSpeedKts <= ON_GROUND_SPD_KTS)
			p->bOnGround = true;
		else
			p->bOnGround = false;

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
			pPacket->bGearDown = false;
			if (p->bOnGround || (p->bLikelyLanding && p->dAltAGLFt < 1000.0))
				pPacket->bGearDown = true;
	
			//If apparently climbing...
			if ((p->bIsJet && VS > 500.0) || (!p->bIsJet && VS > 200.0))
			{
				pPacket->bGearDown = 0;
				if (p->bIsJet)
				{
					if (pPacket->GroundSpeedKts < 165)
						pPacket->FlapsDeg = 8;
					else if (pPacket->GroundSpeedKts < 200)
						pPacket->FlapsDeg = 5;
					else
						pPacket->FlapsDeg = 0;
				}
				else
				{
					if (pPacket->GroundSpeedKts < 80)
						pPacket->FlapsDeg = 8;
					else
						pPacket->FlapsDeg = 0;
				}
			}
			//Descending
			else if ((p->bIsJet && VS < -400.0) || (!p->bIsJet && VS < -200.0))
			{
				if (p->bIsJet)
				{
					if (pPacket->GroundSpeedKts < 155)
					{
						pPacket->FlapsDeg = 40;
						pPacket->bGearDown = true;
					}
					else if (pPacket->GroundSpeedKts < 165)
					{
						pPacket->FlapsDeg = 25;
						pPacket->bGearDown = true;
					}
					else if (pPacket->GroundSpeedKts < 175)
					{
						pPacket->FlapsDeg = 10;
						pPacket->bGearDown = true;
					}
					else if (pPacket->GroundSpeedKts < 200)
						pPacket->FlapsDeg = 5;
					else
						pPacket->FlapsDeg = 0;
				}
				//Props
				else
				{
					if (p->bLikelyLanding)
					{
						if (p->dAltAGLFt < 2000.0)
							pPacket->FlapsDeg = 8;
						if (p->dAltAGLFt < 1500.0)
							pPacket->bGearDown = true;
					}
					pPacket->bGearDown = true;
				}
			}
			//Else level -- leave unchanged because we don't know if in climbing or descent mode
			else 
			{
				pPacket->FlapsDeg = p->LastUpdatePacket.FlapsDeg;
				pPacket->bGearDown = p->LastUpdatePacket.bGearDown;
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
	else
		p->bOnGround = pPacket->bOnGround? true : false;

	
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
int CFSXObjects::OnServerRemoveObject(RemoveObjectPacket *p)
{
	
	String sCallsign = p->szCallsign;
	if (RemoveObject(sCallsign))
		return 1;
	return 0;
}

//Server is asking us to send user aircraft's current state
int CFSXObjects::OnServerReqUserState(ReqUserStatePacket *p)
{

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
	if (m_pSender->Send(&P))
		return 1;
	return 0;
}

//Disconnect request has succeeded and server interface has shut down
int CFSXObjects::OnServerLogoffSuccess(LogoffSuccessPacket *p)
{
	RemoveAllObjects();
	return 1;
}

//Server interface has lost connection to server and has shut down
int CFSXObjects::OnServerLostConnection(LostConnectionPacket *p)
{
	RemoveAllObjects();
	return 1;
}

////////////////////////
//Handler for messages from FSX 

//FSX has gone into normal flight mode (i.e. user's aircraft running)
void CFSXObjects::OnFSXSimRunning()
{
	return;
}

//User's aircraft has been paused either by user or FSX is doing something else (menus, reloading, etc)
void CFSXObjects::OnFSXSimStopped()
{

	return;
}

//Notification FSX is ready for the next frame (ready to begin drawing it)
void CFSXObjects::OnFSXFrame()                         
{
	UpdateAllObjects();
	return;
}

//FSX has removed given FSX object number. Hopefully this is only done in response to our
//Remove Object, so we've already deleted it from our list and all we do is return. 
//For now, double-check that's the case (in case FSX killed an object we think is still there)
//REVISIT
void CFSXObjects::OnFSXRemovedObject(DWORD FSXObjectID)
{
	int Num = m_apObjects.GetSize();
	for (int i = 0; i < Num; i++)
	{
		if (m_apObjects[i]->lFSXObjectID == (int)FSXObjectID)
		{
			WCHAR Buf[256];
			wsprintf(Buf, L"FSX killed active object: %s", m_apObjects[i]->sCallsign);
			m_pGUI->AddErrorMessage(Buf);
			return;
		}
	}
	return;
}

//FSX has visibly spawned the given FSX object number 
void CFSXObjects::OnFSXSpawnedObject(DWORD FSXObjectID)
{

	return;
}

//Requested added object has been added to FSX and assigned an FSXObject number (though not necessarily visible)
void CFSXObjects::OnFSXAddedObject(DWORD OurObjectID, DWORD FSXObjectID)
{
	for (int i = 0; i < m_apObjects.GetSize(); i++)
	{
		if (m_apObjects[i]->lOurID == (long)OurObjectID)
		{
			m_apObjects[i]->lFSXObjectID = FSXObjectID;
			
			//Remove its AI and "freeze" its position so we can control it directly
			SimConnect_AIReleaseControl(m_hSimConnect, FSXObjectID, OurObjectID);
			
			/*REVISIT -- not needed? Reduces jitter & prevents smoking wheels though, I think
			SimConnect_TransmitClientEvent(m_hSimConnect, FSXObjectID, EVENT_FREEZELAT, 1, 
					SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);	
			SimConnect_TransmitClientEvent(m_hSimConnect, FSXObjectID, EVENT_FREEZEALT, 1, 
					SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);	
			SimConnect_TransmitClientEvent(m_hSimConnect, FSXObjectID, EVENT_FREEZEATT, 1, 
					SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);	
			*/

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
void CFSXObjects::OnFSXExit()
{
	m_bQuit = true;
	return;
}

//Request for user state from FSX has been received. Set the member data and flag it's received. GetUserState() is waiting on this.
void CFSXObjects::OnFSXUserStateReceived(void *pUserStateStruct)
{
	UserStateStruct *p = (UserStateStruct *)pUserStateStruct;
	memcpy (&m_UserState, p, sizeof(UserStateStruct));
	m_bUserStateSet = true;
	return;
}

//Request for Object AGL has been received. GetObjAGL is waiting on this
void CFSXObjects::OnFSXObjectAGLReceived(void *pAGLStruct)
{
	AGLStruct *p = (AGLStruct *)pAGLStruct;
	m_dObjAGL = p->dAGLAltitude;
	m_bObjAGLSet = true;
	return;
}

int CFSXObjects::SendFSXEvent(long FSXObjectID, eFSXEvent Event, int Param)
{
	HRESULT hr = SimConnect_TransmitClientEvent(m_hSimConnect, FSXObjectID, Event, Param, SIMCONNECT_GROUP_PRIORITY_HIGHEST, 
		SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);	
	if (SUCCEEDED(hr))
		return 1;
	return 0;
}