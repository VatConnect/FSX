#include "stdafx.h"
#include "Dialogs.h"

#define LOGIN_FIELDNAME_FONT L"Arial"                  //field names
#define LOGIN_DATAFIELDS_FONT L"Lucida Console"        //user-entered content
#define LOGIN_SERVER_DESC_COLUMN 15
#define LOGIN_PREV_TEXT L" \x25C4 "
#define LOGIN_NEXT_TEXT L" \x25BA "
#define LOGIN_BACK_TEXT L" Back "
#define LOGIN_PREV_COLUMN 11
#define LOGIN_NEXT_COLUMN 24
#define LOGIN_SERVER_MARGIN_PIX 1

CLoginDlg::CLoginDlg() : m_bOpen(false), m_hFieldnameFont(nullptr), m_hDataFont(nullptr), m_pEditWithFocus(nullptr), 
	m_bServerSelectOpen(false), m_pServerPages(nullptr), m_iNextServerLine(0), m_pServerInfo(nullptr), m_iNumServerPages(0),
	m_bConnectedToServer(false)
{
}

CLoginDlg::~CLoginDlg()
{
}

int CLoginDlg::Initialize(CMainDlg *pMainDlg, CFlightPlanDlg *pFPDialog, HWND hWnd, 
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
	m_pFlightPlanDlg = pFPDialog;

	m_pGraph->FindBestFont(LOGIN_FIELDNAME_FONT, FONT_SIZE, false, false, false, &m_hFieldnameFont);
	m_pGraph->FindBestFont(LOGIN_DATAFIELDS_FONT, FONT_SIZE, true, false, false, &m_hDataFont);

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
	int EditXPix;

	//////////////
	//Make background bitmap and controls
	m_pGraph->MakeNewBitmap(WidthPix, HeightPix, &m_bitBack);
	m_pGraph->SetOutputBitmap(&m_bitBack);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);
	int YPos = TopMarginPix;
	int LineSpacePix = FHeightPix * 4 / 3;

	//Make highlight and normal bitmap for server select
	m_pGraph->MakeNewBitmap(WidthPix, m_iDataLineHeightPix, &m_bitHighlight);
	m_pGraph->SetOutputBitmap(&m_bitHighlight);
	m_pGraph->FillBitmapWithColor(COL_DLG_HIGHLIGHT);
	m_pGraph->MakeNewBitmap(WidthPix, m_iDataLineHeightPix, &m_bitNormServerBack);
	m_pGraph->SetOutputBitmap(&m_bitNormServerBack);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);

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

	//Make disconnect button
	pOn = new BitmapStruct;
	pOff = new BitmapStruct;
	m_pGraph->SetFont(m_hFieldnameFont);
	m_pGraph->GetStringPixelSize(L"  Disconnect  ", &W, &H);
	m_pGraph->MakeNewBitmap(W, H, pOn);
	m_pGraph->SetOutputBitmap(pOn);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_ON);
	m_pGraph->SetTextColor(COL_BUTTON_TEXT);
	m_pGraph->DrawTxt(0, 0, L"  Disconnect");
	m_pGraph->MakeNewBitmap(W, H, pOff);
	m_pGraph->SetOutputBitmap(pOff);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_OFF);
	m_pGraph->DrawTxt(0, 0, L"  Disconnect");
	m_butDisconnect.Create(m_pGraph, (m_iWidthPix - W) / 2, m_iHeightPix / 2 + FHeightPix / 2, W, H, pOn, pOff);

	//Make disconnect page
	m_pGraph->MakeNewBitmap(m_iWidthPix, m_iHeightPix, &m_bitDisconnectPage);
	m_pGraph->SetOutputBitmap(&m_bitDisconnectPage);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);
	m_pGraph->SetFont(m_hFieldnameFont);
	m_pGraph->SetTextColor(COL_DLG_TEXT);
	m_pGraph->GetStringPixelSize(L"Press to disconnect from server", &W, &H);
	m_pGraph->DrawTxt((m_iWidthPix - W) / 2, m_iHeightPix / 2 - FHeightPix * 3 / 2, L"Press to disconnect from server");
	m_butDisconnect.Draw();
	 
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
	m_butDisconnect.SetOn(false);
	m_butServerSelect.SetOn(false);
	
	DrawWholeDialogToOutput();
	
	return 1;
}  

int CLoginDlg::SetLoginData(LoginDlgDataStruct *p)
{
	m_editServer.SetText(p->ServerName);
	m_editName.SetText(p->Name);
	m_editID.SetText(p->ID);
	m_editPassword.SetText(p->Password);
	m_editCallsign.SetText(p->Callsign);
	m_editACType.SetText(p->ACType);
	DrawWholeDialogToOutput();
	m_pMainDlg->OnChildInitiatedRedraw();
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
	if (pEdit == &m_editCallsign)
	{
		TCHAR *Callsign = m_editCallsign.GetText();
		m_pFlightPlanDlg->SetAircraftInfo(Callsign, NULL, NULL);
	}
	else if (pEdit == &m_editACType)
	{
		TCHAR *ACType = m_editACType.GetText();
		m_pFlightPlanDlg->SetAircraftInfo(NULL, ACType, NULL);
	}

	return 1;
}

//Add Server to our list. This should be called after initialization, but before we open
//the server select dialog because we don't make the bitmaps until it's first opened
//(i.e. if it's opened and this is called, it won't show)
int CLoginDlg::AddServer(WCHAR *ServerName, WCHAR *ServerDesc)
{
	ServerInfoStruct *p = new ServerInfoStruct;
	if (wcslen(ServerName) < MAX_SERVER_NAME)
		wcscpy_s(p->Name, MAX_SERVER_NAME, ServerName);
	else
		wcscpy_s(p->Name, MAX_SERVER_NAME, L"**INVALID");
	if (wcslen(ServerDesc) < MAX_SERVER_DESC)
		wcscpy_s(p->Description, MAX_SERVER_DESC, ServerDesc);
	else
		wcscpy_s(p->Description, MAX_SERVER_DESC, L"**INVALID");

	//Add to end of list
	p->pNext = nullptr;
	ServerInfoStruct *pList = m_pServerInfo, *pEnd = nullptr;
	while (pList)
	{
		pEnd = pList;
		pList = pList->pNext;
	}
	if (!pEnd)
		m_pServerInfo = p;
	else
		pEnd->pNext = p;

	return 1;
}

//Create the server page bitmaps for current m_pServerInfo list. Expected to only be
//called once, when server select first opened.
int CLoginDlg::MakeServerPages()
{
	if (m_pServerPages)
		return 0;

	int CurLine = 0, CurPage = 1, MaxLineNum = (m_iHeightPix - LOGIN_SERVER_MARGIN_PIX) / 
		m_iDataLineHeightPix - 3; //bottom line number, 0..n
	ServerInfoStruct *pI = m_pServerInfo;
	BitmapPageStruct *pPage = new BitmapPageStruct;
	m_pGraph->SetFont(m_hDataFont);
	m_pGraph->MakeNewBitmap(m_iWidthPix, m_iHeightPix, &pPage->Bitmap);
	m_pGraph->SetOutputBitmap(&pPage->Bitmap);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);
	m_pGraph->SetTextColor(COL_DLG_TEXT);
	pPage->pNext = nullptr;
	m_pServerPages = pPage;

	//Make back button
	BitmapStruct bmBackButton;
	WCHAR Str[64];
	int w, h;
	m_pGraph->GetStringPixelSize(LOGIN_BACK_TEXT, &w, &h);
	m_pGraph->MakeNewBitmap(w, m_iDataLineHeightPix, &bmBackButton);
	m_iBackButtonWidthPix = w;
	m_pGraph->SetOutputBitmap(&bmBackButton);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_OFF);
	m_pGraph->SetTextColor(COL_BUTTON_TEXT);
	m_pGraph->DrawTxt(0, 0, LOGIN_BACK_TEXT);
	
	m_pGraph->SetOutputBitmap(&pPage->Bitmap);
	m_pGraph->DrawBitmapToOutputBitmap(&bmBackButton, 0, m_iHeightPix - m_iDataLineHeightPix);

	int MaxColumnNum = m_iWidthPix / m_iDataCharWidthPix;

	while (pI)
	{
		//Draw server name --note copy and paste with DrawServerLine //REVISIT
		wcscpy_s(Str, 64, pI->Name);
		Str[LOGIN_SERVER_DESC_COLUMN] = (WCHAR)0;
		Str[LOGIN_SERVER_DESC_COLUMN - 1] = (WCHAR)' ';
		m_pGraph->DrawTxt(LOGIN_SERVER_MARGIN_PIX, LOGIN_SERVER_MARGIN_PIX + CurLine * m_iDataLineHeightPix, Str);

		//Draw description
		wcscpy_s(Str, 64, pI->Description);
		Str[MaxColumnNum - LOGIN_SERVER_DESC_COLUMN] = (WCHAR)0;
		m_pGraph->DrawTxt(LOGIN_SERVER_MARGIN_PIX + LOGIN_SERVER_DESC_COLUMN * m_iDataCharWidthPix, LOGIN_SERVER_MARGIN_PIX + 
			CurLine * m_iDataLineHeightPix, Str);
		pI->iLineNum = CurLine;
		pI->iPageNum = CurPage;
		CurLine++;
		pI = pI->pNext;

		//New page?
		if (pI && CurLine > MaxLineNum)
		{
			//Go to next page
			CurPage++;
			CurLine = 0;
			pPage->pNext = new BitmapPageStruct;
			pPage = pPage->pNext;
			m_pGraph->MakeNewBitmap(m_iWidthPix, m_iHeightPix, &pPage->Bitmap);
			m_pGraph->SetOutputBitmap(&pPage->Bitmap);
			m_pGraph->FillBitmapWithColor(COL_DLG_BACK);
			
			//Put on back button
			m_pGraph->DrawBitmapToOutputBitmap(&bmBackButton, 0, m_iHeightPix - m_iDataLineHeightPix);
		}
		//End of server list?
		else if (!pI)
			m_pGraph->DrawBitmapToOutputBitmap(&bmBackButton, 0, m_iHeightPix - m_iDataLineHeightPix);
	}

	m_iNumServerPages = CurPage;

	//Put page numbers and page forward/back on pages, if applicable
	if (CurPage > 1)
	{
		WCHAR PageStr[16];

		//Create Previous button
		BitmapStruct bmPrevButton, bmNextButton;
		m_pGraph->GetStringPixelSize(LOGIN_PREV_TEXT, &w, &h);
		m_pGraph->MakeNewBitmap(w, m_iDataLineHeightPix, &bmPrevButton);
		m_pGraph->SetOutputBitmap(&bmPrevButton);
		m_pGraph->FillBitmapWithColor(COL_BUTTON_OFF);
		m_pGraph->SetTextColor(COL_BUTTON_TEXT);
		m_pGraph->DrawTxt(0, 0, LOGIN_PREV_TEXT);
		//Should never be 100+ servers, but just in case adjust button left
		m_iPrevButtonX = (CurPage < 10? LOGIN_PREV_COLUMN : LOGIN_PREV_COLUMN - 3) * m_iDataCharWidthPix;
		m_iPrevButtonWidthPix = bmPrevButton.WidthPix;

		//Create Next button
		m_pGraph->GetStringPixelSize(LOGIN_NEXT_TEXT, &w, &h);
		m_pGraph->MakeNewBitmap(w, m_iDataLineHeightPix, &bmNextButton);
		m_pGraph->SetOutputBitmap(&bmNextButton);
		m_pGraph->FillBitmapWithColor(COL_BUTTON_OFF);
		m_pGraph->SetTextColor(COL_BUTTON_TEXT);
		m_pGraph->DrawTxt(0, 0, LOGIN_NEXT_TEXT);
		m_iNextButtonX = (CurPage < 10? LOGIN_NEXT_COLUMN : LOGIN_NEXT_COLUMN + 3) * m_iDataCharWidthPix;
		m_iNextButtonWidthPix = bmNextButton.WidthPix;
	
		//Go through each page
		pPage = m_pServerPages;
		for (int i = 1; i <= CurPage; i++)
		{
			m_pGraph->SetOutputBitmap(&pPage->Bitmap);
			wsprintf(PageStr, L"Page %i/%i", i, CurPage);
			m_pGraph->GetStringPixelSize(PageStr, &w, &h);
			m_pGraph->DrawTxt((m_iWidthPix - w) / 2, m_iHeightPix - m_iDataLineHeightPix, PageStr);
			if (i > 1)
				m_pGraph->DrawBitmapToOutputBitmap(&bmPrevButton, m_iPrevButtonX, m_iHeightPix - m_iDataLineHeightPix);
			if (i < CurPage)
				m_pGraph->DrawBitmapToOutputBitmap(&bmNextButton, m_iNextButtonX, m_iHeightPix - m_iDataLineHeightPix);
			pPage = pPage->pNext;
		}
		m_pGraph->DeleteBM(&bmPrevButton);
		m_pGraph->DeleteBM(&bmNextButton);
	}
	m_pGraph->DeleteBM(&bmBackButton);

	return 1;
}

//Draw one server line on given page and line 
int CLoginDlg::DrawServerLine(ServerInfoStruct *pServerInfo, int LineNum, int PageNum, bool bHighlighted)
{
	BitmapPageStruct *pBM = m_pServerPages;

	//Skip to appropriate page -- we know it exists because it was selected by user
	for (int i = 1; i < PageNum; i++)
		pBM = pBM->pNext;

	m_pGraph->SetOutputBitmap(&pBM->Bitmap);
	int Y = m_iDataLineHeightPix * LineNum;

	//Draw background bitmap
	if (bHighlighted)
		m_pGraph->DrawBitmapToOutputBitmap(&m_bitHighlight, 0, Y);
	else
		m_pGraph->DrawBitmapToOutputBitmap(&m_bitNormServerBack, 0, Y);

	m_pGraph->SetFont(m_hDataFont);
	m_pGraph->SetTextColor(COL_DLG_TEXT);
	WCHAR Str[64];
	int MaxColumnNum = m_iWidthPix / m_iDataCharWidthPix;

	//Draw server name --cut and paste from MakePages above
	wcscpy_s(Str, 64, pServerInfo->Name);
	Str[LOGIN_SERVER_DESC_COLUMN] = (WCHAR)0;
	Str[LOGIN_SERVER_DESC_COLUMN - 1] = (WCHAR)' ';
	m_pGraph->DrawTxt(LOGIN_SERVER_MARGIN_PIX, LOGIN_SERVER_MARGIN_PIX + LineNum * m_iDataLineHeightPix, Str);

	//Draw description
	wcscpy_s(Str, 64, pServerInfo->Description);
	Str[MaxColumnNum - LOGIN_SERVER_DESC_COLUMN] = (WCHAR)0;
	m_pGraph->DrawTxt(LOGIN_SERVER_MARGIN_PIX + LOGIN_SERVER_DESC_COLUMN * m_iDataCharWidthPix, LOGIN_SERVER_MARGIN_PIX +
		LineNum * m_iDataLineHeightPix, Str);

	return 1;
}

//Return pointer to the serverinfostruct for given line and page number (1..n),
//or nullptr if not found
ServerInfoStruct* CLoginDlg::GetSelectedServer(int LineNum, int PageNum)
{
	ServerInfoStruct *p = m_pServerInfo;
	while (p)
	{
		if (p->iLineNum == LineNum && p->iPageNum == PageNum)
			return p;
		p = p->pNext;
	}
	return nullptr;
}

//Return the data contents of the dialog. Returns 0 if any of the fields
//were too long (and those fields are left blank).
int CLoginDlg::GetLoginData(LoginDlgDataStruct **ppData)
{
	LoginDlgDataStruct *p = *ppData;
	int Ret = 1;

	if (m_editServer.GetTextLength() < 64)
		wcscpy_s(p->ServerName, m_editServer.GetText());
	else Ret = 0;
	if (m_editName.GetTextLength() < 64)
		wcscpy_s(p->Name, m_editName.GetText());
	else Ret = 0;
	if (m_editID.GetTextLength() < 32)
		wcscpy_s(p->ID, m_editID.GetText());
	else Ret = 0;
	if (m_editPassword.GetTextLength() < 32)
		wcscpy_s(p->Password, m_editPassword.GetText());
	else Ret = 0;
	if (m_editCallsign.GetTextLength() < 16)
		wcscpy_s(p->Callsign, m_editCallsign.GetText());
	else Ret = 0;
	if (m_editACType.GetTextLength() < 16)
		wcscpy_s(p->ACType, m_editACType.GetText());
	else Ret = 0;
	p->bIsPilot = m_butPilot.IsOn();
	p->bIsObserver = m_butObserver.IsOn();
	return Ret;
}

//Set just the callsign (called by flight plan dialog e.g. before connecting, so everything syncs)
int CLoginDlg::SetCallsign(WCHAR *Callsign)
{
	m_editCallsign.SetText(Callsign);
	DrawWholeDialogToOutput();
	m_pMainDlg->OnChildInitiatedRedraw();
	return 1;
}

//Set just the AC type
int CLoginDlg::SetACType(WCHAR *ACType)
{
	m_editACType.SetText(ACType);
	DrawWholeDialogToOutput();
	m_pMainDlg->OnChildInitiatedRedraw();
	return 1;
}

//Indicated we're connected (true) or no longer connected to server (false)
int CLoginDlg::IndicateConnected(bool bConnected)
{
	m_bConnectedToServer = bConnected;
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
					{
						if (wParam == 9)
							m_pEditWithFocus = &m_editServer;
						else
							m_pEditWithFocus = nullptr;
					}
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

		if (m_bServerSelectOpen)
		{
			//Check bottom row buttons
			if (Y >= (m_iHeightPix - m_iDataLineHeightPix) && message != WM_LBUTTONDBLCLK)
			{
				//Back button?
				if (X <= m_iBackButtonWidthPix)
				{
					m_bServerSelectOpen = false;
					return WINMSG_HANDLED_REDRAW_US;
				}

				//Previous button?
				if (m_iServerPageNum > 1 && X >= m_iPrevButtonX && X <= (m_iPrevButtonX + m_iPrevButtonWidthPix))
				{
					m_iServerPageNum--;
					m_pCurServerPage = m_pServerPages;
					for (int i = 1; i < m_iServerPageNum; i++)
						m_pCurServerPage = m_pCurServerPage->pNext;
					return WINMSG_HANDLED_REDRAW_US;
				}

				//Next?
				if (m_iServerPageNum < m_iNumServerPages && X >= m_iNextButtonX &&
					X <= (m_iNextButtonX + m_iNextButtonWidthPix))
				{
					m_iServerPageNum++;
					m_pCurServerPage = m_pCurServerPage->pNext;
					return WINMSG_HANDLED_REDRAW_US;
				}
				return WINMSG_NOT_HANDLED;
			}
			//Check if clicking on server line 
			int Line = Y / m_iDataLineHeightPix;
			ServerInfoStruct *p = GetSelectedServer(Line, m_iServerPageNum);
			if (p)
			{
				//Second click on already-selected one? Note double-click will process twice, 
				//first as LBUTTONDOWN then DBLCLICK
				if (m_iSelectedServerLine == Line && m_iSelectedServerPage == m_iServerPageNum)
				{
					//Unhighlight it and leave select screen
					DrawServerLine(p, m_iSelectedServerLine, m_iSelectedServerPage, false);
					m_bServerSelectOpen = false;
					m_editServer.ClearText();
					m_editServer.SetText(p->Name);
					m_pGraph->SetOutputBitmap(&m_bitCurrent);
					m_editServer.Draw();
					return WINMSG_HANDLED_REDRAW_US;
				}
				
				//Draw deselected old 
				if (m_iSelectedServerLine >= 0)
				{
					ServerInfoStruct *pOld = GetSelectedServer(m_iSelectedServerLine, m_iSelectedServerPage);
					DrawServerLine(pOld, m_iSelectedServerLine, m_iSelectedServerPage, false);
				}
				 
				//Draw highlighted new
				DrawServerLine(p, Line, m_iServerPageNum, true);
				m_iSelectedServerLine = Line;
				m_iSelectedServerPage = m_iServerPageNum;
				return WINMSG_HANDLED_REDRAW_US;
			}
			return WINMSG_NOT_HANDLED;
		}
		//Currently in "disconnect from server" page?
		else if (m_bConnectedToServer)
		{
			//See if click on disconnect button
			if (m_butDisconnect.IsWithin(X, Y))
			{
				m_pMainDlg->OnLoginDisconnectPressed();
				return WINMSG_HANDLED_NO_REDRAW;
			}
			return WINMSG_NOT_HANDLED;
		}
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
						RemoveFocusFromEditbox(m_pEditWithFocus);			
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
		{
			return m_pMainDlg->OnLoginConnectPressed(m_editServer.GetText(), m_editName.GetText(),
				m_editID.GetText(), m_editPassword.GetText(), m_editCallsign.GetText(),
				m_editACType.GetText(), m_butObserver.IsOn());
		}

		//Clicked on ServerSelect button?
		if (m_butServerSelect.IsWithin(X, Y))
		{
			m_bServerSelectOpen = true;
			if (!m_pServerPages)
				MakeServerPages();
			m_pCurServerPage = m_pServerPages;
			m_iServerPageNum = 1;
			m_iSelectedServerLine = -1;
			m_iSelectedServerPage = -1;
			return WINMSG_HANDLED_REDRAW_US;
		}
	}

	return WINMSG_NOT_HANDLED;
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
	if (m_bConnectedToServer)
		m_pGraph->DrawBitmapToOutputBitmap(&m_bitDisconnectPage, m_iX, m_iY);
	else if (m_bServerSelectOpen)
		m_pGraph->DrawBitmapToOutputBitmap(&m_pCurServerPage->Bitmap, m_iX, m_iY);
	else
	{
		m_pGraph->DrawBitmapToOutputBitmap(&m_bitCurrent, m_iX, m_iY);

		//Cover over last line (callsign and ac type) in observer mode
		if (m_butObserver.IsOn())
			m_pGraph->DrawBitmapToOutputBitmap(&m_bitCover, m_iX + m_iCoverX, m_iY + m_iCoverY);
	}
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
	m_pEditWithFocus = nullptr;
	m_bOpen = false;
	return 1;
}

int CLoginDlg::Shutdown()
{
	m_pEditWithFocus = nullptr;
	m_pGraph->DeleteBM(&m_bitBack);
	m_pGraph->DeleteBM(&m_bitCurrent);
	m_pGraph->DeleteBM(&m_bitButSelected);
	m_pGraph->DeleteBM(&m_bitButNotSelected);
	m_pGraph->DeleteBM(&m_bitCover);
	m_pGraph->DeleteBM(&m_bitHighlight);
	m_pGraph->DeleteBM(&m_bitNormServerBack);
	m_pGraph->DeleteBM(&m_bitDisconnectPage);
	m_butPilot.Shutdown();
	m_butObserver.Shutdown();
	m_butConnect.Shutdown();
	m_butDisconnect.Shutdown();
	m_butServerSelect.Shutdown();
	for (size_t i = 0; i < m_apEditBoxes.size(); i++)
		m_apEditBoxes[i]->Shutdown();
	DeleteFont(m_hFieldnameFont);
	DeleteFont(m_hDataFont);

	BitmapPageStruct *p = m_pServerPages, *o;
	while (p)
	{
		m_pGraph->DeleteBM(&p->Bitmap);
		o = p;
		p = p->pNext;
		delete o;
	}
	ServerInfoStruct *s = m_pServerInfo, *os;
	while (s)
	{
		os = s; 
		s = s->pNext;
		delete os;
	}
	return 1;
}