// VatConnect.cpp : C functions used in this DLL to talk to simconnect and Direct3D
//

#include "stdafx.h"
#include "CFSXGUI.h"
#include "CFSXObjects.h"
#include <Shlobj.h>         //For directory parsing 
#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "Winmm.lib")

#pragma warning(push, 1)
#include "simconnect.h"
#pragma warning(pop)
#pragma comment(lib,"simconnect.lib")


#define SERVER_PROXY_NAME L"ServerSim Interface.exe"  
#define STR_PROXY_LAUNCH_ERROR L"\nERROR: Unable to launch server interface\n\nTry reinstalling VatConnect\n\n"
#define STR_MENU_TEXT "VatConnect"

//Objects and variables owned by this file. 
CFSXGUI				GUI;
CFSXObjects			Objects;
CPacketReceiver		Receiver;
CPacketSender		Sender;
HWND				hWnd = NULL;
HANDLE				hSimConnect = NULL;		
bool				bModulesInitialized = false;
STARTUPINFO			ServerProcStartupInfo;   //Server Proxy startup information
PROCESS_INFORMATION ServerProcInfo;
			
extern HMODULE g_hModule;       //defined in dllmain.cpp	

//Forward function declarations
void SetAddonMenuText(char *Text);
int  ProcessAllServerPackets();
int  Initialize();

//Direct3D hooking functions and data
unsigned char OrigCode[5];          //Original code at start of D3D's Present function 
unsigned char PatchCode[5];         //The patched code we overlay 
unsigned char* pPatchAddr = NULL;   //Address of the patch (start of the real Present function)
void HookIntoD3D();
void ApplyPatch();
void RemovePatch();
typedef HRESULT (_stdcall *RealPresentFuncType)(void*, const RECT*, const RECT*, HWND, const RGNDATA* );
RealPresentFuncType RealPresent;
HRESULT _stdcall Present(void *pThis, const RECT* pSourceRect,const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);

//Hidden Vatconnect window procedure (needed to process SimConnect messages)
LRESULT CALLBACK WndProc(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWindow, message, wParam, lParam);
}

//Callback from SimConnect when it has an event message for us. 
void CALLBACK SimconnectDispatch(SIMCONNECT_RECV* pData, DWORD cbData, void *pContext)
{   
	switch(pData->dwID)
    {
		case SIMCONNECT_RECV_ID_EVENT:
		{
            SIMCONNECT_RECV_EVENT *evt = (SIMCONNECT_RECV_EVENT*)pData;

            switch(evt->uEventID)
            {
               case EVENT_SIM_RUNNING:
					if (evt->dwData > 0)
					{
						//If we haven't created the hook yet, do so now
						if (pPatchAddr == NULL)
							HookIntoD3D();
						else
							ApplyPatch();
						GUI.OnFSXSimRunning(); 
						Objects.OnFSXSimRunning();
					}
					else
					{
						if (pPatchAddr)
							RemovePatch();
						GUI.OnFSXSimStopped();
						Objects.OnFSXSimStopped();
					}
					break;

			   case EVENT_ADDONMENU_SELECTED:
				   Initialize();
				   GUI.OnFSXAddonMenuSelected();
				   break;

			   case EVENT_ADDED_AIRCRAFT:
				   //Objects.OnFSXAddedObject()   //we only add object once spawned and receives FSX object ID
				   break;

			   case EVENT_REMOVED_AIRCRAFT:
				   Objects.OnFSXRemovedObject(evt->dwData);
				   break;
	        }

	        break;
        }

		case SIMCONNECT_RECV_ID_EVENT_FRAME:
			Objects.OnFSXFrame();
			GUI.OnFSXFrame();
			ProcessAllServerPackets();
			break;

		case SIMCONNECT_RECV_ID_EVENT_OBJECT_ADDREMOVE:
		{
			SIMCONNECT_RECV_EVENT_OBJECT_ADDREMOVE *evt = (SIMCONNECT_RECV_EVENT_OBJECT_ADDREMOVE*)pData;

			if (evt->uEventID == EVENT_ADDED_AIRCRAFT)
				Objects.OnFSXSpawnedObject(evt->dwData);
			else if (evt->uEventID == EVENT_REMOVED_AIRCRAFT)
				Objects.OnFSXRemovedObject(evt->dwData);

			break;
		}

		case SIMCONNECT_RECV_ID_ASSIGNED_OBJECT_ID:
		{
			SIMCONNECT_RECV_ASSIGNED_OBJECT_ID *pObjData = (SIMCONNECT_RECV_ASSIGNED_OBJECT_ID*)pData;
			Objects.OnFSXAddedObject(pObjData->dwRequestID, pObjData->dwObjectID);
			break;
		}

		case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
		{
			SIMCONNECT_RECV_SIMOBJECT_DATA *pObjData = (SIMCONNECT_RECV_SIMOBJECT_DATA*)pData;

			switch (pObjData->dwRequestID)
			{
			case REQ_USERSTATE:
				Objects.OnFSXUserStateReceived(&pObjData->dwData);
				break;
			case REQ_OBJAGL:
				Objects.OnFSXObjectAGLReceived(&pObjData->dwData);
				break;
			}
			break;
		}

		case SIMCONNECT_RECV_ID_QUIT:
		{
			Objects.OnFSXExit();
			GUI.OnFSXExit();
			break;
		}
		case SIMCONNECT_RECV_ID_EXCEPTION:
		{
			SIMCONNECT_RECV_EXCEPTION *pObjData = (SIMCONNECT_RECV_EXCEPTION*)pData;

			//Dont log "generic" error because it seems to happen a lot and is ignorable
			if (pObjData->dwException != SIMCONNECT_EXCEPTION_ERROR)
			{
				//Log("Simconnect Exception: ");
				//LogException(pObjData->dwException);
			}
			break;
		}
	}
	return;
}

//Called at first launch of FSX, during the splash screen
GUIFSX_API void DLLStart()
{
	//Create a hidden window, because SimConnect needs it to process windows messages.
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= NULL;
	wcex.hIcon			= NULL;
	wcex.hCursor		= NULL;
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= L"VatConnectClass";
	wcex.hIconSm		= NULL;
    RegisterClassEx(&wcex);
	hWnd = CreateWindow(L"VatConnectClass", L"VatConnect Window", WS_POPUP, CW_USEDEFAULT, 0, 150, 100, NULL, NULL, NULL, NULL);
	
	//Initialize Simconnect as a DLL (name must be our module name) 
	if (SUCCEEDED(SimConnect_Open(&hSimConnect, "VatConnect", NULL, 0, 0, 0)))
	{
		//Set our callback function
		SimConnect_CallDispatch(hSimConnect, SimconnectDispatch, NULL);

		//Put "Vatconnect" in the addon menu 
		SetAddonMenuText(STR_MENU_TEXT);

		//We will subscribe to system events and initialize everything only after addon menu is selected
	}
	else
	{
		//Log("Failed to connect to Simconnect")
	}

	return;
}

//Called when FSX is unloading the addons
GUIFSX_API void DLLStop()
{
	if (pPatchAddr)
		RemovePatch();
	SimConnect_Close(hSimConnect);
	bModulesInitialized = false;
	return;
}

//Initialize everything -- happens when Addon menu selected
int Initialize()
{
	if (bModulesInitialized)
		return 1;

	//Initialize modules, should come first so we can log errors (or at least cache them until graphics initialized)
	Objects.Initialize(&Sender, hSimConnect, SimconnectDispatch, &GUI);
	GUI.Initialize(&Sender);

	HRESULT hr = S_OK;

	//Note S_OK is 0, so below is quick way to check that all results were S_OK

	//Subscribe to these FSX events
	hr += SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_RUNNING, "Sim");
	hr += SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_FRAME, "Frame");
	hr += SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_ADDED_AIRCRAFT, "ObjectAdded");
	hr += SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_REMOVED_AIRCRAFT, "ObjectRemoved");

	//Define the events we send to FSX
	hr += SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_FREEZELAT, "FREEZE_LATITUDE_LONGITUDE_SET");
	hr += SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_FREEZEALT, "FREEZE_ALTITUDE_SET");
	hr += SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_FREEZEATT, "FREEZE_ATTITUDE_SET");
	hr += SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_ENGINESON, "ENGINE_AUTO_START");
	hr += SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_ENGINESOFF, "ENGINE_AUTO_SHUTDOWN");
	hr += SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_GEARSET, "GEAR_SET");
	hr += SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_FLAPSUP, "FLAPS_UP");
	hr += SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_FLAPS1, "FLAPS_1");
	hr += SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_FLAPS2, "FLAPS_2");
	hr += SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_FLAPS3, "FLAPS_3");
	hr += SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_FLAPSFULL, "FLAPS_DOWN");
	hr += SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_LANDLIGHTSSET, "LANDING_LIGHTS_SET");
	hr += SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_STROBESSET, "STROBES_SET");
	hr += SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_NAVLIGHTSTOGGLE, "TOGGLE_NAV_LIGHTS");
	hr += SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_TAXILIGHTSTOGGLE, "TOGGLE_TAXI_LIGHTS");
	hr += SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_BEACONLIGHTSTOGGLE, "TOGGLE_BEACON_LIGHTS");
	hr += SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_LOGOLIGHTSTOGGLE, "TOGGLE_LOGO_LIGHTS");
	hr += SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_TOGGLEJETWAY, "TOGGLE_JETWAY");

	//Define the object state structure we send to FSX for networked aircraft (must correspond to MSLPosOrientStruct in CFSXObjects.h)
	hr += SimConnect_AddToDataDefinition(hSimConnect, POS_MSL_STRUCT_ID, "plane latitude", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
	hr += SimConnect_AddToDataDefinition(hSimConnect, POS_MSL_STRUCT_ID, "plane longitude", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
	hr += SimConnect_AddToDataDefinition(hSimConnect, POS_MSL_STRUCT_ID, "plane altitude", "feet", SIMCONNECT_DATATYPE_FLOAT64);
	hr += SimConnect_AddToDataDefinition(hSimConnect, POS_MSL_STRUCT_ID, "plane pitch degrees", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
	hr += SimConnect_AddToDataDefinition(hSimConnect, POS_MSL_STRUCT_ID, "plane heading degrees true", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
	hr += SimConnect_AddToDataDefinition(hSimConnect, POS_MSL_STRUCT_ID, "plane bank degrees", "degrees", SIMCONNECT_DATATYPE_FLOAT64);

	//AGL object state we send to FSX (correspond to AGLPosOrientStruct which must correspond to MSLPosOrientStruct too)
	hr += SimConnect_AddToDataDefinition(hSimConnect, POS_AGL_STRUCT_ID, "plane latitude", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
	hr += SimConnect_AddToDataDefinition(hSimConnect, POS_AGL_STRUCT_ID, "plane longitude", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
	hr += SimConnect_AddToDataDefinition(hSimConnect, POS_AGL_STRUCT_ID, "plane alt above ground", "feet", SIMCONNECT_DATATYPE_FLOAT64);
	hr += SimConnect_AddToDataDefinition(hSimConnect, POS_AGL_STRUCT_ID, "plane pitch degrees", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
	hr += SimConnect_AddToDataDefinition(hSimConnect, POS_AGL_STRUCT_ID, "plane heading degrees true", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
	hr += SimConnect_AddToDataDefinition(hSimConnect, POS_AGL_STRUCT_ID, "plane bank degrees", "degrees", SIMCONNECT_DATATYPE_FLOAT64);

	//Define the user aircraft state structure we get from FSX (must correspond to UserStateStruct)
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "plane latitude", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "plane longitude", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "plane altitude", "feet", SIMCONNECT_DATATYPE_FLOAT64);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "plane pitch degrees", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "plane heading degrees true", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "plane bank degrees", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "ground velocity", "knots", SIMCONNECT_DATATYPE_FLOAT64);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "light strobe", "bool", SIMCONNECT_DATATYPE_INT32);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "light landing", "bool", SIMCONNECT_DATATYPE_INT32);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "light taxi", "bool", SIMCONNECT_DATATYPE_INT32);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "light beacon", "bool", SIMCONNECT_DATATYPE_INT32);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "light nav", "bool", SIMCONNECT_DATATYPE_INT32);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "light logo", "bool", SIMCONNECT_DATATYPE_INT32);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "gear handle position", "bool", SIMCONNECT_DATATYPE_INT32);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "trailing edge flaps left percent", "percent", SIMCONNECT_DATATYPE_FLOAT64);

	//Define the AGL altitude structure we ask FSX to get approx ground height under a networked aircraft (AGLStruct)
	hr += SimConnect_AddToDataDefinition(hSimConnect, AGL_STRUCT_ID, "plane alt above ground", "feet", SIMCONNECT_DATATYPE_FLOAT64);

	if (hr != S_OK)
	{
		//Log("One or more initializers to SimConnect failed");
	}

	//Determine this DLL's full path (get full path & name and back up to first backslash)
	//We assume server proxy is located in same directory.
	WCHAR Buffer[MAX_PATH] = { 0 };
	GetModuleFileName(g_hModule, Buffer, MAX_PATH);
	int Index = wcslen(Buffer);
	if (Index < (int)(MAX_PATH - wcslen(SERVER_PROXY_NAME) - 2))
	{
		while (Index >= 0 && Buffer[Index] != '\\')
			Index--;
		if (Buffer[Index] == '\\')
			Index++;
		Buffer[Index] = 0;
		GUI.SetModulePath(Buffer);

		//Tack on server proxy process name
		wcscpy_s(&Buffer[Index], (MAX_PATH - Index), SERVER_PROXY_NAME);
	}
	else
	{
		OutputDebugString(L"Server proxy name and path too long!\n");
		wcscpy_s(Buffer, MAX_PATH, SERVER_PROXY_NAME); //try in current directory, might as well..
	}

	//Launch server proxy -- it'll send a ServerProxyReady packet after it's initialized
	ZeroMemory(&ServerProcStartupInfo, sizeof(ServerProcStartupInfo));
	ServerProcStartupInfo.cb = sizeof(ServerProcStartupInfo);
	ServerProcStartupInfo.dwFlags = STARTF_PREVENTPINNING;
	ZeroMemory(&ServerProcInfo, sizeof(ServerProcInfo));

	//DEBUG CREATE_NEW_CONSOLE to put up console window for ServerSim; use CREATE_NO_WINDOW for final version
	if (!CreateProcess(Buffer, NULL, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &ServerProcStartupInfo, &ServerProcInfo))
		GUI.AddErrorMessage(STR_PROXY_LAUNCH_ERROR);

	//Initialize packet sender and receiver
	Sender.Initialize(SERVER_PROXY_LISTEN_PORT);
	Receiver.Initialize(CLIENT_LISTEN_PORT);

	bModulesInitialized = true;
	return 1;
}

//Check for inbound packets from server
int ProcessAllServerPackets()
{
	static char PacketBuffer[LARGEST_PACKET_SIZE];

	while (Receiver.GetNextPacket(&PacketBuffer))
	{
		//We don't care about return values, let both modules process it or not
		Objects.ProcessPacket(&PacketBuffer);
		GUI.ProcessPacket(&PacketBuffer);
	} 
	
	return 1;
}

//Set the addon menu text (called initially by us, then from GUI if GUI closed)
void SetAddonMenuText (char *Text)
{
	static bool bMenuHasText = false;

	if (bMenuHasText)
		SimConnect_MenuDeleteItem(hSimConnect, EVENT_ADDONMENU_SELECTED);
	else
		bMenuHasText = true;
	SimConnect_MenuAddItem(hSimConnect, Text, EVENT_ADDONMENU_SELECTED, 0);
	return;
}


//This function finds the address to the d3d9.dll Present function, which we know FSX has already 
//loaded into this process, and patches the code with a JMP to our Present function. In our Present 
//function we first call GUI.OnFSXPresent(), then we undo the patch with the original code, call 
//the real d3d Present function, and reinstate the patch. Note this needs to be in the same process
//as FSX (to get the same d3d9.dll instance FSX is using), which is why this is a DLL addon and not
//an EXE. 
void HookIntoD3D()
{
	//Create a temporary direct 3D object and get its IDirect3DDevice9 interface. This is a different 
	//"instance," but the virtual table will point to the function within d3d9.dll also used by FSX's 
	//instance.
	LPDIRECT3D9 p = Direct3DCreate9(D3D_SDK_VERSION);
	if (!p)
		return;
	D3DPRESENT_PARAMETERS presParams;
	ZeroMemory(&presParams,sizeof(presParams));
	presParams.Windowed = TRUE;
	presParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
	IDirect3DDevice9 *pI = NULL;
	HRESULT hr;
	hr = p->CreateDevice( D3DADAPTER_DEFAULT,  D3DDEVTYPE_NULLREF, hWnd, 
		D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &presParams, &pI);
	if (!pI)
	{
		p->Release();
		return;
	}

	//Get the pointer to the virtual table for IDirect3DDevice9 (pI is a pointer to a pointer to the Vtable) 
	PVOID* pVTable = (PVOID*)*((DWORD*)pI); 
	
	//Set permissions so we can read it (117 = size of whole table)
	DWORD dwOldProtect;
	VirtualProtect(pVTable, sizeof(void *) * 117, PAGE_READWRITE, &dwOldProtect);
    
	//Get the address to the (real) Present function (17th function listed, starting at 0 -- see definition 
	//of IDirect3DDevice9). 
	RealPresent = (RealPresentFuncType)pVTable[17];

	//Calculate the offset from the real function to our hooked function. Constant 5 is because JMP
	//offset is from start of next instruction (5 bytes later)
	DWORD offset =(DWORD)Present - (DWORD)RealPresent - 5;  
	
	//Create the patch code: JMP assembly instruction (0xE9) followed by relative offset 
	PatchCode[0] = 0xE9;
	*((DWORD*) &PatchCode[1]) = offset;
	pPatchAddr = (unsigned char*)RealPresent;
	
	//Set permission to allow reading/write/execute
	VirtualProtect(pPatchAddr, 8, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	//Save out the original bytes
	for (DWORD i = 0; i < 5; i++)
		OrigCode[i] = *(pPatchAddr + i);

	//Copy in our patch code
	ApplyPatch();

	//Delete dummy D3D objects
	pI->Release();
	p->Release();

	return;
}

//Our hooked function. FSX calls this thinking it's calling IDirect3DDevice9::Present
HRESULT _stdcall Present(void *pThis, const RECT* pSourceRect,const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
{
	IDirect3DDevice9 *pI = (IDirect3DDevice9 *)pThis;

	GUI.OnFSXPresent(pI);

	//Call the real "Present"
	RemovePatch();
    HRESULT hr = RealPresent(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
	ApplyPatch();

	return hr; 
}

//Modify d3d9.dll Present function to call ours instead
void ApplyPatch()
{
	for (DWORD i = 0; i < 5; i++)
		*(pPatchAddr + i) = PatchCode[i];

	FlushInstructionCache(GetCurrentProcess(), pPatchAddr, 5); 
	return;
}

//Restore d3d9.dll's Present function to its original code
void RemovePatch()
{
	for (DWORD i = 0; i < 5; i++)
		*(pPatchAddr + i) = OrigCode[i];

	FlushInstructionCache(GetCurrentProcess(), pPatchAddr, 5);
	return;
}
