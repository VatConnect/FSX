//This EXE simulates the Vatsim server interface,
//to create test data for development. 

#include "stdafx.h"

#include "Packets.h"
#include "CPacketSender.h"
#include "CPacketReceiver.h"
#include "CTestAircraft.h"

//Globals
CPacketSender g_Sender;
CPacketReceiver g_Receiver;
bool g_bQuit = false;
bool g_bUserSentFirstUpdate = false; //flag so when we receive first user update, we spawn all aircraft
bool g_bUserConnected = false;
double g_dNextRadioSendTime;
CTime g_Time;


#define NUM_AIRCRAFT 12  
CTestAircraft g_Aircraft[NUM_AIRCRAFT];

//Forward declarations
void ProcessPacket(void *pPacket);
void SpawnAllAircraft(UserStateUpdatePacket *p);
void SendATC();
void SendAddControllerPacket(char *FacName, char *ControllerName, char *Freq, double dLatN,
	double dLonE, char *ATIS);
void SendServers();
void SendLoginInfo();
void SendAddServerPacket(char *ServerName, char *ServerLocation);
void SendRandomRadioMessage();


int _tmain(int argc, _TCHAR* argv[])
{
	printf("Server Simulator\n");

	//Initialize the packet sender and receiver
	g_Sender.Initialize(CLIENT_LISTEN_PORT);
	g_Receiver.Initialize(SERVER_PROXY_LISTEN_PORT);

	g_bQuit = false;
	g_bUserSentFirstUpdate = false;
	g_bUserConnected = false;

	//Indicate we're ready and listening
	ProxyReadyPacket P;
	g_Sender.Send(&P);

	SendLoginInfo();
	printf("Sent Login Info\n");

	SendServers();
	printf("Send server list\n");

	printf("Waiting for messages...\n");

	//Main loop
	char PacketBuffer[LARGEST_PACKET_SIZE];
	while (!g_bQuit)
	{
		//Process any incoming packets
		while (g_Receiver.GetNextPacket(&PacketBuffer))
			ProcessPacket(&PacketBuffer);

		//If user has sent their initial position, it means our aircraft are spawned and sim is running
		if (g_bUserConnected && g_bUserSentFirstUpdate)
		{
			for (int i = 0; i < NUM_AIRCRAFT; i++)
				g_Aircraft[i].Update();

			if (g_Time.GetTimeSeconds() >= g_dNextRadioSendTime)
			{
				SendRandomRadioMessage();
				g_dNextRadioSendTime = g_Time.GetTimeSeconds() + 8.0;
			}
			Sleep(16);
		}
		//Otherwise we're just waiting for connection... no hurry
		else
			Sleep(500);
	}

	for (int i = 0; i < NUM_AIRCRAFT; i++)
		g_Aircraft[i].Stop();

	g_Sender.Shutdown();
	g_Receiver.Shutdown();


	ExitProcess(0);
	
}

void ProcessPacket(void *pPacket)
{
	

	switch (((PacketHeader *)pPacket)->Type)
	{
	case REQ_CONNECTION_PACKET:
		//Send connect success
		if (!g_bUserConnected)
		{
			ConnectSuccessPacket P;
			ReqUserStatePacket R;

			strcpy_s(P.szMessage, "Welcome to the test server. You will see aircraft spawned around you. Press disconnect to log off\n\n");
			g_Sender.Send(&P);
			printf("REQ CONNECTION received; sent CONNECT_SUCCESS\n");

			//Send request user state. We will spawn the test aircraft around it when we receive it		
			g_Sender.Send(&R);
			g_bUserConnected = true;
			g_dNextRadioSendTime = g_Time.GetTimeSeconds() + 3.0;

		}
		break;

	case USER_STATE_UPDATE_PACKET:

		printf("Received user state\n");

		if (!g_bUserSentFirstUpdate)
		{
			//SpawnAllAircraft((UserStateUpdatePacket *)pPacket);
			SendATC();
			printf("Spawned aircraft and sent ATC list\n");
			g_bUserSentFirstUpdate = true;
		}
		break;

	case TRANSMIT_KEYDOWN_PACKET:
		break;

	case TRANSMIT_KEYUP_PACKET:
		break;

	case REQ_DISCONNECT_PACKET:
	{
		//Tell client success
		printf("Received REQ_DISCONNECT, sending LOGOFF_SUCCESS.\n");
		LogoffSuccessPacket LSPack;
		strcpy_s(LSPack.szMessage, "Goodbye!\n");
		g_Sender.Send(&LSPack);
		g_bUserConnected = false;

		break;
	}

	case SHUTDOWN_PACKET:
		g_bQuit = 1;
		break;

	case FLIGHT_PLAN_PACKET:
	{
		FlightPlanPacket *pFP = (FlightPlanPacket *)pPacket;
		printf("Received flight plan: %s %s/%s %s P%s %s %s RMK:%s ETE:%s\n", pFP->szCallsign,
			pFP->szACType, pFP->szACEquip, pFP->szTAS, pFP->szDepTime, pFP->szAltitude,
			pFP->szRoute, pFP->szRemarks, pFP->szETE);
		ProxyMessagePacket PM;
		strcpy_s(PM.szProxyMessage, "Flight plan sent.\n");
		g_Sender.Send(&PM);
		break;
	}

	case REQ_METAR_PACKET:
	{
		ReqMetarPacket *pR = (ReqMetarPacket *)pPacket;
		MetarPacket P;
		printf("Received WX request for %s\n", pR->szStationName);
		sprintf_s(P.szMetar, 256, "%s 022112 22012G20 20/12 -RA BR SCT20 BKN40 BKN70 OVC80 RMK LTCGCC DIST E", pR->szStationName);
		g_Sender.Send(&P);
		break;
	}

	case TEXT_MESSAGE_PACKET:
	{
		printf("Received radio message: %s\n", ((TextMessagePacket *)pPacket)->szMessage);
		break;
	}

	}
	return;
}

//Create debug aircraft
void SpawnAllAircraft(UserStateUpdatePacket *p)
{
	g_Aircraft[0].Initialize("TUP_0_TRUTH", "B738", p->LatDegN, p->LonDegE, p->HdgDegTrue, p->AltFtMSL, TAXI, NO_LAG, 0.03, &g_Sender, 0);
	g_Aircraft[1].Initialize("TUP_1_TYPLAG", "B738", p->LatDegN, p->LonDegE, p->HdgDegTrue, p->AltFtMSL, TAXI, TYPICAL_LAG, 1.0, &g_Sender, 1);
	g_Aircraft[2].Initialize("TUP_1_HILAG", "B738", p->LatDegN, p->LonDegE, p->HdgDegTrue, p->AltFtMSL, TAXI, HIGH_LAG, 1.0, &g_Sender, 2);
	g_Aircraft[3].Initialize("TUP_5_HILAG", "B738", p->LatDegN, p->LonDegE, p->HdgDegTrue, p->AltFtMSL, TAXI, HIGH_LAG, 5.0, &g_Sender, 3);
	g_Aircraft[4].Initialize("UP_0_TRUTH", "B738", p->LatDegN, p->LonDegE, p->HdgDegTrue, p->AltFtMSL, TOUCH_AND_GO, NO_LAG, 0.03, &g_Sender, 0);
	g_Aircraft[5].Initialize("UP_1_TYPLAG", "B738", p->LatDegN, p->LonDegE, p->HdgDegTrue, p->AltFtMSL, TOUCH_AND_GO, TYPICAL_LAG, 1.0, &g_Sender, 1);
	g_Aircraft[6].Initialize("UP_5_HILAG", "B738", p->LatDegN, p->LonDegE, p->HdgDegTrue, p->AltFtMSL, TOUCH_AND_GO, HIGH_LAG, 1.0, &g_Sender, 2);

	return;
}

//Send sample ATC (and remove 1)
void SendATC()
{
	//25 controllers sent then one removed
	SendAddControllerPacket("ZLA_A_CTR", "John Doe (S1)", "122.1", 35.0, -118.0, "Attention this is the ATIS");
	SendAddControllerPacket("ZNY_B_CTR", "Jane Smith (C1)", "122.2", 34.0, -118.0, "");
	SendAddControllerPacket("ZLA_A_CTR", "John Doe (S1)", "123.15", 34.0, -118.0, "This is KLAX approach. This is a long string to see if the text will properly wrap around. Also to see if a large string works.");
	SendAddControllerPacket("ZLA_A_CTR", "John Doe (S1)", "122.1", 35.0, -118.0, "Attention this is the ATIS");
	SendAddControllerPacket("ZNY_B_CTR", "Jane Smith (C1)", "122.2", 34.0, -118.0, "");
	SendAddControllerPacket("ZLA_A_CTR", "John Doe (S1)", "123.15", 34.0, -118.0, "This is KLAX approach. This is a long string to see if the text will properly wrap around. Also to see if a large string works.");
	SendAddControllerPacket("ZLA_A_APP", "John Doe (S1)", "122.1", 35.0, -118.0, "Attention this is the ATIS");
	SendAddControllerPacket("ZNY_B_APP", "Jane Smith (C1)", "122.2", 34.0, -118.0, "");
	SendAddControllerPacket("ZLA_R_CTR", "Remove Me (C2)", "123.4", 34.0, -118.0, "This controller was removed and should not be visible");
	SendAddControllerPacket("SOCAL_A_APP", "John Doe (S1)", "123.15", 34.0, -118.0, "This is KLAX approach. This is a long string to see if the text will properly wrap around. Also to see if a large string works.");
	SendAddControllerPacket("SOCAL_A_DEP", "John Doe (S1)", "122.1", 35.0, -118.0, "Attention this is the ATIS");
	SendAddControllerPacket("NY_B_DEP", "Jane Smith (C1)", "122.2", 34.0, -118.0, "");
	SendAddControllerPacket("SOCAL_A_DEP", "John Doe (S1)", "123.15", 34.0, -118.0, "This is KLAX approach. This is a long string to see if the text will properly wrap around. Also to see if a large string works.");
	SendAddControllerPacket("LAX_A_TWR", "John Doe (S1)", "122.1", 35.0, -118.0, "Attention this is the ATIS");
	SendAddControllerPacket("JFK_B_TWR", "Jane Smith (C1)", "122.2", 34.0, -118.0, "");
	SendAddControllerPacket("LAX_A_TWR", "John Doe (S1)", "123.15", 34.0, -118.0, "This is KLAX approach. This is a long string to see if the text will properly wrap around. Also to see if a large string works.");
	SendAddControllerPacket("LAX_A_GND", "John Doe (S1)", "122.1", 35.0, -118.0, "Attention this is the ATIS");
	SendAddControllerPacket("JFK_B_GND", "Jane Smith (C1)", "122.2", 34.0, -118.0, "");
	SendAddControllerPacket("LAX_A_GND", "John Doe (S1)", "123.15", 34.0, -118.0, "This is KLAX approach. This is a long string to see if the text will properly wrap around. Also to see if a large string works.");
	SendAddControllerPacket("LAX_A_CLR", "John Doe (S1)", "122.1", 35.0, -118.0, "Attention this is the ATIS");
	SendAddControllerPacket("LGA_B_CLR", "Jane Smith (C1)", "122.2", 34.0, -118.0, "");
	SendAddControllerPacket("ZLA_A_CTR", "John Doe (S1)", "123.15", 34.0, -110.0, "This is KLAX Center.\n This should be a new line and is a long string to see if the text will properly wrap around.\n\nShould be extra space with above line.\nNew Line\nNewLine2\nNew Line3 and this is a really big line because I want it to go to the end of the page which is over 40 columns long and I don't know how many\nNewLine4");
	SendAddControllerPacket("SOCAL_A_APP", "John Doe (S1)", "122.1", 35.0, -118.0, "Attention this is the ATIS");
	SendAddControllerPacket("SOCAL_A_APP", "John Doe (S1)", "123.15", 34.0, -118.0, "This is KLAX approach. This is a long string to see if the text will properly wrap around. Also to see if a large string works.");
	SendAddControllerPacket("JFK_B_OBS", "Jane Smith (C1)", "122.2", 34.0, -118.0, "");

	RemoveControllerPacket P;
	strcpy_s(P.szPosName, "ZLA_R_CTR");
	g_Sender.Send(&P);

	return;

}

//Send the saved login information (server proxy does this because it's encrypted)
void SendLoginInfo()
{
	LoginInfoPacket L;
	strcpy_s(L.szServerName, "Test Server East");
	strcpy_s(L.szUserName, "Saved User Name -- KBWI");
	strcpy_s(L.szUserID, "123456");
	strcpy_s(L.szPassword, "9876543");
	strcpy_s(L.szCallsign, "DAL724");
	strcpy_s(L.szACType, "B738");
	strcpy_s(L.szACEquip, "I");
	g_Sender.Send(&L);
	return;
}

//Fill and send AddController packet
void SendAddControllerPacket(char *FacName, char *ControllerName, char *Freq, double dLatN,
	double dLonE, char *ATIS)
{
	static AddControllerPacket P;
	strcpy_s(P.szPosName, FacName);
	strcpy_s(P.szControllerNameRating, ControllerName);
	strcpy_s(P.szFreq, Freq);
	P.dLatDegN = dLatN; 
	P.dLonDegE = dLonE;
	strcpy_s(P.szMessage, ATIS);
	g_Sender.Send(&P);
	
		
	return;
}

//Send sample server list
void SendServers()
{
	SendAddServerPacket("VAT_W", "Los Angeles, USA");
	SendAddServerPacket("VAT_N", "Seattle, USA");
	SendAddServerPacket("CANADA", "Vancouver, CA");
	SendAddServerPacket("VAT_E", "New Jersey, USA");
	SendAddServerPacket("AUSTRALIA", "Australia");
	SendAddServerPacket("GERMANY1", "Germany");
	SendAddServerPacket("UK1", "London, UK");
	SendAddServerPacket("VAT_W", "Los Angeles, USA");
	SendAddServerPacket("VAT_N", "Seattle, USA");
	SendAddServerPacket("CANADA", "Vancouver, CA");
	SendAddServerPacket("VAT_E", "New Jersey, USA");
	SendAddServerPacket("AUSTRALIA", "Australia");
	SendAddServerPacket("GERMANY1", "Germany");
	SendAddServerPacket("UK1", "London, UK");
	SendAddServerPacket("VAT_W", "Los Angeles, USA");
	SendAddServerPacket("VAT_N", "Seattle, USA");
	return;
}

//Fill and send AddServer packet
void SendAddServerPacket(char *ServerName, char *ServerLocation)
{
	static AddServerPacket P;
	strcpy_s(P.szServerName, ServerName);
	strcpy_s(P.szServerLocation, ServerLocation);
	g_Sender.Send(&P);
	return;
}

//Send "random" radio message
void SendRandomRadioMessage()
{
	static TextMessagePacket P;
	int Num = rand() % 1000;
	int Blahs = rand() % 5;
	switch (Blahs)
	{
	case 0: sprintf_s(P.szMessage, "AAL%d: With you flight level 330", Num); break;
	case 1: sprintf_s(P.szMessage, "SWA%d: Cleared visual runway 1", Num); break;
	case 2: sprintf_s(P.szMessage, "DAL%d: Left heading 320", Num); break;
	case 3: sprintf_s(P.szMessage, "BAW%d: Descend via CRESO3 except cross JOKUR at or below FL230, 280kts or less", Num); break;
	default: sprintf_s(P.szMessage, "LAX_B_CTR: Roger"); break;
	}
	g_Sender.Send(&P);
	return;
}