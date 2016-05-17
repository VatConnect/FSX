// GUI-FSX.cpp : C functions used in this DLL to talk to simconnect and Direct3D
//

#include "stdafx.h"
#include "GUI-FSX.h"
#include "CFSXGUI.h"

#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")

#pragma warning(push, 1)
#include "simconnect.h"
#pragma warning(pop)

#pragma comment(lib,"simconnect.lib")

CFSXGUI g_GUI;
HWND	g_hWnd = NULL;
HANDLE  g_hSimConnect = NULL;

void SetAddonMenuText(char *Text);

//Direct3D hooking functions and data
unsigned char g_OrigCode[5];          //Original code at start of D3D's Present function 
unsigned char g_PatchCode[5];         //The patched code we overlay 
unsigned char* g_pPatchAddr = NULL;   //Address of the patch (start of the real Present function)
void HookIntoD3D();
void ApplyPatch();
void RemovePatch();

typedef HRESULT (_stdcall *RealPresentFuncType)(void*, const RECT*, const RECT*, HWND, const RGNDATA* );
RealPresentFuncType RealPresent;
HRESULT _stdcall Present(void *pThis, const RECT* pSourceRect,const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);

//Enums for Simconnect callbacks
typedef enum eEvents
{
	EVENT_SIM_RUNNING,
	EVENT_ADDONMENU_SELECTED
} eEvents;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, message, wParam, lParam);
}


//Callback from SimConnect when it has an event message for us. pContext is pointer we passed in
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
						if (g_pPatchAddr == NULL)
							HookIntoD3D();
						else
							ApplyPatch();
						g_GUI.OnFSXSimRunning(); 
					}
					else
					{
						if (g_pPatchAddr)
							RemovePatch();
						g_GUI.OnFSXSimStopped();
					}
					break;
			   case EVENT_ADDONMENU_SELECTED:
				   g_GUI.OnFSXAddonMenuSelected();
				   break;
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
	g_hWnd = CreateWindow(L"VatConnectClass", L"VatConnect GUI Window", WS_POPUP, CW_USEDEFAULT, 0, 150, 100, NULL, NULL, NULL, NULL);
	
	//Initialize Simconnect as a DLL (name must be our module name)
	SimConnect_Open(&g_hSimConnect, "GUI-FSX", NULL, 0, NULL, 0);
    SimConnect_SubscribeToSystemEvent(g_hSimConnect, EVENT_SIM_RUNNING, "Sim");
	SimConnect_CallDispatch(g_hSimConnect, SimconnectDispatch, &g_GUI);

	//Initialize our main app, must come after SimConnect initialization because it calls back to set the menu text
	g_GUI.Initialize();

	return;
}

//Called when FSX is unloading the addons
GUIFSX_API void DLLStop()
{
	if (g_pPatchAddr)
		RemovePatch();
	g_GUI.Shutdown();
	SimConnect_Close(g_hSimConnect);
	return;
}

//Set the addon menu text 
void SetAddonMenuText (char *Text)
{
	static bool bMenuHasText = false;

	if (bMenuHasText)
		SimConnect_MenuDeleteItem(g_hSimConnect, EVENT_ADDONMENU_SELECTED);
	else
		bMenuHasText = true;
	SimConnect_MenuAddItem(g_hSimConnect, Text, EVENT_ADDONMENU_SELECTED, 0);
	return;
}

//This function finds the address to the d3d9.dll Present function, which we know FSX has already 
//loaded into this process, and patches the code with a JMP to our Present function. In our Present 
//function we first call g_GUI.OnFSXPresent(), then we undo the patch with the original code, call 
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
	hr = p->CreateDevice( D3DADAPTER_DEFAULT,  D3DDEVTYPE_NULLREF, g_hWnd, 
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
	g_PatchCode[0] = 0xE9;
	*((DWORD*) &g_PatchCode[1]) = offset;
	g_pPatchAddr = (unsigned char*)RealPresent;
	
	//Set permission to allow reading/write/execute
	VirtualProtect(g_pPatchAddr, 8, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	//Save out the original bytes
	for (DWORD i = 0; i < 5; i++)
		g_OrigCode[i] = *(g_pPatchAddr + i);

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

	//Call our main class
	g_GUI.OnFSXPresent(pI);

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
		*(g_pPatchAddr + i) = g_PatchCode[i];

	FlushInstructionCache(GetCurrentProcess(), g_pPatchAddr, 5); 
	return;
}

//Restore d3d9.dll's Present function to its original code
void RemovePatch()
{
	for (DWORD i = 0; i < 5; i++)
		*(g_pPatchAddr + i) = g_OrigCode[i];

	FlushInstructionCache(GetCurrentProcess(), g_pPatchAddr, 5);
	return;
}
