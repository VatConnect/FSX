#include "stdafx.h"
#include "Dialogs.h"

//This is pretty much a cut-and-paste copy of Login dialog, didn't think it was worth making a common base 
//dialog (maybe a generic "input form with named data fields") to only be used by 2 dialogs, just to make it more 
//customizable if desired. But that might be something for the future.

#define FP_FIELDNAME_FONT L"Arial"                  //field names
#define FP_DATAFIELDS_FONT L"Lucida Console"        //user-entered content


CFlightPlanDlg::CFlightPlanDlg() : m_bOpen(false), m_pGraph(NULL), m_pMainDlg(NULL) 
{
}

CFlightPlanDlg::~CFlightPlanDlg()
{
}

int CFlightPlanDlg::Initialize(CMainDlg *pMainDlg, HWND hWnd, C2DGraphics *pGraph, 
	int X, int Y, int WidthPix, int HeightPix)
{
	m_pGraph = pGraph;
	m_iX = X;
	m_iY = Y;
	m_iWidthPix = WidthPix;
	m_iHeightPix = HeightPix;
	m_hWnd = hWnd;
	m_pMainDlg = pMainDlg;

	m_pGraph->FindBestFont(FP_FIELDNAME_FONT, FONT_SIZE, false, false, false, &m_hFieldnameFont);
	m_pGraph->FindBestFont(FP_DATAFIELDS_FONT, FONT_SIZE, true, false, false, &m_hDataFont);

	//Get character sizes for fieldname and data fields (height's probably the same anyway)
	int FWidthPix, FHeightPix, TWidthPix, THeightPix;
	m_pGraph->SetFont(m_hDataFont);
	m_pGraph->GetStringPixelSize(L"M", &TWidthPix, &THeightPix);
	m_iDataLineHeightPix = THeightPix;
	m_iDataCharWidthPix = TWidthPix;

	m_pGraph->SetFont(m_hFieldnameFont);
	m_pGraph->GetStringPixelSize(L"M", &FWidthPix, &FHeightPix);

	//Determine margins to center our text (10 lines worth)
	int TopMarginPix = (m_iHeightPix - (9 * FHeightPix)) / 2;

	//////////////
	//Make background bitmap and controls
	m_pGraph->MakeNewBitmap(WidthPix, HeightPix, &m_bitBack);
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);
	int YPos = TopMarginPix, XPos, EditXPix, W, H;
	int LineSpacePix = FHeightPix * 4 / 3;
	int LMarginPix = 3;

	//Callsign
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->GetStringPixelSize(L"Callsign:", &EditXPix, &H);
	XPos = LMarginPix;
	m_pGraph->DrawTxt(XPos, YPos, L"Callsign:");
	XPos = EditXPix + FWidthPix * 3 / 2;
	m_editCallsign.Create(m_pGraph, XPos, YPos + 2, TWidthPix * 8 + 2, THeightPix + 1,
		COL_DLG_TEXT, COL_EDITBOX_BACK, m_hDataFont);
	m_editCallsign.SetMaxChars(7);
	XPos += TWidthPix * 8 + FWidthPix;

	//Type
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->GetStringPixelSize(L"Type:", &EditXPix, &H);
	m_pGraph->DrawTxt(XPos, YPos, L"Type:");
	XPos += EditXPix + FWidthPix;
	m_editType.Create(m_pGraph, XPos, YPos + 2, TWidthPix * 5 + 2, THeightPix + 1,
		COL_DLG_TEXT, COL_EDITBOX_BACK, m_hDataFont);
	m_editType.SetMaxChars(4);
	XPos += TWidthPix * 5 + FWidthPix;

	//Equip
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->GetStringPixelSize(L"Equip:", &EditXPix, &H);
	m_pGraph->DrawTxt(XPos, YPos, L"Equip:");
	XPos += EditXPix + FWidthPix;
	m_editEquip.Create(m_pGraph, XPos, YPos + 2, TWidthPix * 2 + 2, THeightPix + 1,
		COL_DLG_TEXT, COL_EDITBOX_BACK, m_hDataFont);
	m_editEquip.SetMaxChars(1);
	XPos = LMarginPix;
	YPos += LineSpacePix;

	//TAS
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->GetStringPixelSize(L"TAS:", &EditXPix, &H);
	m_pGraph->DrawTxt(XPos, YPos, L"TAS:");
	XPos += EditXPix + FWidthPix;
	m_editTAS.Create(m_pGraph, XPos, YPos + 2, TWidthPix * 5 + 2, THeightPix + 1,
		COL_DLG_TEXT, COL_EDITBOX_BACK, m_hDataFont);
	m_editTAS.SetMaxChars(4);
	XPos += TWidthPix * 4 + FWidthPix * 2;

	//Altitude
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->GetStringPixelSize(L"Altitude:", &EditXPix, &H);
	m_pGraph->DrawTxt(XPos, YPos, L"Altitude:");
	XPos += EditXPix + FWidthPix;
	m_editAltitude.Create(m_pGraph, XPos, YPos + 2, TWidthPix * 6 + 2, THeightPix + 1,
		COL_DLG_TEXT, COL_EDITBOX_BACK, m_hDataFont);
	m_editAltitude.SetMaxChars(5);
	XPos += EditXPix + FWidthPix * 3;
			  
	//IFR/VFR boxes 
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
	m_pGraph->LineTo(0, THeightPix + 1);
	m_pGraph->LineTo(0, 0);
	m_pGraph->SetOutputBitmap(&m_bitBack);
	 
	//IFR
	m_butIFR.Create(m_pGraph, XPos, YPos, TWidthPix + 4, THeightPix + 4, &m_bitButSelected,
		&m_bitButNotSelected, false);
	XPos += 2 * TWidthPix;
	m_pGraph->GetStringPixelSize(L"IFR", &EditXPix, &H);
	m_pGraph->DrawTxt(XPos, YPos, L"IFR");
	XPos += EditXPix + 3 * TWidthPix / 2;

	//VFR
	m_butVFR.Create(m_pGraph, XPos, YPos, TWidthPix + 4, THeightPix + 4, &m_bitButSelected,
		&m_bitButNotSelected, false);
	XPos += 2 * TWidthPix;
	m_pGraph->GetStringPixelSize(L"VFR", &EditXPix, &H);
	m_pGraph->DrawTxt(XPos, YPos, L"VFR");
	XPos += EditXPix + 2 * TWidthPix;

	YPos += LineSpacePix;
	XPos = LMarginPix;

	//Dep Time
	m_pGraph->GetStringPixelSize(L"Dep. Time:", &EditXPix, &H);
	m_pGraph->DrawTxt(XPos, YPos, L"Dep. Time:");
	XPos += EditXPix + FWidthPix / 2;
	m_editDepTime.Create(m_pGraph, XPos, YPos + 2, TWidthPix * 5 + 2, THeightPix + 1,
		COL_DLG_TEXT, COL_EDITBOX_BACK, m_hDataFont);
	m_editDepTime.SetMaxChars(4);
	XPos += TWidthPix * 5 + FWidthPix;

	//Estimated flight time
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->GetStringPixelSize(L"ETE:", &EditXPix, &H);
	m_pGraph->DrawTxt(XPos, YPos, L"ETE:");
	XPos += EditXPix + FWidthPix / 2;
	m_editETE.Create(m_pGraph, XPos, YPos + 2, TWidthPix * 5 + 2, THeightPix + 1,
		COL_DLG_TEXT, COL_EDITBOX_BACK, m_hDataFont);
	m_editETE.SetMaxChars(4);
	XPos += TWidthPix * 5 + FWidthPix;
	
	//Remark
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->GetStringPixelSize(L"Rmk:", &EditXPix, &H);
	m_pGraph->DrawTxt(XPos, YPos, L"Rmk:");
	XPos += EditXPix + FWidthPix / 2;
	m_editRmk.Create(m_pGraph, XPos, YPos + 2, TWidthPix * 9, THeightPix + 1,
		COL_DLG_TEXT, COL_EDITBOX_BACK, m_hDataFont);
	XPos = LMarginPix;
	YPos += LineSpacePix;

	//Route
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->GetStringPixelSize(L"Rte:", &EditXPix, &H);
	H = THeightPix + 1;
	m_pGraph->DrawTxt(XPos, YPos + H * 4 / 2 - H / 2, L"Rte:");
	m_editRoute.Create(m_pGraph, EditXPix + TWidthPix, YPos + 2, TWidthPix * 33, H * 4,
		COL_DLG_TEXT, COL_EDITBOX_BACK, m_hDataFont);
	
	YPos += H * 4 + LineSpacePix / 2;
	  
	//Send button
	BitmapStruct *pOn = new BitmapStruct;
	BitmapStruct *pOff = new BitmapStruct;
	m_pGraph->SetFont(m_hFieldnameFont);
	m_pGraph->GetStringPixelSize(L"  Send  ", &W, &H);
	m_pGraph->MakeNewBitmap(W, H, pOn);
	m_pGraph->SetOutputBitmap(pOn);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_ON);
	m_pGraph->SetTextColor(COL_BUTTON_TEXT);
	m_pGraph->DrawTxt(0, 0, L"  Send");
	m_pGraph->MakeNewBitmap(W, H, pOff);
	m_pGraph->SetOutputBitmap(pOff);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_OFF);
	m_pGraph->DrawTxt(0, 0, L"  Send");
	m_pGraph->SetOutputBitmap(&m_bitBack);
	XPos = FWidthPix * 9;
	m_butSend.Create(m_pGraph, XPos, YPos, W, H, pOn, pOff);
	XPos += FWidthPix * 9;

	//Clear button
	pOn = new BitmapStruct;
	pOff = new BitmapStruct;
	m_pGraph->GetStringPixelSize(L"  Clear  ", &W, &H);
	m_pGraph->MakeNewBitmap(W, H, pOn);
	m_pGraph->SetOutputBitmap(pOn);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_ON);
	m_pGraph->SetTextColor(COL_BUTTON_TEXT);
	m_pGraph->DrawTxt(0, 0, L"  Clear");
	m_pGraph->MakeNewBitmap(W, H, pOff);
	m_pGraph->SetOutputBitmap(pOff);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_OFF);
	m_pGraph->DrawTxt(0, 0, L"  Clear");
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_butClear.Create(m_pGraph, XPos, YPos, W, H, pOn, pOff);

	//Create array of pointers to our edit boxes
	m_apEditBoxes.push_back(&m_editCallsign);
	m_apEditBoxes.push_back(&m_editType);
	m_apEditBoxes.push_back(&m_editEquip);
	m_apEditBoxes.push_back(&m_editTAS);
	m_apEditBoxes.push_back(&m_editAltitude);
	m_apEditBoxes.push_back(&m_editDepTime);
	m_apEditBoxes.push_back(&m_editETE);
	m_apEditBoxes.push_back(&m_editRoute);
	m_apEditBoxes.push_back(&m_editRmk);

	//Create output bitmap 
	m_pGraph->MakeNewBitmap(WidthPix, HeightPix, &m_bitCurrent);

	//Set initial control states
	m_butIFR.SetOn(true);
	m_butVFR.SetOn(false);
	m_butSend.SetOn(false);
	m_butClear.SetOn(false);

	DrawWholeDialogToOutput();

	return 1;
}

int CFlightPlanDlg::DrawWholeDialogToOutput()
{
	m_pGraph->SetOutputBitmap(&m_bitCurrent);
	m_pGraph->DrawBitmapToOutputBitmap(&m_bitBack, 0, 0);
	m_editCallsign.Draw();
	m_editType.Draw();
	m_editEquip.Draw();
	m_editDepTime.Draw();
	m_editETE.Draw();
	m_editTAS.Draw();
	m_editAltitude.Draw();
	m_editRoute.Draw();
	m_editRmk.Draw();
	m_butIFR.Draw();
	m_butVFR.Draw();
	m_butSend.Draw();
	m_butClear.Draw();

	return 1;
}

//Enables cursor in given editbox, redraws, caller must set m_pEditWithFocus
int CFlightPlanDlg::SetFocusToEditbox(CEditBox *pEdit)
{
	m_pGraph->SetOutputBitmap(&m_bitCurrent);
	pEdit->EnableCursor(true);
	pEdit->Draw();
	m_pMainDlg->GetKeyboardInput(true);
	return 1;
}

//Disables cursor in given editbox, redraws, caller must set/clear m_pEditWithFocus
int CFlightPlanDlg::RemoveFocusFromEditbox(CEditBox *pEdit)
{
	m_pGraph->SetOutputBitmap(&m_bitCurrent);
	pEdit->EnableCursor(false);
	pEdit->Draw();
	m_pMainDlg->GetKeyboardInput(false);
	return 1;
}
	

/////////////
//Base class

int CFlightPlanDlg::WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam)
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
					if (m_pEditWithFocus == &m_editCallsign)
						m_pEditWithFocus = &m_editType;
					else if (m_pEditWithFocus == &m_editType)
						m_pEditWithFocus = &m_editEquip;
					else if (m_pEditWithFocus == &m_editEquip)
						m_pEditWithFocus = &m_editTAS;
					else if (m_pEditWithFocus == &m_editTAS)
						m_pEditWithFocus = &m_editAltitude;
					else if (m_pEditWithFocus == &m_editAltitude)
						m_pEditWithFocus = &m_editDepTime;
					else if (m_pEditWithFocus == &m_editDepTime)
						m_pEditWithFocus = &m_editETE;
					else if (m_pEditWithFocus == &m_editETE)
						m_pEditWithFocus = &m_editRmk;
					else if (m_pEditWithFocus == &m_editRmk)
						m_pEditWithFocus = &m_editRoute;
					else if (m_pEditWithFocus == &m_editRoute)
						m_pEditWithFocus = &m_editCallsign;

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

	if (message == WM_KILLFOCUS || (message == WM_ACTIVATE && wParam == WA_INACTIVE))
	{
		if (m_pEditWithFocus)
		{
			RemoveFocusFromEditbox(m_pEditWithFocus);
			m_pEditWithFocus = nullptr;
			m_pMainDlg->OnChildInitiatedRedraw();
			return WINMSG_HANDLED_REDRAW_US;
		}
		return WINMSG_NOT_HANDLED;
	}

	if (message == WM_LBUTTONDOWN || message == WM_LBUTTONDBLCLK)
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
			m_pEditWithFocus = nullptr;

			//Force redraw because cursor was erased, but don't flag as processed.
			//We could maybe add WINMSG_NOT_HANDLED_REDRAW_US if we do this a lot 
			m_pMainDlg->OnChildInitiatedRedraw();
		}

		//See if click on IFR button -- turn on and turn VFR off
		if (m_butIFR.IsWithin(X, Y))
		{
			if (!m_butIFR.IsOn())
			{
				m_pGraph->SetOutputBitmap(&m_bitCurrent);
				m_butIFR.SetOn(true);
				m_butVFR.SetOn(false);
				m_butIFR.Draw();
				m_butVFR.Draw();
				return WINMSG_HANDLED_REDRAW_US;
			}
			return WINMSG_HANDLED_NO_REDRAW;
		}

		//Click on VFR button? Turn on and turn IFR button off
		if (m_butVFR.IsWithin(X, Y))
		{
			if (!m_butVFR.IsOn())
			{
				m_pGraph->SetOutputBitmap(&m_bitCurrent);
				m_butIFR.SetOn(false);
				m_butVFR.SetOn(true);
				m_butIFR.Draw();
				m_butVFR.Draw();
				return WINMSG_HANDLED_REDRAW_US;
			}
			return WINMSG_HANDLED_NO_REDRAW;
		}

		//Clicked on Send button?
		if (m_butSend.IsWithin(X, Y))
			return m_pMainDlg->OnSendFlightPlanPressed();

		//Clicked on Clear button?
		if (m_butClear.IsWithin(X, Y))
			return m_pMainDlg->OnClearFlightPlanPressed();

	}

	return WINMSG_NOT_HANDLED;
}

int CFlightPlanDlg::Update()
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
int CFlightPlanDlg::Draw(IDirect3DDevice9* pDevice)
{
	if (!m_bOpen)
		return 0;

	m_pGraph->DrawBitmapToOutputBitmap(&m_bitCurrent, m_iX, m_iY);

	return 1;
}

int CFlightPlanDlg::Open()
{
	m_bOpen = true;
	return 1;
}

int CFlightPlanDlg::Close()
{
	if (m_pEditWithFocus)
		RemoveFocusFromEditbox(m_pEditWithFocus);
	m_pEditWithFocus = nullptr;
	m_bOpen = false;
	return 1;
}


int CFlightPlanDlg::Shutdown()
{
	m_pEditWithFocus = nullptr;
	m_pGraph->DeleteBM(&m_bitBack);
	m_pGraph->DeleteBM(&m_bitCurrent);
	m_pGraph->DeleteBM(&m_bitButSelected);
	m_pGraph->DeleteBM(&m_bitButNotSelected);
	m_butIFR.Shutdown();
	m_butVFR.Shutdown();
	m_butSend.Shutdown();
	m_butClear.Shutdown();
	for (size_t i = 0; i < m_apEditBoxes.size(); i++)
		m_apEditBoxes[i]->Shutdown();
	DeleteFont(m_hFieldnameFont);
	DeleteFont(m_hDataFont);

	return 0;
}