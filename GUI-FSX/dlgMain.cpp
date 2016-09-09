#include "stdafx.h"
#include "Dialogs.h"
#include "CFSXGUI.h"

#define FONT_NAME L"Arial"
#define BUTTON_WIDTH_CHAR 8   //should be longest button string below, plus 2 chars
#define BUTTON_HEIGHT_CHAR 1

#define TEXT_CONNECT L"Connect"
#define TEXT_DISCONNECT L"Disconnect"
#define TEXT_TEXT L"Text"
#define TEXT_FP L"Flight Plan"
#define TEXT_ATC L"ATC"
#define TEXT_WX L"Weather"
#define TEXT_SETTINGS L"Settings"
#define TEXT_MINIMIZE L"_"     //assumed 1 character
#define TEXT_CLOSE L"X"        //1 char
#define TEXT_MAXIMIZE L"\x25BA"  //Right pointer, 1 char
#define TEXT_CIRCLE L"\x25CF"    //Solid large circle, 1 char

#define BLINK_INTERVAL_SECS 0.2  //number of seconds between each blink
  
CMainDlg::CMainDlg() : m_pGUI(nullptr), m_pGraph(nullptr), m_iScreenX(0), m_iScreenY(0) , m_iCursorScreenX(0),
	m_iCursorScreenY(0), m_iWidthPix(0), m_iHeightPix(0), m_bDraggingDialog(false), m_pFullscreenDevice(nullptr),
	m_bInWindowedMode(true), m_CurButtonLit(BUT_NONE), m_bMinimized(false), m_Status(STAT_RED),
	m_bBlinkOn(true), m_dNextBlinkSwitchTime(0.0), m_bHaveMouseCapture(false), m_pCurDialogOpen(nullptr),
	m_bWindowActive(true)
{
}

CMainDlg::~CMainDlg()
{
}

/////////////
//Base class

//Returns WINMSG_RESULT 
int CMainDlg::WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	//We're trapping FSX parent window messages so this is how we get notified
	//of LButton presses -- convert to LButton message for easier handling below
	if (message == WM_PARENTNOTIFY && LOWORD(wParam) == WM_LBUTTONDOWN)
		message = WM_LBUTTONDOWN;

	//Update mouse position (in client area -- 0,0 is top left but could be negative)
	if (message == WM_MOUSEMOVE || message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || message == WM_RBUTTONDOWN)
	{
		m_iCursorScreenX = GET_X_LPARAM(lParam);
		m_iCursorScreenY = GET_Y_LPARAM(lParam);
	}

	//Check if we lost capture (could happen if we are dragging dialog because we 
	//SetCapture to get the mouse)
	if (message == WM_CAPTURECHANGED)
	{
		m_bDraggingDialog = false;
		m_bHaveMouseCapture = false;
		return WINMSG_NOT_HANDLED;
	}

	//Determine if mouse within dialog
	bool bFullRedraw = false, bHandled = false, bCursorWithinDialog = false;
	int ret;
	if ((m_iCursorScreenX >= m_iScreenX && m_iCursorScreenX <= (m_iScreenX + m_iWidthPix) &&
		m_iCursorScreenY >= m_iScreenY && m_iCursorScreenY <= (m_iScreenY + m_iHeightPix)))
		bCursorWithinDialog = true;

	//Special case -- in windowed mode we get both PARENTNOTIFY and MOUSEACTIVATE if LButton down 
	//-- we handle PARENTNOTIFY above so tell caller we handled MOUSEACTIVATE 
	if (m_bHaveMouseCapture && (message == WM_MOUSEACTIVATE) && ((HIWORD(lParam) == WM_LBUTTONDOWN) || 
		(HIWORD(lParam) == WM_RBUTTONDOWN)))
		return WINMSG_HANDLED_NO_REDRAW;

	//Grab mouse if cursor within dialog, release if not (and not dragging, which handles release below)
	//REVISIT -- if any dialog extends beyond background (list box?) this won't work for the overreaching 
	//part. We may need a new flag for special case "something extending beyond background box"
	if (bCursorWithinDialog && !m_bHaveMouseCapture)
	{
		SetCapture(m_hFSXWin);
		m_bHaveMouseCapture = true;
	}
	else if (m_bHaveMouseCapture && !bCursorWithinDialog && !m_bDraggingDialog)
	{
		ReleaseCapture();
		m_bHaveMouseCapture = false;
	}
	
	//If within dialog, first check all our buttons. We only process LButtondown currently.
	if (message == WM_LBUTTONDOWN && !m_bDraggingDialog && bCursorWithinDialog)
	{
		
		//Convert to X&Y within the dialog
		int DlgMouseX = m_iCursorScreenX - m_iScreenX;
		int DlgMouseY = m_iCursorScreenY - m_iScreenY;

		//See if within each button (separate logic for minimized and normal size mode). 
		if (m_bMinimized)
		{
			if (m_butMaximize.IsWithin(DlgMouseX, DlgMouseY))
			{
				ret = ProcessButtonClick(m_butMaximize.ButtonID);
				return WINMSG_HANDLED_NO_REDRAW;
			}
		}
		else
		{
			for (size_t i = 0; i < m_apButtons.size() && !bHandled; i++)
			{
				if (m_apButtons[i]->IsWithin(DlgMouseX, DlgMouseY))
				{
					ret = ProcessButtonClick(m_apButtons[i]->ButtonID);
					if (ret == WINMSG_HANDLED_NO_REDRAW)
						return ret;
					if (ret == WINMSG_HANDLED_REDRAW_US)
					{
						//Redraw just this button and update surface
						m_pGraph->SetOutputBitmap(&m_bitFullOutput);
						m_apButtons[i]->Draw();
						m_pGraph->CopyBitmapDCToSurface(&m_bitFullOutput);
						return WINMSG_HANDLED_NO_REDRAW;
					}
					if (ret == WINMSG_HANDLED_REDRAW_ALL)
					{
						bFullRedraw = true;
						bHandled = true;
					}
				}
			}
		}
	}
	
	//If not handled by buttons, forward to screen dialogs. Done outside of above loop in case of WM_CHAR
	if (!bHandled && !m_bDraggingDialog && (bCursorWithinDialog || message == WM_CHAR))
	{
		//If mouse message, re-make param to be relative to us
		if (message == WM_LBUTTONDOWN || message == WM_MOUSEMOVE)
		{
			int X = GET_X_LPARAM(lParam) - m_iScreenX, Y = GET_Y_LPARAM(lParam) - m_iScreenY;
			lParam = MAKELPARAM(X, Y);
		}
		for (size_t i = 0; i < m_apChildDialogs.size() && !bHandled; i++)
		{
			ret = m_apChildDialogs[i]->WindowsMessage(message, wParam, lParam);
			if (ret == WINMSG_HANDLED_NO_REDRAW)
				bHandled = true;
			if (ret == WINMSG_HANDLED_REDRAW_US)
			{
				bHandled = true;
				if (!bFullRedraw)
				{
					//Redraw just the screen dialog and update surface
					m_pGraph->SetOutputBitmap(&m_bitFullOutput);
					m_apChildDialogs[i]->Draw(nullptr);
					m_pGraph->CopyBitmapDCToSurface(&m_bitFullOutput);
				}
			}
			if (ret == WINMSG_HANDLED_REDRAW_ALL)
			{
				bHandled = true;
				bFullRedraw = true;
			}
		}
	}

	if (bFullRedraw)
		DrawWholeDialogToDC();

	if (bHandled)
		return WINMSG_HANDLED_NO_REDRAW;

	//Currently dragging (whether or not cursor within dialog)?
	if (m_bDraggingDialog && (message == WM_MOUSEMOVE || message == WM_LBUTTONUP))
	{
		//Ending? 
		if (message == WM_LBUTTONUP)
		{
			m_bDraggingDialog = false;
			ReleaseCapture();
			m_bHaveMouseCapture = false;
			ClampDialogToScreen();
		} 

		//Update dragged position
		else if (message == WM_MOUSEMOVE)
		{
			m_iScreenX += (m_iCursorScreenX - m_iLastDragScreenX);
			m_iScreenY += (m_iCursorScreenY - m_iLastDragScreenY);
			int iNewX = m_iScreenX, iNewY = m_iScreenY;

			//Keep onscreen
			bool bClamped = ClampDialogToScreen();
			if (bClamped)
			{
				//Offset cursor by amount we clamped
				int iOffX = (m_iScreenX - iNewX), iOffY = (m_iScreenY - iNewY);
				m_iCursorScreenX += iOffX;
				m_iCursorScreenY += iOffY;
				POINT P;
				GetCursorPos(&P);
				SetCursorPos(P.x + iOffX, P.y + iOffY);
			}
			m_iLastDragScreenX = m_iCursorScreenX;
			m_iLastDragScreenY = m_iCursorScreenY;
		}
		return WINMSG_HANDLED_NO_REDRAW;
	}

	//Forward to open dialog if deactivating
	if (message == WM_ACTIVATE && wParam == WA_INACTIVE)
	{
		m_bWindowActive = false;
		if (m_pCurDialogOpen)
			m_pCurDialogOpen->WindowsMessage(message, wParam, lParam);
		return WINMSG_HANDLED_REDRAW_US;
	}

	//If cursor within dialog, since it wasn't handled by buttons or controls, check if starting a drag 
	if (bCursorWithinDialog)
	{
		//Starting drag?
		if (message == WM_LBUTTONDOWN)
		{
			m_bDraggingDialog = true;
			m_iLastDragScreenX = m_iCursorScreenX;
			m_iLastDragScreenY = m_iCursorScreenY;
			
			//Indicate to dialog it lost focus (in case it has an edit box with focus)
			if (m_pCurDialogOpen)
				m_pCurDialogOpen->WindowsMessage(WM_KILLFOCUS, 0, 0);

			//If window wasn't active but user clicking within the dialog, say not handled so FSX will process it
			//REVISIT I don't think this is working -- can drag dialog without reactivating it
			if (!m_bWindowActive)
				return WINMSG_NOT_HANDLED;

			return WINMSG_HANDLED_NO_REDRAW;
		}
		//No -- trap the other mouse messages so FSX doesn't handle them
		if (message == WM_RBUTTONDOWN)
			return WINMSG_HANDLED_NO_REDRAW;
	}
	
	//These messages we note but pass through as not handled so FSX can 
	//handle them too
	if (m_bInWindowedMode && message == WM_SIZE)
		SwitchToWindowed(m_hFSXWin);

	//L button outside our whole dialog, force lose-focus message in case we were capturing keyboard
	if (m_pCurDialogOpen && (message == WM_LBUTTONDOWN || message == WM_KILLFOCUS))
		m_pCurDialogOpen->WindowsMessage(WM_KILLFOCUS, 0, 0);

	return WINMSG_NOT_HANDLED;
}

//Given button ID has been left-clicked. Process it and return WINMSG enum
WINMSG_RESULT CMainDlg::ProcessButtonClick(int ButtonID)
{
	//Do nothing if clicking already-lit button. (Buttons that stay lit are set below)
	if (ButtonID == m_CurButtonLit)
		return WINMSG_HANDLED_NO_REDRAW;
	
	//Release keyboard input (if we had it)
	GetKeyboardInput(false);

	//If new button is selected, tell old one to unlight and close associated dialog 
	if (ButtonID != BUT_MIN && ButtonID != BUT_MAX && ButtonID != BUT_CLOSE)
	{
		if (m_pCurButtonLit)
			m_pCurButtonLit->SetOn(false);
		m_pCurButtonLit = nullptr;
		if (m_pCurDialogOpen)
		{
			m_pCurDialogOpen->Close();
			m_pCurDialogOpen = nullptr;
		}
	}

	switch (ButtonID)
	{
	case BUT_CONNECT:
		m_pCurButtonLit = static_cast<CTwoStateButton*>(&m_butConnect); 
		m_CurButtonLit = BUT_CONNECT;
		m_butConnect.SetOn(true);
		m_dlgLogin.Open();
		m_pCurDialogOpen = static_cast<CDialog*>(&m_dlgLogin);
		break;

	case BUT_DISCONNECT:
		m_pCurButtonLit = static_cast<CTwoStateButton*>(&m_butDisconnect);
		m_CurButtonLit = BUT_DISCONNECT;
		m_butDisconnect.SetOn(true);
		m_dlgLogin.Open();
		m_pCurDialogOpen = static_cast<CDialog*>(&m_dlgLogin);
		break;

	case BUT_TEXT:
		m_pCurButtonLit = static_cast<CTwoStateButton*>(&m_butText);
		m_butText.SetOn(true);
		m_CurButtonLit = BUT_TEXT;
		m_dlgText.Open();
		m_pCurDialogOpen = static_cast<CDialog*>(&m_dlgText);
		break;

	case BUT_FP:
		m_pCurButtonLit = static_cast<CTwoStateButton*>(&m_butFlightPlan);
		m_butFlightPlan.SetOn(true);
		m_CurButtonLit = BUT_FP;
		m_dlgFlightPlan.Open();
		m_pCurDialogOpen = static_cast<CDialog*>(&m_dlgFlightPlan);
		break;

	case BUT_ATC:
		m_pCurButtonLit = static_cast<CTwoStateButton*>(&m_butATC);
		m_butATC.SetOn(true);
		m_CurButtonLit = BUT_ATC;
		m_dlgATC.Open();
		m_pCurDialogOpen = static_cast<CDialog*>(&m_dlgATC);
		break;

	case BUT_WX:
		m_pCurButtonLit = static_cast<CTwoStateButton*>(&m_butWeather);
		m_butWeather.SetOn(true);
		m_CurButtonLit = BUT_WX;
		m_dlgWX.Open();
		m_pCurDialogOpen = static_cast<CDialog*>(&m_dlgWX);
		break;

	case BUT_SETTINGS:
		m_pCurButtonLit = static_cast<CTwoStateButton*>(&m_butSettings);
		m_butSettings.SetOn(true);
		m_CurButtonLit = BUT_SETTINGS;
		m_dlgSettings.Open();
		m_pCurDialogOpen = static_cast<CDialog*>(&m_dlgSettings);
		break;

	case BUT_MIN:
		if (m_pCurDialogOpen)
			m_pCurDialogOpen->Close();
		SwitchToMinimized();
		break;

	case BUT_MAX:
		SwitchToNormal();
		break;

	case BUT_CLOSE:
		m_pGUI->IndicateClose();
		break;

	default:
		//undefined button?
		return WINMSG_NOT_HANDLED;

	}

	return WINMSG_HANDLED_REDRAW_ALL;
}

//Indicate to given button in m_apButtons that another button has been selected.
//Return WINMSG enum
WINMSG_RESULT CMainDlg::DeselectButton(int ButtonID)
{

	return WINMSG_NOT_HANDLED;
}

int CMainDlg::Update()
{
	int rc = WINMSG_NOT_HANDLED;

	//Update our blinking (the little status light in mini-mode)
	if (m_Status == STAT_BLINKING && m_Timer.GetTimeSeconds() >= m_dNextBlinkSwitchTime)
	{
		m_bBlinkOn ^= 1;
		m_dNextBlinkSwitchTime = m_Timer.GetTimeSeconds() + BLINK_INTERVAL_SECS;
		DrawWholeDialogToDC();
		rc = WINMSG_HANDLED_REDRAW_US;
	}
	
	//Update open dialog if any
	if (m_pCurDialogOpen)
		rc = m_pCurDialogOpen->Update();
	
	return rc;
}

//Draw current output dialog to given D3D device
int CMainDlg::Draw(IDirect3DDevice9* pDevice)
{
	if (!m_pGraph)
		return 0;

	if (m_bMinimized)
	{
		if (!pDevice)
			m_pGraph->DrawBitmapToOutputBitmap(&m_bitMinimizedOutput, 0, 0);
		else
			m_pGraph->DrawBitmapSurfaceOnDevice(&m_bitMinimizedOutput, pDevice, m_iScreenX, m_iScreenY);

	}
	else
	{
		if (!pDevice)
			m_pGraph->DrawBitmapToOutputBitmap(&m_bitFullOutput, 0, 0);
		else
			m_pGraph->DrawBitmapSurfaceOnDevice(&m_bitFullOutput, pDevice, m_iScreenX, m_iScreenY);
	}

	return 1;
}

//Called by child dialogs when they redrew themselves on their own (versus in response to a Windows message)
//so we will know to copy them to our output surface
int CMainDlg::OnChildInitiatedRedraw()
{
	return DrawWholeDialogToDC();
}

//Called if something changes -- redraw the dialog's in-memory m_bitFullOutput/minimized output bitmap and update 
//its corresponding D3D surface 
int CMainDlg::DrawWholeDialogToDC()
{ 
	//Minimized mode?
	if (m_bMinimized)
	{
		m_pGraph->SetOutputBitmap(&m_bitMinimizedOutput);

		//Start with background
		m_pGraph->DrawBitmapToOutputBitmap(&m_bitMinimizedBack, 0, 0);

		//Draw maximize button
		m_butMaximize.Draw();

		//Draw status light, indended one char because light is 1 char wide
		if (m_Status == STAT_RED)
			m_pGraph->DrawBitmapToOutputBitmap(&m_bitRedCircle, (m_butMaximize.m_iX - m_bitRedCircle.WidthPix) / 2, (m_bitMinimizedOutput.HeightPix - m_bitRedCircle.HeightPix) / 2);
		else if (m_Status == STAT_GREEN || (m_Status == STAT_BLINKING && m_bBlinkOn))
			m_pGraph->DrawBitmapToOutputBitmap(&m_bitGreenCircle, (m_butMaximize.m_iX - m_bitGreenCircle.WidthPix) / 2, (m_bitMinimizedOutput.HeightPix - m_bitGreenCircle.HeightPix) / 2);
		//else blink off -- draw nothing

		//Update Direct3D surface
		m_pGraph->CopyBitmapDCToSurface(&m_bitMinimizedOutput);

	}
	//Regular (maximized) mode
	else
	{
		m_pGraph->SetOutputBitmap(&m_bitFullOutput);

		//start with background
		m_pGraph->DrawBitmapToOutputBitmap(&m_bitDialogBack, 0, 0);

		//Draw each frame button to current output
		size_t i;
		for (i = 0; i < m_apButtons.size(); i++)
			m_apButtons[i]->Draw();

		//Draw "screen" dialog to current output 
		if (m_pCurDialogOpen)
			m_pCurDialogOpen->Draw(nullptr);
		
		//Update Direct3D surface
		m_pGraph->CopyBitmapDCToSurface(&m_bitFullOutput);
	}

	return 1;
}

//Minimize the dialog
int CMainDlg::SwitchToMinimized()
{
	//Save out current maximized position
	m_iMaximizedScreenX = m_iScreenX;
	m_iMaximizedScreenY = m_iScreenY;

	//Load up saved out minimized position and size
	m_bMinimized = true;
	m_iScreenX = m_iMinimizedScreenX;
	m_iScreenY = m_iMinimizedScreenY;
	m_iWidthPix = m_iMinimizedWidthPix;
	m_iHeightPix = m_iMinimizedHeightPix;

	DrawWholeDialogToDC();
	ClampDialogToScreen();

	return 1;
}

//Make minimized dialog normal sized (aka maximized)
int CMainDlg::SwitchToNormal()
{
	//Save out current minimized position
	m_iMinimizedScreenX = m_iScreenX;
	m_iMinimizedScreenY = m_iScreenY;

	//Load up saved out maximized position and size
	m_bMinimized = false;
	m_iScreenX = m_iMaximizedScreenX;
	m_iScreenY = m_iMaximizedScreenY;
	m_iWidthPix = m_iMaximizedWidthPix;
	m_iHeightPix = m_iMaximizedHeightPix;

	//"Press" current button selected again to open up the corresponding dialog
	BUTTONID OldSelected = m_CurButtonLit;
	m_CurButtonLit = BUT_NONE;
	ProcessButtonClick(OldSelected);

	DrawWholeDialogToDC();
	ClampDialogToScreen();

	return 1;
}

int CMainDlg::Open()
{
	ProcessButtonClick(BUT_CONNECT);
	return 1;
}

int CMainDlg::Close()
{
	if (m_bHaveMouseCapture)
	{
		ReleaseCapture();
		m_bHaveMouseCapture = false;
	}
	m_bDraggingDialog = false;
	if (m_pCurDialogOpen)
		m_pCurDialogOpen->Close();
	m_CurButtonLit = BUT_NONE;

	return 1;
}

int CMainDlg::Shutdown()
{

	for (size_t i = 0; i < m_apButtons.size(); i++)
		m_apButtons[i]->Shutdown();
	for (size_t i = 0; i < m_apChildDialogs.size(); i++)
		m_apChildDialogs[i]->Shutdown();
	m_pGraph->DeleteBM(&m_bitDialogBack);
	m_pGraph->DeleteBM(&m_bitMinimizedBack);
	m_pGraph->DeleteBM(&m_bitFullOutput);
	m_pGraph->DeleteBM(&m_bitMinimizedOutput);
	m_pGraph->DeleteBM(&m_bitRedCircle);
	m_pGraph->DeleteBM(&m_bitGreenCircle);
	if (m_bHaveMouseCapture)
	{
		ReleaseCapture();
		m_bHaveMouseCapture = false;
	}
	return 0;
}

//Switch to windowed mode, or if in windowed already, refresh window size
//(e.g. after WM_SIZE message)
int CMainDlg::SwitchToWindowed(HWND hWnd)
{
	WINDOWINFO wi;
	wi.cbSize = sizeof(WINDOWINFO);
	GetWindowInfo(hWnd, &wi);

	if (!m_bInWindowedMode)
	{
		if (m_bDraggingDialog)
		{
			m_bDraggingDialog = false;
			ReleaseCapture();
			m_bHaveMouseCapture = false;
		}
		m_bInWindowedMode = true;
	}

	//Determine new screen position
	float fXPos = (float)m_iScreenX / (float)(m_rectFSXWin.right - m_rectFSXWin.left);
	float fYPos = (float)m_iScreenY / (float)(m_rectFSXWin.bottom - m_rectFSXWin.top);
	m_iScreenX = (int)(fXPos * (float)(wi.rcClient.right - wi.rcClient.left));
	m_iScreenY = (int)(fYPos * (float)(wi.rcClient.bottom - wi.rcClient.top));

	//Re-scale cached position of full/minimized version that's not currently shown
	if (m_bMinimized)
	{
		fXPos = (float)m_iMaximizedScreenX / (float)(m_rectFSXWin.right - m_rectFSXWin.left);
		fYPos = (float)m_iMaximizedScreenY / (float)(m_rectFSXWin.bottom - m_rectFSXWin.top);
		m_iMaximizedScreenX = (int)(fXPos * (float)(wi.rcClient.right - wi.rcClient.left));
		m_iMaximizedScreenY = (int)(fYPos * (float)(wi.rcClient.bottom - wi.rcClient.top));
	}
	else
	{
		fXPos = (float)m_iMinimizedScreenX / (float)(m_rectFSXWin.right - m_rectFSXWin.left);
		fYPos = (float)m_iMinimizedScreenY / (float)(m_rectFSXWin.bottom - m_rectFSXWin.top);
		m_iMinimizedScreenX = (int)(fXPos * (float)(wi.rcClient.right - wi.rcClient.left));
		m_iMinimizedScreenY = (int)(fYPos * (float)(wi.rcClient.bottom - wi.rcClient.top));
	}

	//Set new window/draw area size
	memcpy(&m_rectFSXWin, &wi.rcClient, sizeof(RECT));
	ClampDialogToScreen();
	return 1;
}

int CMainDlg::SwitchToFullscreen(IDirect3DDevice9* pFullscreenDevice, int Width, int Height)
{
	if (m_bInWindowedMode)
	{
		//Determine new screen position
		float fXPos = (float)m_iScreenX / (float)(m_rectFSXWin.right - m_rectFSXWin.left);
		float fYPos = (float)m_iScreenY / (float)(m_rectFSXWin.bottom - m_rectFSXWin.top);
		m_iScreenX = (int)(fXPos * (float)Width);
		m_iScreenY = (int)(fYPos * (float)Height);

		//Re-scale cached position of full/minimized version that's not currently shown
		if (m_bMinimized)
		{
			fXPos = (float)m_iMaximizedScreenX / (float)(m_rectFSXWin.right - m_rectFSXWin.left);
			fYPos = (float)m_iMaximizedScreenY / (float)(m_rectFSXWin.bottom - m_rectFSXWin.top);
			m_iMaximizedScreenX = (int)(fXPos * (float)Width);
			m_iMaximizedScreenY = (int)(fYPos * (float)Height);
		}
		else
		{
			fXPos = (float)m_iMinimizedScreenX / (float)(m_rectFSXWin.right - m_rectFSXWin.left);
			fYPos = (float)m_iMinimizedScreenY / (float)(m_rectFSXWin.bottom - m_rectFSXWin.top);
			m_iMinimizedScreenX = (int)(fXPos * (float)Width);
			m_iMinimizedScreenY = (int)(fYPos * (float)Height);
		}

		if (m_bDraggingDialog)
		{
			m_bDraggingDialog = false;
			ReleaseCapture();
			m_bHaveMouseCapture = false;
		}
		m_bInWindowedMode = false;
	}
			
	//Record new position and size
	m_pFullscreenDevice = pFullscreenDevice;  
	IDirect3DSwapChain9 *pISwapChain;
	if (FAILED(pFullscreenDevice->GetSwapChain(0, &pISwapChain)))
		return 0;

	D3DPRESENT_PARAMETERS PP;
	if (FAILED(pISwapChain->GetPresentParameters(&PP)))
	{
		pISwapChain->Release();
		return 0;
	}
	pISwapChain->Release();
	WINDOWINFO wi;
	wi.cbSize = sizeof(WINDOWINFO);
	GetWindowInfo(PP.hDeviceWindow, &wi);
	memcpy(&m_rectFSXWin, &wi.rcWindow, sizeof(RECT));

	ClampDialogToScreen();
	return 1;
}


int CMainDlg::Initialize(CFSXGUI *pGUI, C2DGraphics *pGraph, HWND hFSXWin, bool bInWindowedMode)
{
	m_pGUI = pGUI;
	m_pGraph = pGraph;

	//Initialize state variables
	m_CurButtonLit = BUT_NONE;
	m_bMinimized = false;
	m_Status = STAT_RED;
	m_bHaveMouseCapture = false;
	m_pCurDialogOpen = nullptr;

	//Record initial window position
	WINDOWINFO wi;
	wi.cbSize = sizeof(WINDOWINFO);
	GetWindowInfo(hFSXWin, &wi);
	if (bInWindowedMode)
		memcpy(&m_rectFSXWin, &wi.rcClient, sizeof(RECT));
	else
		memcpy(&m_rectFSXWin, &wi.rcWindow, sizeof(RECT));

	m_bInWindowedMode = bInWindowedMode;
	m_hFSXWin = hFSXWin;
	
	if (!CreateFrame() || !CreateScreenDialogs())
		return 0;

	//Set initial dialog position to center of screen
	m_iScreenX = (m_rectFSXWin.right - m_rectFSXWin.left - m_iWidthPix) / 2;
	m_iScreenY = (m_rectFSXWin.bottom - m_rectFSXWin.top - m_iHeightPix) / 2;
	
	//Set initial minimized dialog position to bottom-left
	m_iMinimizedScreenX = 0;
	m_iMinimizedScreenY = m_rectFSXWin.bottom - m_bitMinimizedBack.HeightPix;
	
	//Load created buttons into our pointer array
	m_apButtons.push_back(static_cast<CTwoStateButton*>(&m_butConnect));
	m_apButtons.push_back(static_cast<CTwoStateButton*>(&m_butDisconnect));
	m_apButtons.push_back(static_cast<CTwoStateButton*>(&m_butATC));
	m_apButtons.push_back(static_cast<CTwoStateButton*>(&m_butText));
	m_apButtons.push_back(static_cast<CTwoStateButton*>(&m_butFlightPlan));
	m_apButtons.push_back(static_cast<CTwoStateButton*>(&m_butWeather));
	m_apButtons.push_back(static_cast<CTwoStateButton*>(&m_butSettings));
	m_apButtons.push_back(static_cast<CTwoStateButton*>(&m_butMinimize));
	m_apButtons.push_back(static_cast<CTwoStateButton*>(&m_butClose));

	//Load created screen dialogs into our array
	m_apChildDialogs.push_back(static_cast<CDialog *>(&m_dlgLogin));
	m_apChildDialogs.push_back(static_cast<CDialog *>(&m_dlgText));
	m_apChildDialogs.push_back(static_cast<CDialog *>(&m_dlgATC));
	m_apChildDialogs.push_back(static_cast<CDialog *>(&m_dlgWX));
	m_apChildDialogs.push_back(static_cast<CDialog *>(&m_dlgSettings));
	m_apChildDialogs.push_back(static_cast<CDialog *>(&m_dlgFlightPlan));

	//Set initial selected button
	m_butConnect.SetOn(true);
	m_pCurButtonLit = &m_butConnect;
	m_CurButtonLit = BUT_CONNECT;
	m_pCurDialogOpen = &m_dlgLogin;
	m_dlgLogin.Open();

	DrawWholeDialogToDC();
	ClampDialogToScreen();

	return 1;
}

//Indicate error occured -- for now put in text dialog and switch to that
int CMainDlg::AddErrorMessage(WCHAR *pMsg)
{
	ProcessButtonClick(BUT_TEXT);
	m_dlgText.AddText(pMsg, COL_ERROR_TEXT);
	DrawWholeDialogToDC();
	return 1;
}

//Push this saved login info (from previous session) into appropriate dialogs (Login and FlightPlan)
int CMainDlg::SetSavedLoginInfo(LoginInfoPacket *pLoginInfo)
{
	LoginDlgDataStruct L;
	size_t n;

	mbstowcs_s(&n, L.ServerName, pLoginInfo->szServerName, 64);
	mbstowcs_s(&n, L.Name, pLoginInfo->szUserName, 64);
	mbstowcs_s(&n, L.ID, pLoginInfo->szUserID, 32);
	mbstowcs_s(&n, L.Password, pLoginInfo->szPassword, 32);
	mbstowcs_s(&n, L.Callsign, pLoginInfo->szCallsign, 16);
	mbstowcs_s(&n, L.ACType, pLoginInfo->szACType, 8);
	L.bIsPilot = true;
	L.bIsObserver = false;

	WCHAR Equip[4];
	mbstowcs_s(&n, Equip, pLoginInfo->szACEquip, 4);

	m_dlgLogin.SetLoginData(&L);
	m_dlgFlightPlan.SetAircraftInfo(&L.Callsign[0], &L.ACType[0], &Equip[0]);

	return 1;
}

////////////////////////
// Internal


//Create frame bitmap and button controls, also minimized version. The frame holds buttons on top 
//(connect/disconnect/minimize/close) and five on left, then has a "screen" area where child dialogs appear. 
int CMainDlg::CreateFrame()
{
	//Find and set the font
	int CharW, CharH, W, H;
	if (!m_pGraph->FindBestFont(FONT_NAME, FONT_SIZE, true, false, false, &m_hFont))
		return 0;
	m_pGraph->SetFont(m_hFont);
	m_pGraph->SetTextColor(COL_BUTTON_TEXT);
	if (!m_pGraph->GetStringPixelSize(_T("M"), &CharW, &CharH))  //M = widest of all characters
		return 0;

	//Create background (and original "full-dialog output")
	int ButWidthPix = CharW * BUTTON_WIDTH_CHAR;
	int ButHeightPix = CharH * BUTTON_HEIGHT_CHAR;
	m_iWidthPix = ButWidthPix + CharW * 2 + CharW * TEXT_WIDTH_CHAR;
	m_iHeightPix = CharH * (TEXT_HEIGHT_CHAR + 2) + CharH / 2;
	m_iMaximizedWidthPix = m_iWidthPix;
	m_iMaximizedHeightPix = m_iHeightPix;
	if (!m_pGraph->MakeNewBitmap(m_iWidthPix, m_iHeightPix, &m_bitDialogBack))
		return 0;
	m_pGraph->MakeNewBitmap(m_iWidthPix, m_iHeightPix, &m_bitFullOutput);
	m_pGraph->SetOutputBitmap(&m_bitDialogBack);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);

	//Determine "screen" position
	m_rectChildWindowPos.left = ButWidthPix + CharW * 2;
	m_rectChildWindowPos.top = CharH / 2 + CharH;
	m_rectChildWindowPos.right = m_iWidthPix - CharW;
	m_rectChildWindowPos.bottom = m_iHeightPix - CharH;

	//Create "CONNECTED" and "NOT CONNECTED" overlays along top of dialog back. "CONNECTED" is 
	//made same size as "not connected" so drawing it will overlay "not connected"
	m_pGraph->GetStringPixelSize(L"NOT CONNECTED", &W, &H);
	int W2, H2;
	m_pGraph->GetStringPixelSize(L"CONNECTED", &W2, &H2);
	m_pGraph->MakeNewBitmap(W, H, &m_bitNotConnected);
	m_pGraph->SetOutputBitmap(&m_bitNotConnected);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);
	m_pGraph->SetTextColor(COL_RED_STATUS);
	m_pGraph->DrawTxt(0, 0, L"NOT CONNECTED");
	m_pGraph->MakeNewBitmap(W, H, &m_bitConnected);
	m_pGraph->SetOutputBitmap(&m_bitConnected);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);
	m_pGraph->SetTextColor(COL_GREEN_STATUS);
	m_pGraph->DrawTxt((W - W2) / 2, 0, L"CONNECTED");
	m_iConnectStatusX = m_rectChildWindowPos.left + (m_rectChildWindowPos.right - m_rectChildWindowPos.left - W) / 2;
	m_iConnectStatusY = CharH / 4;

	//Start with status "not connected"
	m_pGraph->SetOutputBitmap(&m_bitDialogBack);
	m_pGraph->DrawBitmapToOutputBitmap(&m_bitNotConnected, m_iConnectStatusX, m_iConnectStatusY);

	//Draw outline around screen
	m_pGraph->SetOutputBitmap(&m_bitDialogBack);
	m_pGraph->SetLineColor(COL_SCREEN_OUTLINE);
	m_pGraph->DrawLine(m_rectChildWindowPos.left - OUTLINE_THICKNESS, m_rectChildWindowPos.top - OUTLINE_THICKNESS,
		m_rectChildWindowPos.right + OUTLINE_THICKNESS, m_rectChildWindowPos.top - OUTLINE_THICKNESS, OUTLINE_THICKNESS);
	m_pGraph->LineTo(m_rectChildWindowPos.right + OUTLINE_THICKNESS, m_rectChildWindowPos.bottom + OUTLINE_THICKNESS);
	m_pGraph->LineTo(m_rectChildWindowPos.left - OUTLINE_THICKNESS, m_rectChildWindowPos.bottom + OUTLINE_THICKNESS);
	m_pGraph->LineTo(m_rectChildWindowPos.left - OUTLINE_THICKNESS, m_rectChildWindowPos.top - OUTLINE_THICKNESS);

	//Create and position each button -- we don't check for failure of bitmap creation because if there was a problem it would have 
	//probably failed on background creation above
	BitmapStruct *pOn, *pOff;
	m_pGraph->SetTextColor(COL_BUTTON_TEXT);

	//Connect 
	MakeButtonBitmaps(ButWidthPix, CharH, TEXT_CONNECT, &pOn, &pOff);
	m_butConnect.Create(m_pGraph, CharW, 0, ButWidthPix, CharH, pOn, pOff);
	m_butConnect.ButtonID = BUT_CONNECT;

	//Disconnect -- create in spame spot as Connect but set initial mode to invisible 
	MakeButtonBitmaps(ButWidthPix, CharH, TEXT_DISCONNECT, &pOn, &pOff);
	m_butDisconnect.Create(m_pGraph, CharW, 0, ButWidthPix, CharH, pOn, pOff);
	m_butDisconnect.ButtonID = BUT_DISCONNECT;
	m_butDisconnect.SetVisible(false);

	//Minimize
	MakeButtonBitmaps(CharW * 2, CharH, TEXT_MINIMIZE, &pOn, &pOff);   
	m_butMinimize.Create(m_pGraph, m_iWidthPix - 5 * CharW + CharW / 2, 0, CharW * 2, CharH, pOn, pOff);
	m_butMinimize.ButtonID = BUT_MIN;

	//Close
	MakeButtonBitmaps(CharW * 2, CharH, TEXT_CLOSE, &pOn, &pOff);
	m_butClose.Create(m_pGraph, m_iWidthPix - 2 * CharW, 0, CharW * 2, CharH, pOn, pOff);
	m_butClose.ButtonID = BUT_CLOSE;

	//Determine side row button spacing and X position
	int ButRowX = CharW;
	int ButRowH = ButHeightPix + CharH;
	int ButRowY = m_rectChildWindowPos.top +
		((m_rectChildWindowPos.bottom - m_rectChildWindowPos.top) - ButRowH * 4 - CharH) / 2;
	
	//Text button
	MakeButtonBitmaps(ButWidthPix, ButHeightPix, TEXT_TEXT, &pOn, &pOff);
	m_butText.Create(m_pGraph, ButRowX, ButRowY + 0 * ButRowH, ButWidthPix, ButHeightPix, pOn, pOff);   //0 = button 0 
	m_butText.ButtonID = BUT_TEXT;

	//ATC
	MakeButtonBitmaps(ButWidthPix, ButHeightPix, TEXT_ATC, &pOn, &pOff);
	m_butATC.Create(m_pGraph, ButRowX, ButRowY  + 1 * ButRowH, ButWidthPix, ButHeightPix, pOn, pOff);   // 1 = button 1 etc
	m_butATC.ButtonID = BUT_ATC;

	//Weather
	MakeButtonBitmaps(ButWidthPix, ButHeightPix, TEXT_WX, &pOn, &pOff);
	m_butWeather.Create(m_pGraph, ButRowX, ButRowY + 2 * ButRowH, ButWidthPix, ButHeightPix, pOn, pOff);
	m_butWeather.ButtonID = BUT_WX;

	//Flight plan
	MakeButtonBitmaps(ButWidthPix, ButHeightPix, TEXT_FP, &pOn, &pOff);
	m_butFlightPlan.Create(m_pGraph, ButRowX, ButRowY + 3 * ButRowH, ButWidthPix, ButHeightPix, pOn, pOff);
	m_butFlightPlan.ButtonID = BUT_FP;

	//Settings
	MakeButtonBitmaps(ButWidthPix, ButHeightPix, TEXT_SETTINGS, &pOn, &pOff);
	m_butSettings.Create(m_pGraph, ButRowX, ButRowY + 4 * ButRowH, ButWidthPix, ButHeightPix, pOn, pOff);
	m_butSettings.ButtonID = BUT_SETTINGS;

	///////////////////////////
	//Make minimized version -- start with minimized background
	m_iMinimizedWidthPix = CharW * 4;
	m_iMinimizedHeightPix = CharH * 2;
	m_pGraph->MakeNewBitmap(m_iMinimizedWidthPix, m_iMinimizedHeightPix, &m_bitMinimizedBack);
	m_pGraph->SetOutputBitmap(&m_bitMinimizedBack);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);
	m_pGraph->MakeNewBitmap(m_iMinimizedWidthPix, m_iMinimizedHeightPix, &m_bitMinimizedOutput);

	//Create red "status light" bitmap 
	m_pGraph->MakeNewBitmap(CharW, CharH, &m_bitRedCircle);
	m_pGraph->SetOutputBitmap(&m_bitRedCircle);
	m_pGraph->FillBitmapWithColor(RGB(0, 0, 0));
	m_bitRedCircle.bHasTransparency = true;
	m_bitRedCircle.TransparentColor = RGB(0, 0, 0);
	m_pGraph->SetTextColor(COL_RED_STATUS);
	m_pGraph->DrawTxt(0, 0, TEXT_CIRCLE);

	//Create green "status light" bitmap
	m_pGraph->MakeNewBitmap(CharW, CharH, &m_bitGreenCircle);
	m_pGraph->SetOutputBitmap(&m_bitGreenCircle);
	m_pGraph->FillBitmapWithColor(RGB(0, 0, 0));
	m_bitRedCircle.bHasTransparency = true;
	m_bitRedCircle.TransparentColor = RGB(0, 0, 0);
	m_pGraph->SetTextColor(COL_GREEN_STATUS);
	m_pGraph->DrawTxt(0, 0, TEXT_CIRCLE);

	//Create Maximize button -- special case because no real "right arrow" in arial font so we add 
	//arrow shaft

	m_pGraph->SetTextColor(COL_BUTTON_TEXT);
	MakeButtonBitmaps(CharW * 2, CharH, TEXT_MAXIMIZE, &pOn, &pOff);
	m_pGraph->SetLineColor(COL_BUTTON_TEXT);  
	m_pGraph->SetOutputBitmap(pOn);
	m_pGraph->DrawLine(CharW / 2 - 1, CharH / 2 - 1, CharW * 2 / 3, CharH / 2, CharH / 4);
	m_pGraph->SetOutputBitmap(pOff);
	m_pGraph->DrawLine(CharW / 2 - 1, CharH / 2 - 1, CharW * 2 / 3, CharH / 2, CharH / 4);
	m_butMaximize.Create(m_pGraph, m_iMinimizedWidthPix - CharW * 2, (m_iMinimizedHeightPix - CharH) / 2, CharW * 2, CharH, pOn, pOff);
	m_butMaximize.ButtonID = BUT_MAX;
	 
	return 1;
}

//Create both an "on" and "off" button for the given text in the given button size
int CMainDlg::MakeButtonBitmaps(int W, int H, WCHAR *pText, BitmapStruct **ppOn, BitmapStruct **ppOff)
{
	int StrWidthPix, h;
	bool bIsRightArrow = (wcscmp(pText, TEXT_MAXIMIZE) == 0 ? true : false);  //special case to move it over more

	*ppOn = new BitmapStruct;
	*ppOff = new BitmapStruct;
	m_pGraph->MakeNewBitmap(W, H, *ppOn);
	m_pGraph->SetOutputBitmap(*ppOn);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_ON);
	m_pGraph->GetStringPixelSize(pText, &StrWidthPix, &h);
	m_pGraph->DrawTxt((W - StrWidthPix) / 2 + (bIsRightArrow? StrWidthPix / 4 : 0), (H - h) / 2, pText);
	m_pGraph->MakeNewBitmap(W, H, *ppOff);
	m_pGraph->SetOutputBitmap(*ppOff);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_OFF);
	m_pGraph->DrawTxt((W - StrWidthPix) / 2 + (bIsRightArrow ? StrWidthPix / 4 : 0), (H - h) / 2, pText);
		
	return 1;
	 
}
//Create and initialize the child dialogs that go on the frame's "screen"
int CMainDlg::CreateScreenDialogs()
{
	int W = m_rectChildWindowPos.right - m_rectChildWindowPos.left;
	int H = m_rectChildWindowPos.bottom - m_rectChildWindowPos.top;
	int X = m_rectChildWindowPos.left;
	int Y = m_rectChildWindowPos.top;

	int res = m_dlgLogin.Initialize(this, &m_dlgFlightPlan, m_hFSXWin, m_pGraph, X, Y, W, H);
	res += m_dlgText.Initialize(this, m_hFSXWin, m_pGraph, X, Y, W, H);
	res += m_dlgATC.Initialize(this, m_hFSXWin, m_pGraph, X, Y, W, H);
	res += m_dlgWX.Initialize(this, m_hFSXWin, m_pGraph, X, Y, W, H);
	res += m_dlgSettings.Initialize(m_pGUI, this, m_pGraph, W, H);
	res += m_dlgFlightPlan.Initialize(this, &m_dlgLogin, m_hFSXWin, m_pGraph, X, Y, W, H);
	
	if (res != 6)
		return 0;

	//DEBUG Add controllers
	m_dlgATC.AddATC(L"LAX_A_CTR", L"John Doe (S1)", L"122.1", 35.0, -118.0, L"Attention this is the ATIS");
	m_dlgATC.AddATC(L"NY_B_CTR", L"Jane Smith (C1)", L"122.2", 34.0, -118.0, L"");
	m_dlgATC.AddATC(L"KLAX_A_CTR", L"John Doe (S1)", L"123.15", 34.0, -118.0, L"This is KLAX approach. This is a long string to see if the text will properly wrap around. Also to see if a large string works.");
	m_dlgATC.AddATC(L"LAX_A_CTR", L"John Doe (S1)", L"122.1", 35.0, -118.0, L"Attention this is the ATIS");
	m_dlgATC.AddATC(L"NY_B_CTR", L"Jane Smith (C1)", L"122.2", 34.0, -118.0, L"");
	m_dlgATC.AddATC(L"KLAX_A_CTR", L"John Doe (S1)", L"123.15", 34.0, -118.0, L"This is KLAX approach. This is a long string to see if the text will properly wrap around. Also to see if a large string works.");
	m_dlgATC.AddATC(L"LAX_A_APP", L"John Doe (S1)", L"122.1", 35.0, -118.0, L"Attention this is the ATIS");
	m_dlgATC.AddATC(L"NY_B_APP", L"Jane Smith (C1)", L"122.2", 34.0, -118.0, L"");
	m_dlgATC.AddATC(L"KLAX_A_APP", L"John Doe (S1)", L"123.15", 34.0, -118.0, L"This is KLAX approach. This is a long string to see if the text will properly wrap around. Also to see if a large string works.");
	m_dlgATC.AddATC(L"LAX_A_DEP", L"John Doe (S1)", L"122.1", 35.0, -118.0, L"Attention this is the ATIS");
	m_dlgATC.AddATC(L"NY_B_DEP", L"Jane Smith (C1)", L"122.2", 34.0, -118.0, L"");
	m_dlgATC.AddATC(L"KLAX_A_DEP", L"John Doe (S1)", L"123.15", 34.0, -118.0, L"This is KLAX approach. This is a long string to see if the text will properly wrap around. Also to see if a large string works.");
	m_dlgATC.AddATC(L"LAX_A_TWR", L"John Doe (S1)", L"122.1", 35.0, -118.0, L"Attention this is the ATIS");
	m_dlgATC.AddATC(L"NY_B_TWR", L"Jane Smith (C1)", L"122.2", 34.0, -118.0, L"");
	m_dlgATC.AddATC(L"KLAX_A_TWR", L"John Doe (S1)", L"123.15", 34.0, -118.0, L"This is KLAX approach. This is a long string to see if the text will properly wrap around. Also to see if a large string works.");
	m_dlgATC.AddATC(L"LAX_A_GND", L"John Doe (S1)", L"122.1", 35.0, -118.0, L"Attention this is the ATIS");
	m_dlgATC.AddATC(L"NY_B_GND", L"Jane Smith (C1)", L"122.2", 34.0, -118.0, L"");
	m_dlgATC.AddATC(L"KLAX_A_GND", L"John Doe (S1)", L"123.15", 34.0, -118.0, L"This is KLAX approach. This is a long string to see if the text will properly wrap around. Also to see if a large string works.");
	m_dlgATC.AddATC(L"LAX_A_CLR", L"John Doe (S1)", L"122.1", 35.0, -118.0, L"Attention this is the ATIS");
	m_dlgATC.AddATC(L"NY_B_CLR", L"Jane Smith (C1)", L"122.2", 34.0, -118.0, L"");
	m_dlgATC.AddATC(L"KLAX_A_CTR", L"John Doe (S1)", L"123.15", 34.0, -110.0, L"This is KLAX Center.\n This should be a new line and is a long string to see if the text will properly wrap around.\n\nShould be extra space with above line.\nNew Line\nNewLine2\nNew Line3 and this is a really big line because I want it to go to the end of the page which is over 40 columns long and I don't know how many\nNewLine4");
	m_dlgATC.AddATC(L"LAX_A_APP", L"John Doe (S1)", L"122.1", 35.0, -118.0, L"Attention this is the ATIS");
	m_dlgATC.AddATC(L"NY_B_OBS", L"Jane Smith (C1)", L"122.2", 34.0, -118.0, L"");
	m_dlgATC.AddATC(L"KLAX_A_APP", L"John Doe (S1)", L"123.15", 34.0, -118.0, L"This is KLAX approach. This is a long string to see if the text will properly wrap around. Also to see if a large string works.");
	m_dlgATC.SetUserPosition(34, -119.0);

	//DEBUG add servers
	m_dlgLogin.AddServer(L"VAT_W", L"Los Angeles, USA");
	m_dlgLogin.AddServer(L"VAT_N", L"Seattle, USA");
	m_dlgLogin.AddServer(L"CANADA", L"Vancouver, CA");
	m_dlgLogin.AddServer(L"VAT_E", L"New Jersey, USA");
	m_dlgLogin.AddServer(L"AUSTRALIA", L"Australia");
	m_dlgLogin.AddServer(L"GERMANY1", L"Germany");
	m_dlgLogin.AddServer(L"UK1", L"London, UK");
	m_dlgLogin.AddServer(L"VAT_W", L"Los Angeles, USA");
	m_dlgLogin.AddServer(L"VAT_N", L"Seattle, USA");
	m_dlgLogin.AddServer(L"CANADA", L"Vancouver, CA");
	m_dlgLogin.AddServer(L"VAT_E", L"New Jersey, USA");
	m_dlgLogin.AddServer(L"AUSTRALIA", L"Australia");
	m_dlgLogin.AddServer(L"GERMANY1", L"Germany");
	m_dlgLogin.AddServer(L"UK1", L"London, UK");
	m_dlgLogin.AddServer(L"VAT_W", L"Los Angeles, USA");
	m_dlgLogin.AddServer(L"VAT_N", L"Seattle, USA");
	return 1;
}

//Adjust screen X,Y so dialog fits on screen, return true if adjusted
bool CMainDlg::ClampDialogToScreen()
{
	bool bClampedX = true, bClampedY = true;

	if (m_iScreenX < 0)
		m_iScreenX = 0;
	else if (m_iScreenX >(m_rectFSXWin.right - m_rectFSXWin.left - m_iWidthPix))
		m_iScreenX = m_rectFSXWin.right - m_rectFSXWin.left - m_iWidthPix;
	else
		bClampedX = false;

	if (m_iScreenY < 0)
		m_iScreenY = 0;
	else if (m_iScreenY >(m_rectFSXWin.bottom - m_rectFSXWin.top - m_iHeightPix))
		m_iScreenY = m_rectFSXWin.bottom - m_rectFSXWin.top - m_iHeightPix;
	else
		bClampedY = false;

	if (bClampedX || bClampedY)
		return true;
	return false;
}

////////////////
//Calls from server

//Connected to server (bConnected TRUE), or disconnected (bConnected false), bIsError
//true to show as an error, e.g. can cleanly connect, fail to connect, cleanly disconnect,
//disconnect from error all with this call
int CMainDlg::OnServerConnected(bool bConnected, WCHAR *ConnectionText, bool bIsError)
{
	//Notify certain dialogs that need to know
	m_dlgLogin.IndicateConnected(bConnected);
	m_dlgFlightPlan.IndicateConnected(bConnected);
	m_dlgATC.IndicateConnected(bConnected);
	m_dlgText.IndicateConnected(bConnected);
	m_dlgWX.IndicateConnected(bConnected);

	if (bConnected)
	{
		m_Status = STAT_GREEN;
		m_butConnect.SetVisible(false);
		m_butDisconnect.SetVisible(true);
		m_pGraph->SetOutputBitmap(&m_bitDialogBack);
		m_pGraph->DrawBitmapToOutputBitmap(&m_bitConnected, m_iConnectStatusX, m_iConnectStatusY);
		m_butDisconnect.Draw();
	}
	else
	{
		m_Status = STAT_RED;
		m_butConnect.SetVisible(true);
		m_butDisconnect.SetVisible(false);
		m_pGraph->SetOutputBitmap(&m_bitDialogBack);
		m_pGraph->DrawBitmapToOutputBitmap(&m_bitNotConnected, m_iConnectStatusX, m_iConnectStatusY);
		m_butConnect.Draw();
	}
	if (bIsError)
		m_dlgText.AddText(ConnectionText, COL_ERROR_TEXT);
	else
		m_dlgText.AddText(ConnectionText, COL_SERVER_TEXT);
	ProcessButtonClick(m_butText.ButtonID);
	DrawWholeDialogToDC();
	return 1;
}

////////////////
// Callbacks from child dialogs

//"Connect" button on Login dialog screen pressed
WINMSG_RESULT CMainDlg::OnLoginConnectPressed(WCHAR *ServerName, WCHAR *UserName, WCHAR *ID,
	WCHAR *Password, WCHAR *Callsign, WCHAR *ACType, bool bIsObserver)
{
	m_pGUI->UserReqConnection(ServerName, UserName, ID, Password, Callsign, ACType, bIsObserver);

	return WINMSG_HANDLED_REDRAW_US;
}

WINMSG_RESULT CMainDlg::OnLoginDisconnectPressed()
{
	m_pGUI->UserReqDisconnect();

	return WINMSG_HANDLED_REDRAW_US;
}

//Indicate to grab keyboard input (or release)
int CMainDlg::GetKeyboardInput(bool bNeedKeyboard)
{
	if (m_pGUI)
		m_pGUI->IndicateNeedKeyboard(bNeedKeyboard);
	return 1;
}

//User wants to send this text to server 
int CMainDlg::OnSendText(WCHAR *pText)
{
	//DEBUG
	//For now just echo back
	m_dlgText.AddText(pText);
	return 1;
}

//User requesting METAR weather for this station
int CMainDlg::OnRequestWeather(WCHAR *pStation)
{
	//DEBUG
	m_dlgWX.SetText(L"KLAX 022112 22012G20 20/12 -RA BR SCT20 BKN40 BKN70 OVC80 RMK LTCGCC DIST E");
	return 1;
}

//Flight plan "send" button pressed
WINMSG_RESULT CMainDlg::OnSendFlightPlanPressed()
{

	return WINMSG_HANDLED_NO_REDRAW;
}
