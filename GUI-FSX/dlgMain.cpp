#include "stdafx.h"
#include "Dialogs.h"

#define FONT_SIZE 8
#define FONT_NAME L"Arial"
#define TEXT_WIDTH_CHAR 32    //text/dialog-screen within the dialog. Whole dialog is this max width plus 2 left and 2 right char width borders
#define TEXT_HEIGHT_CHAR 10   
#define BUTTON_WIDTH_CHAR 8   //should be longest button string below, plus 2 chars
#define BUTTON_HEIGHT_CHAR 1
#define OUTLINE_THICKNESS 1

#define COL_BUTTON_OFF RGB(0,64,64)
#define COL_BUTTON_ON RGB(0,128,128)
#define COL_BUTTON_TEXT RGB(220, 220, 220)
#define COL_DLG_BACK RGB(5,5,50)
#define COL_SCREEN_OUTLINE RGB(200, 200, 200)

#define TEXT_CONNECT L"Connect"
#define TEXT_DISCONNECT L"Disconnect"
#define TEXT_TEXT L"Text"
#define TEXT_FP L"Flight Plan"
#define TEXT_ATC L"ATC"
#define TEXT_WX L"Weather"
#define TEXT_SETTINGS L"Settings"
#define TEXT_MINIMIZE L"_"     //assumed 1 character
#define TEXT_CLOSE L"X"        //1 char

CMainDlg::CMainDlg() : m_pGUI(NULL), m_pGraph(NULL), m_iScreenX(0), m_iScreenY(0) , m_iCursorScreenX(0),
	m_iCursorScreenY(0), m_iWidthPix(0), m_iHeightPix(0), m_bDraggingDialog(false), m_pFullscreenDevice(NULL),
	m_bInWindowedMode(true)
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
	if (message == WM_MOUSEMOVE || message == WM_LBUTTONDOWN || message == WM_LBUTTONUP)
	{
		m_iCursorScreenX = GET_X_LPARAM(lParam);
		m_iCursorScreenY = GET_Y_LPARAM(lParam);
	}

	//Check if we lost capture (could happen if we are dragging dialog because we 
	//SetCapture to get the mouse)
	if (message == WM_CAPTURECHANGED)
	{
		m_bDraggingDialog = false;
		return WINMSG_NOT_HANDLED;
	}

	//See if mouse within dialog
	bool bFullRedraw = false, bHandled = false, bCursorWithinDialog = false;
	int ret;
	if ((m_iCursorScreenX >= m_iScreenX && m_iCursorScreenX <= (m_iScreenX + m_iWidthPix) &&
		m_iCursorScreenY >= m_iScreenY && m_iCursorScreenY <= (m_iScreenY + m_iHeightPix)))
		bCursorWithinDialog = true;

	//Special case--in windowed mode we get both PARENTNOTIFY and MOUSEACTIVATE if LButton down 
	//-- only handle the first and tell FSX to ignore this one
	if (bCursorWithinDialog && message == WM_MOUSEACTIVATE && (HIWORD(lParam) == WM_LBUTTONDOWN || 
		HIWORD(lParam == WM_RBUTTONDOWN)))
		return WINMSG_HANDLED_NO_REDRAW;

	//If within dialog, first check all our buttons
	if (!m_bDraggingDialog && bCursorWithinDialog)
	{
		//Convert to X&Y within the dialog
		int DlgMouseX = m_iCursorScreenX - m_iScreenX;
		int DlgMouseY = m_iCursorScreenY - m_iScreenY;

		//First check our dialog's buttons
		for (size_t i = 0; i < m_apButtons.size() && !bHandled; i++)
		{
			if (m_apButtons[i]->IsWithin(DlgMouseX, DlgMouseY))
			{
				ret = m_apButtons[i]->WindowsMessage(message, wParam, lParam);
				if (ret == WINMSG_HANDLED_NO_REDRAW)
					return ret;
				if (ret == WINMSG_HANDLED_REDRAW_US)
				{
					//Redraw just this button and update surface
					m_pGraph->SetOutputBitmap(&m_bitCurrentOutput);
					m_apButtons[i]->Draw();
					m_pGraph->CopyBitmapDCToSurface(&m_bitCurrentOutput);
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
	
	//If not handled by buttons, forward to screen dialogs. Done outside of above loop in case of WM_CHAR
	if (!bHandled && !m_bDraggingDialog && (bCursorWithinDialog || message == WM_CHAR))
	{
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
					m_pGraph->SetOutputBitmap(&m_bitCurrentOutput);
					m_apChildDialogs[i]->Draw(NULL);
					m_pGraph->CopyBitmapDCToSurface(&m_bitCurrentOutput);
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

	//If cursor within dialog, since it wasn't handled by buttons or controls, check if starting a drag 
	if (bCursorWithinDialog)
	{
		//Starting drag?
		if (message == WM_LBUTTONDOWN)
		{
			m_bDraggingDialog = true;
			m_iLastDragScreenX = m_iCursorScreenX;
			m_iLastDragScreenY = m_iCursorScreenY;
			SetCapture(m_hFSXWin);
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

	return WINMSG_NOT_HANDLED;
}

int CMainDlg::Update()
{

	return 0;
}

int CMainDlg::Draw(IDirect3DDevice9* pDevice)
{
	assert(m_pGraph);
	if (!m_pGraph)
		return 0;

	if (!pDevice)
		m_pGraph->DrawBitmapToOutputBitmap(&m_bitCurrentOutput, 0, 0);
	else
		m_pGraph->DrawBitmapSurfaceOnDevice(&m_bitCurrentOutput, pDevice, m_iScreenX, m_iScreenY);

	return 1;
}

//Called if something changes -- redraw the dialog's in-memory m_bitCurrentOutput bitmap and update 
//the corresponding D3D surface
int CMainDlg::DrawWholeDialogToDC()
{
	m_pGraph->SetOutputBitmap(&m_bitCurrentOutput);

	//start with background
	m_pGraph->DrawBitmapToOutputBitmap(&m_bitDialogBack, 0, 0);

	//Draw each frame button to current output
	size_t i;
	for (i = 0; i < m_apButtons.size(); i++)
		m_apButtons[i]->Draw();

	//Draw "screen" dialog to current output -- each one only draws if it's open
	for (i = 0; i < m_apChildDialogs.size(); i++)
		m_apChildDialogs[i]->Draw(NULL);

	//Update Direct3D surface
	m_pGraph->CopyBitmapDCToSurface(&m_bitCurrentOutput);

	return 1;
}

int CMainDlg::Open()
{
	m_iScreenX = 0;
	m_iScreenY = 200;
	
	return 0;
}

int CMainDlg::Close()
{

	return 0;
}

int CMainDlg::Shutdown()
{

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
		//Determine new screen position
		float fXPos = (float)m_iScreenX / (float)(m_rectFSXWin.right - m_rectFSXWin.left);
		float fYPos = (float)m_iScreenY / (float)(m_rectFSXWin.bottom - m_rectFSXWin.top);
		m_iScreenX = (int)(fXPos * (float)(wi.rcClient.right - wi.rcClient.left));
		m_iScreenY = (int)(fYPos * (float)(wi.rcClient.bottom - wi.rcClient.top));
		if (m_bDraggingDialog)
		{
			m_bDraggingDialog = false;
			ReleaseCapture();
		}
		m_bInWindowedMode = true;
	}

	memcpy(&m_rectFSXWin, &wi.rcWindow, sizeof(RECT));
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

		if (m_bDraggingDialog)
		{
			m_bDraggingDialog = false;
			ReleaseCapture();
		}
		m_bInWindowedMode = false;
	}
			
	//Record new position and size
	m_pFullscreenDevice = pFullscreenDevice;  
	IDirect3DSwapChain9 *pISwapChain;
	if (FAILED(pFullscreenDevice->GetSwapChain(0, &pISwapChain)))
	{
		assert(0);
		return 0;
	}
	D3DPRESENT_PARAMETERS PP;
	if (FAILED(pISwapChain->GetPresentParameters(&PP)))
	{
		assert(0);
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


int CMainDlg::Initialize(CFSXGUI *pGUI, C2DGraphics *pGraph, HWND hFSXWin)
{
	m_pGUI = pGUI;
	m_pGraph = pGraph;

	//Record initial window position
	WINDOWINFO wi;
	wi.cbSize = sizeof(WINDOWINFO);
	GetWindowInfo(hFSXWin, &wi);
	memcpy(&m_rectFSXWin, &wi.rcWindow, sizeof(RECT));
	m_bInWindowedMode = true;
	m_hFSXWin = hFSXWin;
	
	if (!CreateFrame() || !CreateScreenDialogs())
		return 0;

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

	DrawWholeDialogToDC();
	return 1;
}

////////////////////////
// Internal

//Create frame bitmap and button controls. The frame holds buttons on top (connect/disconnect) and five on bottom,
//then has a "screen" area where child dialogs appear. 
int CMainDlg::CreateFrame()
{
	//Find and set the font
	int CharW, CharH;
	if (!m_pGraph->FindBestFont(FONT_NAME, FONT_SIZE, true, false, false, &m_hFont))
		return 0;
	m_pGraph->SetFont(m_hFont);
	m_pGraph->SetTextColor(COL_BUTTON_TEXT);
	if (!m_pGraph->GetStringPixelSize(_T("M"), &CharW, &CharH))  //M = widest of all characters
		return 0;

	//Create background (and original "current output")
	int ButWidthPix = CharW * BUTTON_WIDTH_CHAR;
	int ButHeightPix = CharH * BUTTON_HEIGHT_CHAR;
	m_iWidthPix = ButWidthPix + CharW * 2 + CharW * TEXT_WIDTH_CHAR;
	m_iHeightPix = CharH * (TEXT_HEIGHT_CHAR + 2) + CharH / 2;
	if (!m_pGraph->MakeNewBitmap(m_iWidthPix, m_iHeightPix, &m_bitDialogBack))
		return 0;
	m_pGraph->MakeNewBitmap(m_iWidthPix, m_iHeightPix, &m_bitCurrentOutput);
	m_pGraph->SetOutputBitmap(&m_bitDialogBack);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);

	//Determine "screen" position
	m_rectChildWindowPos.left = ButWidthPix + CharW * 2;
	m_rectChildWindowPos.top = CharH / 2 + ButHeightPix;
	m_rectChildWindowPos.right = m_iWidthPix - CharW;
	m_rectChildWindowPos.bottom = m_iHeightPix - CharH;

	//Draw outline around screen
	m_pGraph->SetLineColor(COL_SCREEN_OUTLINE);
	m_pGraph->DrawLine(m_rectChildWindowPos.left - OUTLINE_THICKNESS, m_rectChildWindowPos.top - OUTLINE_THICKNESS,
		m_rectChildWindowPos.right + OUTLINE_THICKNESS, m_rectChildWindowPos.top - OUTLINE_THICKNESS, OUTLINE_THICKNESS);
	m_pGraph->LineTo(m_rectChildWindowPos.right + OUTLINE_THICKNESS, m_rectChildWindowPos.bottom + OUTLINE_THICKNESS);
	m_pGraph->LineTo(m_rectChildWindowPos.left - OUTLINE_THICKNESS, m_rectChildWindowPos.bottom + OUTLINE_THICKNESS);
	m_pGraph->LineTo(m_rectChildWindowPos.left - OUTLINE_THICKNESS, m_rectChildWindowPos.top - OUTLINE_THICKNESS);

	//Create and position each button -- we don't check for failure of bitmap creation because if there was a problem it would have 
	//probably failed on background creation above
	BitmapStruct *pOn, *pOff;

	//Connect (on and off) -- position one char over, one char down from top-left (same with disconnect)
	MakeButtonBitmaps(ButWidthPix, ButHeightPix, TEXT_CONNECT, &pOn, &pOff);
	m_butConnect.Create(m_pGraph, CharW, 0, ButWidthPix, ButHeightPix, pOn, pOff);

	//Disconnect
	MakeButtonBitmaps(ButWidthPix, ButHeightPix, TEXT_DISCONNECT, &pOn, &pOff);
	m_butDisconnect.Create(m_pGraph, CharW, 0, ButWidthPix, ButHeightPix, pOn, pOff);

	//Minimize
	MakeButtonBitmaps(CharW * 2, ButHeightPix, TEXT_MINIMIZE, &pOn, &pOff);
	m_butMinimize.Create(m_pGraph, m_iWidthPix - 5 * CharW + CharW / 2, 0, CharW * 2, ButHeightPix, pOn, pOff);

	//Close
	MakeButtonBitmaps(CharW * 2, ButHeightPix, TEXT_CLOSE, &pOn, &pOff);
	m_butClose.Create(m_pGraph, m_iWidthPix - 2 * CharW, 0, CharW * 2, ButHeightPix, pOn, pOff);
	
	//Determine side row button spacing and X position
	int ButRowX = CharW;
	int ButRowH = ButHeightPix + CharH;
	int ButRowY = m_rectChildWindowPos.top +
		((m_rectChildWindowPos.bottom - m_rectChildWindowPos.top) - ButRowH * 4 - CharH) / 2;
	
	//Text button
	MakeButtonBitmaps(ButWidthPix, ButHeightPix, TEXT_TEXT, &pOn, &pOff);
	m_butText.Create(m_pGraph, ButRowX, ButRowY + 0 * ButRowH, ButWidthPix, ButHeightPix, pOn, pOff);   //0 = button 0 

	//ATC
	MakeButtonBitmaps(ButWidthPix, ButHeightPix, TEXT_ATC, &pOn, &pOff);
	m_butATC.Create(m_pGraph, ButRowX, ButRowY  + 1 * ButRowH, ButWidthPix, ButHeightPix, pOn, pOff);   // 1 = button 1 etc

	//Weather
	MakeButtonBitmaps(ButWidthPix, ButHeightPix, TEXT_WX, &pOn, &pOff);
	m_butWeather.Create(m_pGraph, ButRowX, ButRowY + 2 * ButRowH, ButWidthPix, ButHeightPix, pOn, pOff);

	//Flight plan
	MakeButtonBitmaps(ButWidthPix, ButHeightPix, TEXT_FP, &pOn, &pOff);
	m_butFlightPlan.Create(m_pGraph, ButRowX, ButRowY + 3 * ButRowH, ButWidthPix, ButHeightPix, pOn, pOff);

	//Settings
	MakeButtonBitmaps(ButWidthPix, ButHeightPix, TEXT_SETTINGS, &pOn, &pOff);
	m_butSettings.Create(m_pGraph, ButRowX, ButRowY + 4 * ButRowH, ButWidthPix, ButHeightPix, pOn, pOff);

	return 1;
}

//Create both an "on" and "off" button for the given text in the given button size
int CMainDlg::MakeButtonBitmaps(int W, int H, WCHAR *pText, BitmapStruct **ppOn, BitmapStruct **ppOff)
{
	int StrWidthPix, h;

	*ppOn = new BitmapStruct;
	*ppOff = new BitmapStruct;
	m_pGraph->MakeNewBitmap(W, H, *ppOn);
	m_pGraph->SetOutputBitmap(*ppOn);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_ON);
	m_pGraph->GetStringPixelSize(pText, &StrWidthPix, &h);
	m_pGraph->DrawTxt((W - StrWidthPix) / 2, (H - h) / 2, pText);
	m_pGraph->MakeNewBitmap(W, H, *ppOff);
	m_pGraph->SetOutputBitmap(*ppOff);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_OFF);
	m_pGraph->DrawTxt((W - StrWidthPix) / 2, (H - h) / 2, pText);

	return 1;

}
//Create and initialize the child dialogs that go on the frame's "screen"
int CMainDlg::CreateScreenDialogs()
{
	int W = m_rectChildWindowPos.right - m_rectChildWindowPos.left;
	int H = m_rectChildWindowPos.bottom - m_rectChildWindowPos.top;

	int res = m_dlgLogin.Initialize(m_pGUI, this, m_pGraph, W, H);
	res += m_dlgText.Initialize(m_pGUI, this, m_pGraph, W, H);
	res += m_dlgATC.Initialize(m_pGUI, this, m_pGraph, W, H);
	res += m_dlgWX.Initialize(m_pGUI, this, m_pGraph, W, H);
	res += m_dlgSettings.Initialize(m_pGUI, this, m_pGraph, W, H);
	res += m_dlgFlightPlan.Initialize(m_pGUI, this, m_pGraph, W, H);
	
	if (res != 6)
		return 0;
	return 1;
}
//Adjust screen X,Y so dialog fits on screen, return true if adjusted
bool CMainDlg::ClampDialogToScreen()
{
	bool bClamped = true;
	if (m_iScreenX < 0)
		m_iScreenX = 0;
	else if (m_iScreenX >(m_rectFSXWin.right - m_rectFSXWin.left - m_iWidthPix))
		m_iScreenX = m_rectFSXWin.right - m_rectFSXWin.left - m_iWidthPix;
	else if (m_iScreenY < 0)
		m_iScreenY = 0;
	else if (m_iScreenY >(m_rectFSXWin.bottom - m_rectFSXWin.top - m_iHeightPix))
		m_iScreenY = m_rectFSXWin.bottom - m_rectFSXWin.top - m_iHeightPix;
	else
		bClamped = false;
	return bClamped;
}
