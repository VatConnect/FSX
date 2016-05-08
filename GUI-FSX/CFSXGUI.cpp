#include "stdafx.h"
#include "CFSXGUI.h"

extern HMODULE g_hModule;
extern HWND g_hWnd; 
extern void SetAddonMenuText(char *Text);

#define MENU_TEXT "VatConnect"
#define DLG_UPDATE_HZ 4     //update rate for dialog->Update

const DWORD KEY_REPEAT = 1 << 30;
const DWORD ALT_PRESSED = 1 << 29;

/////////////////
//Hooked windows procedure
CFSXGUI *g_pGUI = NULL;
LRESULT CALLBACK FSXWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return g_pGUI->ProcessFSXWindowMessage(hWnd, message, wParam, lParam);
}

//////////////////
//CFSXGUI

CFSXGUI::CFSXGUI() : m_bRunning(false), m_bGraphicsInitialized(false), m_FSXWindowProc(NULL), 
	m_bNeedMouseMove(false), m_bNeedKeyboard(false), m_bCheckForNewDevices(false),
	m_bInWindowedMode(true), m_pFullscreenPrimaryDevice(NULL), m_dwNextDlgUpdateTime(0)
{
	g_pGUI = this;
	m_WindowedDeviceDesc.hWnd = NULL;
	m_WindowedDeviceDesc.pDevice = NULL;
}

CFSXGUI::~CFSXGUI()
{
}

void CFSXGUI::Initialize()
{
	SetAddonMenuText(MENU_TEXT);

	//Load preferences
	//TODO
	strcpy_s(m_Prefs.LoginID, 32, "9999999");
	strcpy_s(m_Prefs.Password, 32, "123456");
	strcpy_s(m_Prefs.Callsign, 32, "DAL724");
	strcpy_s(m_Prefs.ICAOType, 32, "B738/Q");
	m_Prefs.PTTVKey = VK_CAPITAL;
	m_Prefs.PTTJoyButton = 1;

	return;
}

//Called when FSX unloading the addons, unknown results if this is called 
//then initialized again later
void CFSXGUI::Shutdown()
{
	//DisconnectFromVATSIM();

	for (size_t i = 0; i < m_apDialogs.size(); i++)
		m_apDialogs[i]->Shutdown();
	
	m_Graphics.Shutdown();

	//Restore FSX Windows procedure chain back to original
	if (m_FSXWindowProc)
	{
		SetWindowLongPtr(m_hFSXWindow, GWLP_WNDPROC, m_FSXWindowProc);
		m_FSXWindowProc = NULL;
	}
	m_apDialogs.empty();
	m_apOpenDialogs.empty();

	m_bRunning = false;
	m_bGraphicsInitialized = false;

	return;
}

//FSX has finished drawing to given device... add any overlays on that device 
void CFSXGUI::OnFSXPresent(IDirect3DDevice9 *pI)
{
	
	if (!m_bRunning)
		return;
		
	if (!m_bGraphicsInitialized)
		InitGraphics(pI);

	//See if we've switched from windowed to full-screen, or back
	if (!m_bCheckForNewDevices)
	{
		if (m_bInWindowedMode)
		{
			if (pI != m_WindowedDeviceDesc.pDevice)
			{
				m_bCheckForNewDevices = true;
				m_dwCheckNewDevicesEndTime = GetTickCount() + CHECK_NEW_DEVICES_INTERVAL_MS;
			}
		}
		//Fullscreen mode, check if one of the known fullscreen devices 
		else
		{
			bool bFound = false;
			for (size_t i = 0; !bFound && i < m_aFullscreenDevices.size(); i++)
				if (m_aFullscreenDevices[i].pDevice == pI)
					bFound = true;
			if (!bFound)
			{
				m_bCheckForNewDevices = true;
				m_dwCheckNewDevicesEndTime = GetTickCount() + CHECK_NEW_DEVICES_INTERVAL_MS;
			}
		}
	}

	//Continue scan for new devices
	if (m_bCheckForNewDevices)
	{
		if (GetTickCount() < m_dwCheckNewDevicesEndTime)
			CheckIfNewDevice(pI);
		else
			m_bCheckForNewDevices = false;
	}

	if (pI != m_WindowedDeviceDesc.pDevice && pI != m_pFullscreenPrimaryDevice)
		CheckIfNewDevice(pI);

	//Update dialogs at DLG_UPDATE_HZ
	if (GetTickCount() >= m_dwNextDlgUpdateTime)
	{
		for (size_t i = 0; i < m_apOpenDialogs.size(); i++)
			m_apDialogs[i]->Update();
		m_dwNextDlgUpdateTime += (1000 / DLG_UPDATE_HZ);   //time is in milliseconds
	}


	//If windowed, or fullscreen primary device, tell each open dialog to draw
	if ((m_bInWindowedMode && pI == m_WindowedDeviceDesc.pDevice) ||
		(!m_bInWindowedMode && pI == m_pFullscreenPrimaryDevice))
	{
		for (size_t i = 0; i < m_apOpenDialogs.size(); i++)
			m_apDialogs[i]->Draw(pI);
	}

	return;
}

//FSX has gone into flight mode
void CFSXGUI::OnFSXSimRunning()
{
	return;
}

//FSX has gone out of flight mode (e.g. to options, or exited flight)
void CFSXGUI::OnFSXSimStopped()
{
	return;
}

//User has selected FSX addon menu ("VatConnect...")
void CFSXGUI::OnFSXAddonMenuSelected()
{
	if (m_bRunning)
		return;

	m_bRunning = true;
	m_dlgMain.Open();
	if (!m_bGraphicsInitialized)
	{
		m_apDialogs.push_back(&m_dlgMain);
		m_apOpenDialogs.push_back(&m_dlgMain);
	}

	return;
} 

//Handle FSX's windows messages before FSX does (to intercept mouse and keyboard). Return code
//depends on the message.
LRESULT CFSXGUI::ProcessFSXWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	bool bHandled = false;
	int RetCode = 0;

	if (m_bRunning && message < WM_USER)
	{
		if (message == WM_KEYDOWN || message == WM_KEYUP)
		{
			//Check if it's keyboard Push-to-talk (other keypresses we handle in WM_CHAR)
			if (m_Prefs.PTTVKey && (long)wParam == m_Prefs.PTTVKey)
			{
				//Only process first KEYDOWN	
				if (message == WM_KEYDOWN && !(lParam & KEY_REPEAT))
					OnPTTButton(true);
				else if (message == WM_KEYUP)
					OnPTTButton(false);
				bHandled = true;
			} 

			//If we have keyboard focus, return that this was handled and we'll process in WM_CHAR
			else if (m_bNeedKeyboard && message == WM_KEYDOWN)
				bHandled = true;

		}

		//Forward to dialogs, except keyboard key if we aren't capturing it -- note they return our WINMSG enum
		if (message != WM_CHAR || m_bNeedKeyboard)
		{
			for (size_t i = 0; i < m_apOpenDialogs.size() && !bHandled; i++)
			{
				RetCode = m_apOpenDialogs[i]->WindowsMessage(message, wParam, lParam);
				if (RetCode != WINMSG_NOT_HANDLED)
					bHandled = true;
			}
		} 
	}
	if (!bHandled)
		return CallWindowProc((WNDPROC)m_FSXWindowProc, hWnd, message, wParam, lParam);

	//Certain messages have special return codes to indicate handled
	if (message == WM_SETCURSOR)
	{
		SetWindowLongPtr((HWND)wParam, DWLP_MSGRESULT, TRUE);
		return TRUE;
	}
	else if (message == WM_MOUSEACTIVATE)
	{
		SetWindowLongPtr((HWND)wParam, DWLP_MSGRESULT, MA_NOACTIVATEANDEAT);
		return MA_NOACTIVATEANDEAT;
	}

	//Other messages 0 means handled
	SetWindowLongPtr((HWND)wParam, DWLP_MSGRESULT, 0);
	return 0;   
}


///////////////////
//Internal

//Initialize graphics, and put up first dialog. This occurs after user enables VatConnect, i.e. FSX is
//running although we don't know yet if it's windowed or fullscreen. 
void CFSXGUI::InitGraphics(IDirect3DDevice9 *pI)
{
	if (!pI)
		return;

	//Initialize graphics library 
	m_Graphics.Initialize(pI); 

	//Hook into FSX's window procedure -- (FindWindow must come before CheckIfNewDevice so we know which
	//is the primary FSX window)
	HWND hTop = FindWindow(L"FS98MAIN", NULL);
	m_hFSXWindow = hTop;
	m_FSXWindowProc = SetWindowLongPtr(m_hFSXWindow, GWLP_WNDPROC, (LONG_PTR)FSXWndProc);

	//Determine if this is windowed device or one of the fullscreen ones and cache it
	CheckIfNewDevice(pI);

	//Initialize dialogs
	m_dlgMain.Initialize(this, &m_Graphics, m_hFSXWindow, m_bInWindowedMode);

	m_bGraphicsInitialized = true; 
	return;
}

//Given device to draw to, see if we already have it cached and if not, cache it.
//Also check if we have switched mode (i.e. we were windowed and this device is 
//a fullscreen device, or we were full-screen and this is a windowed one).
void CFSXGUI::CheckIfNewDevice(IDirect3DDevice9 *pI)
{
	if (!pI)
	{
		assert(0);
		return;
	}
	//Check if already have it
	bool bAlreadyHave = false;
	if (m_WindowedDeviceDesc.pDevice == pI)
		bAlreadyHave = true;
	else for (size_t i = 0; !bAlreadyHave && i < m_aFullscreenDevices.size(); i++)
	{
		if (m_aFullscreenDevices[i].pDevice == pI)
			bAlreadyHave = true;
	}

	//If we don't have it, determine if it's windowed or full-screen device and cache it
	if (!bAlreadyHave)
	{
		IDirect3DSwapChain9 *pISwapChain;
		if (FAILED(pI->GetSwapChain(0, &pISwapChain)))
			return;

		D3DPRESENT_PARAMETERS PP;
		if (FAILED(pISwapChain->GetPresentParameters(&PP)))
		{
			pISwapChain->Release();
			return;
		}
		pISwapChain->Release();

		//Windowed? 
		if (PP.Windowed)
		{
			//There is only one windowed device, so we don't cache it. Also since we're presenting
			//on one, we must be in windowed mode. 
			m_WindowedDeviceDesc.hWnd = PP.hDeviceWindow;  //should always be m_hFSXWin?
			m_WindowedDeviceDesc.pDevice = pI;
			m_pFullscreenPrimaryDevice = NULL;
			if (!m_bInWindowedMode)
			{
				m_bInWindowedMode = true;
				NotifyDialogsNowWindowed(PP.hDeviceWindow);
			}
		}
		//Fullscreen -- add to list. 
		else
		{
			static DeviceDescStruct D;
			D.hWnd = PP.hDeviceWindow;
			D.pDevice = pI;
			m_aFullscreenDevices.push_back(D);
			
			//Currently only draw to device associated with FSX's main window (primary screen on multi-monitor setups).
			//We cache the other devices in case we want to drag to other monitors in fullscreen mode, someday.
			if (PP.hDeviceWindow == m_hFSXWindow)
			{
				m_pFullscreenPrimaryDevice = pI;
				m_bInWindowedMode = false;
				m_WindowedDeviceDesc.pDevice = NULL; //no longer valid (FSX likely deletes it)
				NotifyDialogsNowFullscreen(pI, PP.BackBufferWidth, PP.BackBufferHeight);
			}
		}
	}
	return;
}

//Notify all dialogs we have switched to windowed mode (m_WindowedDeviceDesc)
void CFSXGUI::NotifyDialogsNowWindowed(HWND hWnd)
{
	for (size_t i = 0; i < m_apDialogs.size(); i++)
		m_apDialogs[i]->SwitchToWindowed(hWnd);
	return;

}

//Notify all dialogs we have switched to fullscreen mode, primary device = m_pFullscreenPrimaryDevice
void CFSXGUI::NotifyDialogsNowFullscreen(IDirect3DDevice9 *pI, int Width, int Height)
{
	for (size_t i = 0; i < m_apDialogs.size(); i++)
		m_apDialogs[i]->SwitchToFullscreen(pI, Width, Height);
	return;
}
 

//Push-to-talk button has been pressed (bPressed = true), or released (false)
void CFSXGUI::OnPTTButton(bool bPressed)
{

	return;
}


/////////////////////////////////////////////////////////////////////
//Called by dialogs

//Indicate some dialog needs keyboard keys (true) or no longer (false)
void CFSXGUI::IndicateNeedKeyboard(bool bNeedKeyboard)
{
	if (bNeedKeyboard)
		SetFocus(m_hFSXWindow);
	m_bNeedKeyboard = bNeedKeyboard;
	
	return;
}

//Indicate user has selected to close
void CFSXGUI::IndicateClose()
{
	m_dlgMain.Close();
	m_bRunning = false;
	ReleaseCapture();
	
	return;
}  