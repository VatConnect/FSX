// VatConnect.cpp : C functions used in this DLL to talk to simconnect and Direct3D
//

#include "stdafx.h"
#include "CFSXGUI.h"
#include "CFSXObjects.h"
#include <Shlobj.h>         //For directory parsing 
#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")
#include <d3d10.h>
#pragma comment(lib, "d3d10.lib")
#pragma comment(lib, "Winmm.lib")

#pragma warning(push, 1)
#include "simconnect.h"
#pragma warning(pop)
#pragma comment(lib,"simconnect.lib")


//For Serversim proxy use these
//#define SERVER_PROXY_NAME L"ServerSim Interface.exe"  
//#define CONS_WINDOW CREATE_NEW_CONSOLE

//For actual builds to connect to VATSIM, use these
#define SERVER_PROXY_NAME L"..\\..\\..\\Vatsim Proxy\\src\\Debug\\Vatsim Proxy.exe"
#define CONS_WINDOW CREATE_NO_WINDOW

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
 
//Direct3D 9 hooking functions and data
unsigned char OrigCode9[5];          //Original code at start of D3D's Present function 
unsigned char PatchCode9[5];         //The patched code we overlay 
unsigned char* pPatchAddr9 = NULL;   //Address of the patch (start of the real Present function)
void HookIntoD3D9();
void ApplyPatch9();
void RemovePatch9();
typedef HRESULT (_stdcall *RealPresent9FuncType)(void*, const RECT*, const RECT*, HWND, const RGNDATA* );
RealPresent9FuncType RealPresent9;
HRESULT _stdcall Present9(void *pThis, const RECT* pSourceRect,const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);

//Direct3D 10 hooking functions and data
unsigned char OrigCode10[5];          //Original code at start of D3D's Present function 
unsigned char PatchCode10[5];         //The patched code we overlay 
unsigned char* pPatchAddr10 = NULL;   //Address of the patch (start of the real Present function)
void HookIntoD3D10();
void ApplyPatch10();
void RemovePatch10();
typedef HRESULT(_stdcall *RealPresent10FuncType)(void*, UINT, UINT);
RealPresent10FuncType RealPresent10;
HRESULT _stdcall Present10(void *pThis, UINT Sync, UINT Flags); 



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
						if (pPatchAddr9 == NULL)
							HookIntoD3D9();
						else
							ApplyPatch9();
						if (pPatchAddr10 == NULL)
							HookIntoD3D10();
						else
							ApplyPatch10();

						GUI.OnFSXSimRunning(); 
						Objects.OnFSXSimRunning();
					}
					else
					{
						if (pPatchAddr9)
							RemovePatch9();
						if (pPatchAddr10)
							RemovePatch10();

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
	if (pPatchAddr9)
		RemovePatch9();
	if (pPatchAddr10)
		RemovePatch10();

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
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "transponder code:1", "number", SIMCONNECT_DATATYPE_INT32);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "com active frequency:1", "number", SIMCONNECT_DATATYPE_INT32);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "general eng rpm:1", "number", SIMCONNECT_DATATYPE_INT32);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "sim on ground", "bool", SIMCONNECT_DATATYPE_INT32);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "light strobe", "bool", SIMCONNECT_DATATYPE_INT32);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "light landing", "bool", SIMCONNECT_DATATYPE_INT32);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "light taxi", "bool", SIMCONNECT_DATATYPE_INT32);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "light beacon", "bool", SIMCONNECT_DATATYPE_INT32);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "light nav", "bool", SIMCONNECT_DATATYPE_INT32);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "light logo", "bool", SIMCONNECT_DATATYPE_INT32);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "gear handle position", "bool", SIMCONNECT_DATATYPE_INT32);
	hr += SimConnect_AddToDataDefinition(hSimConnect, USER_STATE_STRUCT_ID, "flaps handle percent", "percent over 100", SIMCONNECT_DATATYPE_FLOAT64);

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

	//Initialize packet sender and receiver
	Sender.Initialize(SERVER_PROXY_LISTEN_PORT);
	Receiver.Initialize(CLIENT_LISTEN_PORT);

	//Launch server proxy -- it'll send a ServerProxyReady packet after it's initialized

	ZeroMemory(&ServerProcStartupInfo, sizeof(ServerProcStartupInfo));
	ServerProcStartupInfo.cb = sizeof(ServerProcStartupInfo);
	ServerProcStartupInfo.dwFlags = STARTF_PREVENTPINNING;
	ZeroMemory(&ServerProcInfo, sizeof(ServerProcInfo));

	if (!CreateProcess(Buffer, NULL, NULL, NULL, TRUE, CONS_WINDOW , NULL, NULL, &ServerProcStartupInfo, &ServerProcInfo))
		GUI.AddErrorMessage(STR_PROXY_LAUNCH_ERROR);
	
	//ProxyReadyPacket R; //for DEBUG where we prelaunch proxy, do this instead of createprocess and it will echo this back
	//Sender.Send(&R); 

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
void HookIntoD3D9()
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
	RealPresent9 = (RealPresent9FuncType)pVTable[17];

	//Calculate the offset from the real function to our hooked function. Constant 5 is because JMP
	//offset is from start of next instruction (5 bytes later)
	DWORD offset =(DWORD)Present9 - (DWORD)RealPresent9 - 5;  
	
	//Create the patch code: JMP assembly instruction (0xE9) followed by relative offset 
	PatchCode9[0] = 0xE9;
	*((DWORD*) &PatchCode9[1]) = offset;
	pPatchAddr9 = (unsigned char*)RealPresent9;
	
	//Set permission to allow reading/write/execute
	VirtualProtect(pPatchAddr9, 8, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	//Save out the original bytes
	for (DWORD i = 0; i < 5; i++)
		OrigCode9[i] = *(pPatchAddr9 + i);

	//Copy in our patch code
	ApplyPatch9();

	//Delete dummy D3D objects
	pI->Release();
	p->Release();

	return;
}

void HookIntoD3D10()
{
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = 100;
	swapChainDesc.BufferDesc.Height = 100;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = TRUE;

	IDXGISwapChain *pI;
	ID3D10Device *pD;

	if (FAILED(D3D10CreateDeviceAndSwapChain(NULL, 	D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
		D3D10_SDK_VERSION, &swapChainDesc, &pI, &pD)))
	{
		return;
	}
	
	//Get the pointer to the virtual table for IDXGISwapChain (pI is a pointer to a pointer to the Vtable) 
	PVOID* pVTable = (PVOID*)*((DWORD*)pI);

	//Set permissions so we can read it (18 = size of whole table)
	DWORD dwOldProtect;
	VirtualProtect(pVTable, sizeof(void *) * 18, PAGE_READWRITE, &dwOldProtect);

	//Get the address to the (real) Present function (8th function listed, starting at 0 -- see definition 
	//of IDXGISwapChain (which derives from a few other interfaces so those come before in the vtable). 
	RealPresent10 = (RealPresent10FuncType)pVTable[8];

	//Calculate the offset from the real function to our hooked function. Constant 5 is because JMP
	//offset is from start of next instruction (5 bytes later)
	DWORD offset = (DWORD)Present10 - (DWORD)RealPresent10 - 5;

	//Create the patch code: JMP assembly instruction (0xE9) followed by relative offset 
	PatchCode10[0] = 0xE9;
	*((DWORD*)&PatchCode10[1]) = offset;
	pPatchAddr10 = (unsigned char*)RealPresent10;

	//Set permission to allow reading/write/execute
	VirtualProtect(pPatchAddr10, 8, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	//Save out the original bytes
	for (DWORD i = 0; i < 5; i++)
		OrigCode10[i] = *(pPatchAddr10 + i);

	//Copy in our patch code
	ApplyPatch10();

	//Delete dummy D3D objects
	pI->Release();
	pD->Release();

	return;
}

//Our hooked function for D3D9. FSX calls this thinking it's calling IDirect3DDevice9::Present
HRESULT _stdcall Present9(void *pThis, const RECT* pSourceRect,const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
{
	IDirect3DDevice9 *pI = (IDirect3DDevice9 *)pThis;

	GUI.OnFSXPresent(pI);

	//Call the real "Present"
	RemovePatch9();
    HRESULT hr = RealPresent9(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
	ApplyPatch9();

	return hr; 
}

//Modify d3d9.dll Present function to call ours instead
void ApplyPatch9()
{
	for (DWORD i = 0; i < 5; i++)
		*(pPatchAddr9 + i) = PatchCode9[i];

	FlushInstructionCache(GetCurrentProcess(), pPatchAddr9, 5); 
	return;
}

//Restore d3d9.dll's Present function to its original code
void RemovePatch9()
{
	for (DWORD i = 0; i < 5; i++)
		*(pPatchAddr9 + i) = OrigCode9[i];

	FlushInstructionCache(GetCurrentProcess(), pPatchAddr9, 5);
	return;
}

//Same but for DirectX 10
HRESULT _stdcall Present10(void *pThis, UINT SyncInterval, UINT Flags)
{
	IDXGISwapChain* pI = (IDXGISwapChain *)pThis;
	IDXGISurface1 *pSurface = NULL;
	HRESULT hr = pI->GetBuffer(0, __uuidof(IDXGISurface1), (void**)&pSurface);
	if (pSurface)
	{
		GUI.OnFSXPresent10(pSurface);
		pSurface->Release();
	}

	/**
	ID3D10Texture2D *pBackBuffer;
	if (SUCCEEDED(pI->GetBuffer(0, __uuidof(ID3D10Texture2D),
		(LPVOID*)&pBackBuffer)))
	{

		ID3D10Device *pDevice;
		pBackBuffer->GetDevice(&pDevice);
		GUI.OnFSXPresent10(pDevice);
		pDevice->Release();
	}

	pBackBuffer->Release();
	*/
	
	//Call the real "Present"
	RemovePatch10();
	hr = RealPresent10(pThis, SyncInterval, Flags);
	ApplyPatch10();
	
	return hr;

}

//Patch jump instruction in d3d10's code
void ApplyPatch10()
{
	for (DWORD i = 0; i < 5; i++)
		*(pPatchAddr10 + i) = PatchCode10[i];

	FlushInstructionCache(GetCurrentProcess(), pPatchAddr10, 5);
	return;
}

//Restore d3d10.dll's Present function to its original code
void RemovePatch10()
{
	for (DWORD i = 0; i < 5; i++)
		*(pPatchAddr10 + i) = OrigCode10[i];

	FlushInstructionCache(GetCurrentProcess(), pPatchAddr10, 5);
	return;
}