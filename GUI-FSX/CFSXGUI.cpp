#include "stdafx.h"
#include "CFSXGUI.h"

extern HMODULE g_hModule;
extern HWND g_hWnd; 
extern void SetAddonMenuText(char *Text);

#define STR_MENU_TEXT "VatConnect"
#define STR_PROXY_LAUNCH_ERROR L"\nERROR: Unable to launch server interface\n\nTry reinstalling VatConnect\n\n"
#define STR_PREF_HEADER "//\n//Please use the Settings screen to change these values instead of manually editing them.\n//\n"

#define SERVER_PROXY_NAME L"ServerSim Interface.exe"  //process name
#define DLG_UPDATE_HZ 4     //update rate for dialog->Update
#define RECEIVER_UPDATE_HZ 10  //update rate to check for pending packets
#define PREF_FILENAME_L L"\\Preferences.txt"
#define APPDATA_FOLDER L"\\VatConnect"

#define PROXY_LAUNCH_ERROR 

const DWORD KEY_REPEAT = 1 << 30;
const DWORD ALT_PRESSED = 1 << 29;

/////////////////
//Hooked windows procedure
CFSXGUI *g_pGUI = nullptr;
LRESULT CALLBACK FSXWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return g_pGUI->ProcessFSXWindowMessage(hWnd, message, wParam, lParam);
}

//////////////////
//CFSXGUI

CFSXGUI::CFSXGUI() : m_bRunning(false), m_bInitialized(false), m_FSXWindowProc(NULL), 
	m_bNeedMouseMove(false), m_bNeedKeyboard(false), m_bCheckForNewDevices(false),
	m_bInWindowedMode(true), m_pFullscreenPrimaryDevice(nullptr), m_dwNextDlgUpdateTime(0),
	m_dwNextReceiverUpdateTime(0), m_bServerProxyReady(false)
{
	g_pGUI = this;
	m_WindowedDeviceDesc.hWnd = nullptr;
	m_WindowedDeviceDesc.pDevice = nullptr;
}

CFSXGUI::~CFSXGUI()
{
}

//Set default preferences and also save it to PREF_FILENAME
int CFSXGUI::CreateDefaultPreferences(bool bAlsoSave)
{
	m_Prefs.PTTVKey = VK_CAPITAL;
	m_Prefs.PTTJoyButton = 1;
	
	if (bAlsoSave)
		return SavePreferences();
	return 1;
}

//Load preferences from given open preferences file. Closes the file when done. 
//Return 1 if everything read okay, 0 if invalid parameters somewhere, although it
//tries to keep reading what it can. Caller should first call 
//CreateDefaultPreferences(false) to load defaults in case one or more fields are 
//missing. Obviously field names need to be same as in SavePreferences()
int CFSXGUI::LoadPreferences(CParser &File)
{
	char FieldName[512];
	int Value;
	bool bSomethingBad = false;
	do
	{
		if (!File.GetString(FieldName, 512, true))
		{
			File.CloseFile();
			if (bSomethingBad)
				return 0;
			return 1;
		}
		if (strcmp(FieldName, "PTTKEY") == 0)
		{
			File.GetInt(&Value);
			if (Value > 0)
				m_Prefs.PTTVKey = Value;
			else
				bSomethingBad = true;
		}
		else if (strcmp(FieldName, "PTTJOYBUTTON") == 0)
		{
			File.GetInt(&Value);
			if (Value > 0)
				m_Prefs.PTTJoyButton = Value;
			else
				bSomethingBad = true;
		}
	} while (true);
}

//Save preferences to m_cPrefPathAndFilename
int CFSXGUI::SavePreferences()
{
	CParser File;

	if (!File.OpenFileForOutput(m_cPrefPathAndFilename))  
		return 0;
	
	File.WriteString(STR_PREF_HEADER);
	File.WriteString("\nPTTKey = ");
	File.WriteInt(m_Prefs.PTTVKey);

	File.WriteString("\nPTTJoyButton = ");
	File.WriteInt(m_Prefs.PTTJoyButton);

	File.CloseFile();

	return 1;
}


void CFSXGUI::Initialize()
{
	SetAddonMenuText(STR_MENU_TEXT);

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
	m_bInitialized = false;

	return;
}

//FSX has finished drawing to given device... add any overlays on that device 
void CFSXGUI::OnFSXPresent(IDirect3DDevice9 *pI)
{
	
	if (!m_bRunning)
		return;
		
	if (!m_bInitialized)
		Initialize(pI);

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
		m_dwNextDlgUpdateTime = GetTickCount() + (1000 / DLG_UPDATE_HZ);   //time is in milliseconds
	}

	//Update packet receiver at RECEIVER_UPDATE_HZ
	if (GetTickCount() >= m_dwNextReceiverUpdateTime)
	{
		ProcessPackets();
		m_dwNextReceiverUpdateTime = GetTickCount() + (1000 / RECEIVER_UPDATE_HZ);
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
	if (!m_bInitialized)
	{
		m_apDialogs.push_back(&m_dlgMain);
		m_apOpenDialogs.push_back(&m_dlgMain);
	}

	//Determine full path to preferences file. 
	PWSTR pPath;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &pPath)))
	{
		WCHAR Temp[MAX_PATH];
		size_t n;
		wcscpy_s(Temp, MAX_PATH, pPath);
		CoTaskMemFree(pPath);

		//Make directory just in case not there. Since this is the first module called,
		//other classes don't have to do this.
		PathAppend(Temp, APPDATA_FOLDER);
		CreateDirectory(Temp, NULL);

		PathAppend(Temp, PREF_FILENAME_L);
		wcstombs_s(&n, m_cPrefPathAndFilename, Temp, MAX_PATH);
	}

	//Load preferences/options 
	CParser File;
	CreateDefaultPreferences(false);

	if (!File.SetFileAsInput(m_cPrefPathAndFilename))
		CreateDefaultPreferences(true);
	else
		LoadPreferences(File); //REVISIT if this fails (mangled file?), values are okay but should alert user

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
		if ((message != WM_CHAR && message != WM_UNICHAR) || m_bNeedKeyboard)
		{
			//Parent window should be responding to UNICODE_NOCHAR query, not us
			if (message == WM_UNICHAR && wParam == UNICODE_NOCHAR)
				bHandled = false;
			else
			{
				if (wParam == UNICODE_NOCHAR)
					message = WM_CHAR;

				for (size_t i = 0; i < m_apOpenDialogs.size() && !bHandled; i++)
				{
					RetCode = m_apOpenDialogs[i]->WindowsMessage(message, wParam, lParam);
					if (RetCode != WINMSG_NOT_HANDLED)
						bHandled = true;
				}
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

//Initialize everything and put up first dialog. This occurs after user enables VatConnect, i.e. FSX is
//running although we don't know yet if it's windowed or fullscreen. 
void CFSXGUI::Initialize(IDirect3DDevice9 *pI)
{
	if (!pI)
		return;

	m_Graphics.Initialize(pI); 

	//Hook into FSX's window procedure -- (FindWindow must come before CheckIfNewDevice so we know which
	//is the primary FSX window)
	HWND hTop = FindWindow(L"FS98MAIN", nullptr);
	m_hFSXWindow = hTop;
	m_FSXWindowProc = SetWindowLongPtr(m_hFSXWindow, GWLP_WNDPROC, (LONG_PTR)FSXWndProc);

	//Determine if this is windowed device or one of the fullscreen ones and cache it
	CheckIfNewDevice(pI);

	//Initialize dialogs
	m_dlgMain.Initialize(this, &m_Graphics, m_hFSXWindow, m_bInWindowedMode);
	m_dlgMain.Open();

	m_bInitialized = true;

	//Initialize packet sender and receiver
	m_Sender.Initialize(SERVER_PROXY_LISTEN_PORT);
	m_Receiver.Initialize(CLIENT_LISTEN_PORT);

	//Determine this DLL's full path (get full path & name and back up to first backslash)
	WCHAR Buffer[MAX_PATH] = { 0 };
	GetModuleFileName(g_hModule, Buffer, MAX_PATH);
	int Index = wcslen(Buffer);
	while (Index >= 0 && Buffer[Index] != '\\')
		Index--;
	if (Buffer[Index] == '\\')
		Index++;

	//Tack on server proxy process name
	wcscpy_s(&Buffer[Index], (MAX_PATH - Index), SERVER_PROXY_NAME);
	
	//Launch server proxy -- it'll send a ServerProxyReady packet after it's initialized
	ZeroMemory(&m_ServerProcStartupInfo, sizeof(m_ServerProcStartupInfo));
	m_ServerProcStartupInfo.cb = sizeof(m_ServerProcStartupInfo);
	m_ServerProcStartupInfo.dwFlags = STARTF_PREVENTPINNING;
	ZeroMemory(&m_ServerProcInfo, sizeof(m_ServerProcInfo));

	//DEBUG CREATE_NEW_CONSOLE to put up console window for ServerSim; use CREATE_NO_WINDOW for final version
	if (!CreateProcess(Buffer, NULL, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &m_ServerProcStartupInfo, &m_ServerProcInfo))
		m_dlgMain.AddErrorMessage(STR_PROXY_LAUNCH_ERROR);
	
	return;
}

//Process any incoming packets
int CFSXGUI::ProcessPackets()
{
	static char Buffer[LARGEST_PACKET_SIZE + 8];  //extra 8 because why not

	while (m_Receiver.GetNextPacket(Buffer))
	{
		ePacketType Type = ((PacketHeader *)(&Buffer[0]))->Type;

		//Server proxy has launched and is ready 
		if (Type == PROXY_READY_PACKET)
		{
			m_bServerProxyReady = true;
			ReqLoginInfoPacket P;
			m_Sender.Send(&P);
		}
		else if (Type == LOGIN_INFO_PACKET)
		{
			m_dlgMain.SetSavedLoginInfo((LoginInfoPacket *)(&Buffer[0]));
		}
	}

	return 1;
}

//Given device to draw to, see if we already have it cached and if not, cache it.
//Also check if we have switched mode (i.e. we were windowed and this device is 
//a fullscreen device, or we were full-screen and this is a windowed one).
void CFSXGUI::CheckIfNewDevice(IDirect3DDevice9 *pI)
{
	if (!pI)
		return;
	
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
			m_pFullscreenPrimaryDevice = nullptr;
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
				m_WindowedDeviceDesc.pDevice = nullptr; //no longer valid (FSX likely deletes it)
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