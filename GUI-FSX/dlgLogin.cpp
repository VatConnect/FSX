#include "stdafx.h"
#include "Dialogs.h"

#define LOGIN_FIELDNAME_FONT L"Arial"                  //field names
#define LOGIN_DATAFIELDS_FONT L"Lucida Console"        //user-entered content

CLoginDlg::CLoginDlg() : m_bOpen(false), m_hFieldnameFont(NULL), m_hDataFont(NULL), m_pEditWithFocus(NULL)
{
}

CLoginDlg::~CLoginDlg()
{
}

int CLoginDlg::Initialize(CMainDlg *pMainDlg, HWND hWnd, 
	C2DGraphics *pGraph, int X, int Y, int WidthPix, int HeightPix)
{
	int W, H;

	m_pGraph = pGraph;
	m_iX = X;
	m_iY = Y;
	m_iWidthPix = WidthPix;
	m_iHeightPix = HeightPix;
	m_hWnd = hWnd;
	m_pMainDlg = pMainDlg;

	m_pGraph->FindBestFont(LOGIN_FIELDNAME_FONT, FONT_SIZE, false, false, false, &m_hFieldnameFont);
	m_pGraph->FindBestFont(LOGIN_DATAFIELDS_FONT, FONT_SIZE, true, false, false, &m_hDataFont);

	//Get character sizes for fieldname and data fields (height's probably the same anyway)
	int FWidthPix, FHeightPix, TWidthPix, THeightPix;
	m_pGraph->SetFont(m_hDataFont);
	m_pGraph->GetStringPixelSize(L"M", &TWidthPix, &THeightPix);
	m_pGraph->SetFont(m_hFieldnameFont);
	m_pGraph->GetStringPixelSize(L"M", &FWidthPix, &FHeightPix);

	//Determine margins to center our text (10 lines worth)
	int TopMarginPix = (m_iHeightPix - (9 * FHeightPix)) / 2;
	int EditXPix;

	//////////////
	//Make background bitmap and controls
	m_pGraph->MakeNewBitmap(WidthPix, HeightPix, &m_bitBack);
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);
	int YPos = TopMarginPix;
	int LineSpacePix = FHeightPix * 4 / 3;

	//Connect As...
	//First make button selected and not-selected bitmaps -- filled box and outlined box
	m_pGraph->MakeNewBitmap(TWidthPix + 4, THeightPix + 2, &m_bitButSelected);
	m_pGraph->MakeNewBitmap(TWidthPix + 4, THeightPix + 2, &m_bitButNotSelected);
	m_pGraph->SetOutputBitmap(&m_bitButSelected);
	m_pGraph->FillBitmapWithColor(COL_DLG_TEXT);
	m_pGraph->SetLineColor(COL_DLG_BACK);
	m_pGraph->DrawLine(1, 1, TWidthPix + 2, 1, 1);
	m_pGraph->LineTo(TWidthPix + 2, THeightPix);
	m_pGraph->LineTo(1, THeightPix);
	m_pGraph->LineTo(1, 1);
	m_pGraph->SetOutputBitmap(&m_bitButNotSelected);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);
	m_pGraph->SetLineColor(COL_DLG_TEXT);
	m_pGraph->DrawLine(0, 0, TWidthPix + 3, 0, 1);
	m_pGraph->LineTo(TWidthPix + 3, THeightPix + 1);
	m_pGraph->LineTo(0, THeightPix + 1 );
	m_pGraph->LineTo(0, 0);
 
	//Connect As text
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->DrawTxt(FWidthPix * 6, YPos, L"Connect as :");
	int XPos = FWidthPix * 13 + FWidthPix /  2;
	m_butPilot.Create(m_pGraph, XPos, YPos - 1, TWidthPix + 4, THeightPix + 4, &m_bitButSelected,
		&m_bitButNotSelected, false);
	XPos += 2 * TWidthPix;

	//Pilot
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->GetStringPixelSize(L"Pilot", &EditXPix, &H);
	m_pGraph->DrawTxt(XPos, YPos, L"Pilot");
	XPos += EditXPix + 2 * TWidthPix;
	m_butObserver.Create(m_pGraph, XPos, YPos - 1, TWidthPix + 4, THeightPix + 4, &m_bitButSelected,
		&m_bitButNotSelected, false);
	
	//Observer
	XPos += 2 * TWidthPix;
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->DrawTxt(XPos, YPos, L"Observer");

	YPos += LineSpacePix + LineSpacePix / 2;
	m_pGraph->SetFont(m_hFieldnameFont);
	m_pGraph->SetTextColor(COL_DLG_TEXT);

	//Server edit box
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->GetStringPixelSize(L"Server :", &EditXPix, &H);
	m_pGraph->DrawTxt(FWidthPix, YPos, L"Server :");
	XPos = EditXPix + FWidthPix * 3 / 2;
	m_editServer.Create(m_pGraph, XPos, YPos + 2, TWidthPix * 16 + 2, THeightPix + 1,
		COL_DLG_TEXT, COL_EDITBOX_BACK, m_hDataFont);
	XPos += TWidthPix * 16 + FWidthPix;

	//Server Select button
	BitmapStruct *pOn = new BitmapStruct;
	BitmapStruct *pOff = new BitmapStruct;
	m_pGraph->SetFont(m_hFieldnameFont);
	m_pGraph->GetStringPixelSize(L" Select... ", &W, &H);
	m_pGraph->MakeNewBitmap(W, H, pOn);
	m_pGraph->SetOutputBitmap(pOn);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_ON);
	m_pGraph->SetTextColor(COL_BUTTON_TEXT);
	m_pGraph->DrawTxt(0, 0, L" Select...");
	m_pGraph->MakeNewBitmap(W, H, pOff);
	m_pGraph->SetOutputBitmap(pOff);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_OFF);
	m_pGraph->DrawTxt(0, 0, L" Select...");
	m_butServerSelect.Create(m_pGraph, XPos, YPos, W, H, pOn, pOff);
	
	YPos += LineSpacePix;

	//Name 
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->GetStringPixelSize(L"Name :", &EditXPix, &H);
	m_pGraph->DrawTxt(FWidthPix, YPos, L"Name :");
	m_editName.Create(m_pGraph, EditXPix + FWidthPix * 3 / 2, YPos + 2, TWidthPix * 29 + 2 + 4, THeightPix + 1, 
		COL_DLG_TEXT, COL_EDITBOX_BACK, m_hDataFont);
	YPos += LineSpacePix;

	//VATSIM ID 
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->GetStringPixelSize(L"VATSIM ID :", &EditXPix, &H);
	m_pGraph->DrawTxt(FWidthPix, YPos, L"VATSIM ID :");
	m_editID.Create(m_pGraph, EditXPix + FWidthPix * 3 / 2, YPos + 2, TWidthPix * 9 + 2, THeightPix + 1, 
		COL_DLG_TEXT, COL_EDITBOX_BACK, m_hDataFont);
	XPos = EditXPix + FWidthPix * 3 / 2 + TWidthPix * 9 + FWidthPix;
	 
	//Password 
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->GetStringPixelSize(L"Password :", &EditXPix, &H);
	m_pGraph->DrawTxt(XPos, YPos, L"Password :");
	m_editPassword.Create(m_pGraph, XPos + EditXPix + FWidthPix / 2, YPos + 2, TWidthPix * 8 + 2 + 1, THeightPix + 1, 
		COL_DLG_TEXT, COL_EDITBOX_BACK, m_hDataFont, true);
	YPos += LineSpacePix;

	//Callsign
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->GetStringPixelSize(L"Callsign :", &EditXPix, &H);
	m_pGraph->DrawTxt(FWidthPix, YPos, L"Callsign :");
	m_editCallsign.Create(m_pGraph, EditXPix + FWidthPix * 3 / 2, YPos + 2, TWidthPix * 9 + 2, THeightPix + 1,
		COL_DLG_TEXT, COL_EDITBOX_BACK, m_hDataFont);
	m_editCallsign.SetMaxChars(8); 

	//ICAO type (same line as above)
	XPos = EditXPix + FWidthPix + TWidthPix * 9 + FWidthPix;  //Start of next text, 9 same as 2 lines up
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->GetStringPixelSize(L"ICAO Aircraft Type :", &EditXPix, &H);
	m_pGraph->DrawTxt(XPos, YPos, L"ICAO Aircraft Type :");
	m_editACType.Create(m_pGraph, XPos + EditXPix + FWidthPix / 2, YPos + 2, TWidthPix * 5 + 2, THeightPix + 1,
		COL_DLG_TEXT, COL_EDITBOX_BACK, m_hDataFont);
	m_editACType.SetMaxChars(4);

	//Create cover bitmap to cover Callsign and ICAO text when "observer" button selected
	m_pGraph->MakeNewBitmap(WidthPix, FHeightPix + 2, &m_bitCover);
	m_pGraph->SetOutputBitmap(&m_bitCover);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);
	m_iCoverX = 0;
	m_iCoverY = YPos;
	
	YPos += FHeightPix;  
	 
	//Make Connect button
	pOn = new BitmapStruct;
	pOff = new BitmapStruct;
	m_pGraph->SetFont(m_hFieldnameFont);
	m_pGraph->GetStringPixelSize(L"  Connect  ", &W, &H);
	m_pGraph->MakeNewBitmap(W, H, pOn);
	m_pGraph->SetOutputBitmap(pOn);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_ON);
	m_pGraph->SetTextColor(COL_BUTTON_TEXT);
	m_pGraph->DrawTxt(0, 0, L"  Connect");
	m_pGraph->MakeNewBitmap(W, H, pOff);
	m_pGraph->SetOutputBitmap(pOff);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_OFF);
	m_pGraph->DrawTxt(0, 0, L"  Connect");
	m_butConnect.Create(m_pGraph, (m_iWidthPix - W) / 2, YPos + (m_iHeightPix - YPos) / 2 - H / 2, W, H, pOn, pOff);

	

	//Create array of pointers to our edit boxes
	m_apEditBoxes.push_back(&m_editServer);
	m_apEditBoxes.push_back(&m_editName);
	m_apEditBoxes.push_back(&m_editID);
	m_apEditBoxes.push_back(&m_editPassword);
	m_apEditBoxes.push_back(&m_editCallsign);
	m_apEditBoxes.push_back(&m_editACType);

	//Create output bitmap 
	m_pGraph->MakeNewBitmap(WidthPix, HeightPix, &m_bitCurrent);

	//Set initial control states
	m_butPilot.SetOn(true);
	m_butObserver.SetOn(false);
	m_butConnect.SetOn(false);
	m_butServerSelect.SetOn(false);
	
	DrawWholeDialogToOutput();
	
	return 1;
}   

int CLoginDlg::DrawWholeDialogToOutput()
{
	m_pGraph->SetOutputBitmap(&m_bitCurrent);
	m_pGraph->DrawBitmapToOutputBitmap(&m_bitBack, 0, 0);
	m_butPilot.Draw();
	m_butObserver.Draw();
	m_butConnect.Draw();
	m_butServerSelect.Draw();
	m_editServer.Draw();
	m_editName.Draw();
	m_editID.Draw();
	m_editPassword.Draw();
	m_editCallsign.Draw();
	m_editACType.Draw();
	return 1;
}
/////////////
//Base class

int CLoginDlg::WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	static const DWORD CharFilter = ALT_KEY_PRESSED | PREV_PRESSED; 

	if (!m_bOpen)
		return WINMSG_NOT_HANDLED;

	if (message == WM_CHAR)
	{
		if (m_pEditWithFocus)
		{
			//Ignore alt+key, and only handle first keydown except bksp/char 8
			if (!(lParam & CharFilter) || (wParam == 8 && !(lParam & ALT_KEY_PRESSED))) 
			{
				//Return or tab key
				if (wParam == 13 || wParam == 9) 
				{
					RemoveFocusFromEditbox(m_pEditWithFocus);

					//Determine next editbox to get focus 
					if (m_pEditWithFocus == &m_editServer)
						m_pEditWithFocus = &m_editName;
					else if (m_pEditWithFocus == &m_editName)
						m_pEditWithFocus = &m_editID;
					else if (m_pEditWithFocus == &m_editID)
						m_pEditWithFocus = &m_editPassword;
					else if (m_pEditWithFocus == &m_editPassword)
					{
						if (m_butPilot.IsOn())
							m_pEditWithFocus = &m_editCallsign;
						else
							m_pEditWithFocus = &m_editServer;
					}
					else if (m_pEditWithFocus == &m_editCallsign)
						m_pEditWithFocus = &m_editACType;
					else if (m_pEditWithFocus == &m_editACType)
						m_pEditWithFocus = &m_editServer;

					if (m_pEditWithFocus)
						SetFocusToEditbox(m_pEditWithFocus);
					
					m_pMainDlg->OnChildInitiatedRedraw();  //force refresh

					return WINMSG_HANDLED_REDRAW_US;
				}

				if (m_pEditWithFocus->CharIn((TCHAR)wParam))
				{
					m_pGraph->SetOutputBitmap(&m_bitCurrent);
					m_pEditWithFocus->Draw();
					return WINMSG_HANDLED_REDRAW_US;
				}
			}
			return WINMSG_HANDLED_NO_REDRAW;
		} 
		return WINMSG_NOT_HANDLED;
	}

	if (message == WM_KILLFOCUS)
	{
		if (m_pEditWithFocus)
		{
			RemoveFocusFromEditbox(m_pEditWithFocus);
			m_pEditWithFocus = NULL;
			m_pMainDlg->OnChildInitiatedRedraw();
			return WINMSG_HANDLED_REDRAW_US;
		}
		return WINMSG_NOT_HANDLED;
	}

	if (message == WM_LBUTTONDOWN)
	{
		int X = GET_X_LPARAM(lParam) - m_iX, Y = GET_Y_LPARAM(lParam) - m_iY;

		//See if click on edit box 
		for (size_t i = 0; i < m_apEditBoxes.size(); i++)
		{
			if (m_apEditBoxes[i]->IsWithin(X, Y))
			{
				//First click on this box?
				if (m_pEditWithFocus != m_apEditBoxes[i])
				{
					m_pGraph->SetOutputBitmap(&m_bitCurrent);
					if (m_pEditWithFocus)
					{
						m_pEditWithFocus->EnableCursor(false);
						m_pEditWithFocus->Draw();
					}
					m_pEditWithFocus = m_apEditBoxes[i];
					SetFocusToEditbox(m_pEditWithFocus);
					return WINMSG_HANDLED_REDRAW_US;
				}
				//Box already active
				return WINMSG_HANDLED_NO_REDRAW;
			}
		}
		//Click somewhere other than edit boxes -- remove focus if it had it
		if (m_pEditWithFocus)
		{
			RemoveFocusFromEditbox(m_pEditWithFocus);
			m_pEditWithFocus = NULL;

			//Force redraw because cursor was erased, but don't flag as processed.
			//We could maybe add WINMSG_NOT_HANDLED_REDRAW_US if we do this a lot 
			m_pMainDlg->OnChildInitiatedRedraw();   
		}
		
		//See if click on Pilot button -- turn on and turn observer off
		if (m_butPilot.IsWithin(X, Y))
		{
			if (!m_butPilot.IsOn())
			{
				m_pGraph->SetOutputBitmap(&m_bitCurrent);
				m_butPilot.SetOn(true);
				m_butObserver.SetOn(false);
				m_butPilot.Draw();
				m_butObserver.Draw();
				m_editCallsign.SetHidden(false);
				m_editACType.SetHidden(false);
				return WINMSG_HANDLED_REDRAW_US;
			}
			return WINMSG_HANDLED_NO_REDRAW;
		}
		//Click on observer button? Turn on and turn pilot button off
		if (m_butObserver.IsWithin(X, Y))
		{
			if (!m_butObserver.IsOn())
			{
				m_pGraph->SetOutputBitmap(&m_bitCurrent);
				m_butPilot.SetOn(false);
				m_butObserver.SetOn(true);
				m_butPilot.Draw();
				m_butObserver.Draw();
				m_editCallsign.SetHidden(true);
				m_editACType.SetHidden(true);
				return WINMSG_HANDLED_REDRAW_US;
			}
			return WINMSG_HANDLED_NO_REDRAW;
		}

		//Clicked on connect button?
		if (m_butConnect.IsWithin(X, Y))
			return m_pMainDlg->OnLoginConnectPressed();
		
		//Clicked on ServerSelect button?
		if (m_butConnect.IsWithin(X, Y))
			return m_pMainDlg->OnServerSelectPressed();
	}

	return WINMSG_NOT_HANDLED;
}

//Enables cursor in given editbox, redraws, caller must set m_pEditWithFocus
int CLoginDlg::SetFocusToEditbox(CEditBox *pEdit)
{
	m_pGraph->SetOutputBitmap(&m_bitCurrent);
	pEdit->EnableCursor(true);
	pEdit->Draw(); 
	m_pMainDlg->GetKeyboardInput(true);
	return 1;
}

//Disables cursor in given editbox, redraws, caller must set/clear m_pEditWithFocus
int CLoginDlg::RemoveFocusFromEditbox(CEditBox *pEdit)
{
	m_pGraph->SetOutputBitmap(&m_bitCurrent);
	pEdit->EnableCursor(false);
	pEdit->Draw();
	m_pMainDlg->GetKeyboardInput(false);
	return 1;
}

int CLoginDlg::Update()
{
	//Only edit box with focus needs to be updated
	if (m_pEditWithFocus)
	{
		if (m_pEditWithFocus->Update())
		{
			m_pGraph->SetOutputBitmap(&m_bitCurrent);
			m_pEditWithFocus->Draw();
			m_pMainDlg->OnChildInitiatedRedraw();
			return WINMSG_HANDLED_REDRAW_US;
		}
		return WINMSG_HANDLED_NO_REDRAW;
	}
	return WINMSG_NOT_HANDLED;
}

//Draw current dialog to current m_pGraph output (parent dialog caller sets). We ignore pDevice because 
//we know we're not a top-level dialog
int CLoginDlg::Draw(IDirect3DDevice9* pDevice)
{
	if (!m_bOpen)
		return 0;
	m_pGraph->DrawBitmapToOutputBitmap(&m_bitCurrent, m_iX, m_iY);
	
	//Cover over last line (callsign and ac type) in observer mode
	if (m_butObserver.IsOn())
		m_pGraph->DrawBitmapToOutputBitmap(&m_bitCover, m_iX + m_iCoverX, m_iY + m_iCoverY);

	return 1;
}

int CLoginDlg::Open()
{
	m_bOpen = true;
	return 1;
}

int CLoginDlg::Close()
{
	if (m_pEditWithFocus)
		RemoveFocusFromEditbox(m_pEditWithFocus);
	m_pEditWithFocus = NULL;
	m_bOpen = false;
	return 1;
}

int CLoginDlg::Shutdown()
{
	m_pEditWithFocus = NULL;
	m_pGraph->DeleteBM(&m_bitBack);
	m_pGraph->DeleteBM(&m_bitCurrent);
	m_pGraph->DeleteBM(&m_bitButSelected);
	m_pGraph->DeleteBM(&m_bitButNotSelected);
	m_pGraph->DeleteBM(&m_bitCover);
	m_butPilot.Shutdown();
	m_butObserver.Shutdown();
	m_butConnect.Shutdown();
	m_butServerSelect.Shutdown();
	for (size_t i = 0; i < m_apEditBoxes.size(); i++)
		m_apEditBoxes[i]->Shutdown();
	DeleteFont(m_hFieldnameFont);
	DeleteFont(m_hDataFont);

	return 1;
}