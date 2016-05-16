#include "stdafx.h"
#include "Dialogs.h"

#define ATC_EMPTY_TEXT L"No controllers in range."  
#define ATC_INFO_TEXT L"    Info    "
#define ATC_PREV_TEXT L"   \x25C4   "
#define ATC_NEXT_TEXT L"   \x25BA   "
#define ATC_BACK_TEXT L"   Back   "
#define ATC_FONT L"Arial"
#define ATC_PROP_FONT L"Lucida Console"
#define ATC_LMARGIN_PIX 1
#define ATC_TMARGIN_PIX 1
#define ATC_COL_CTR RGB(0, 255, 255)
#define ATC_COL_TRA RGB(220, 220, 0)
#define ATC_COL_LCL RGB(0, 192, 0)
#define ATC_COL_GND RGB(210, 160, 0)    
#define ATC_COL_CLR COL_DLG_TEXT          //also unknown facility type
#define ATC_FAC_COLUMN 0
#define ATC_FREQ_COLUMN 10
#define ATC_DIST_COLUMN_R 19              //Right-most column (this field is right-justified to this column) 
#define ATC_INFO_COLUMN 22
#define ATC_PREV_COLUMN 7
#define ATC_NEXT_COLUMN 16
#define METERS_PER_DEG 111120.0           //meters per degree longitude (spherical earth)
#define M_TO_NM 1.0/1852.0                //meters to nautical miles

CATCDlg::CATCDlg() : m_bOpen(false), m_pCenter(NULL), m_pTracon(NULL), m_pLocal(NULL), m_pGround(NULL),
	m_pClearance(NULL), m_iNumPages(0), m_iCurPage(0), m_dUserLatDegN(0.0), m_dUserLonDegE(0.0)
{
}

CATCDlg::~CATCDlg()
{
} 

int CATCDlg::Initialize(CMainDlg *pMainDlg, HWND hFSXWin, C2DGraphics *pGraph, int X, int Y, int WidthPix, int HeightPix)
{
	m_iX = X;
	m_iY = Y;
	m_iWidthPix = WidthPix;
	m_iHeightPix = HeightPix;
	m_pGraph = pGraph;
	m_pMainDlg = pMainDlg;
	
	//Determine special case controller info screen width and height (proportional font)
	int fw, fh;
	m_pGraph->FindBestFont(ATC_PROP_FONT, FONT_SIZE, true, false, false, &m_hPropFont);
	m_pGraph->SetFont(m_hPropFont);
	m_pGraph->GetStringPixelSize(L"W", &fw, &fh);
	m_iInfoWidthChar = (m_iWidthPix - ATC_LMARGIN_PIX * 2) / fw;
	m_iInfoHeightChar = (m_iHeightPix - ATC_TMARGIN_PIX * 2) / fh;
	m_iInfoCharWidthPix = fw;
	m_iInfoCharHeightPix = fh;

	//Determine regular character width and height of window 
	int iCharWidthPix, iCharHeightPix;
	m_pGraph->FindBestFont(ATC_FONT, FONT_SIZE, true, false, false, &m_hFont);
	m_pGraph->SetFont(m_hFont);
	m_pGraph->GetStringPixelSize(L"W", &iCharWidthPix, &iCharHeightPix);
	m_iWidthChar = (m_iWidthPix - ATC_LMARGIN_PIX * 2) / iCharWidthPix;    //2* for same on right margin 
	m_iHeightChar = (m_iHeightPix - ATC_TMARGIN_PIX) / iCharHeightPix;    
	m_iLineHeightPix = iCharHeightPix;
	m_iCharWidthPix = iCharWidthPix;

	//Create controller info bitmap
	m_pGraph->MakeNewBitmap(m_iWidthPix, m_iHeightPix, &m_bmControllerInfo);

	//Create info button
	int w, h;
	m_pGraph->GetStringPixelSize(ATC_INFO_TEXT, &w, &h);
	m_pGraph->MakeNewBitmap(w, m_iLineHeightPix - 2, &m_bmInfoButton);
	m_pGraph->SetOutputBitmap(&m_bmInfoButton);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_OFF);
	m_pGraph->SetTextColor(COL_BUTTON_TEXT);
	m_pGraph->DrawTxt(0, 0, ATC_INFO_TEXT);
	m_iInfoX = ATC_LMARGIN_PIX + ATC_INFO_COLUMN * m_iCharWidthPix;

	//Create back button
	m_pGraph->GetStringPixelSize(ATC_BACK_TEXT, &w, &h);
	m_pGraph->MakeNewBitmap(w, m_iLineHeightPix - 1, &m_bmBackButton);
	m_pGraph->SetOutputBitmap(&m_bmBackButton);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_OFF);
	m_pGraph->SetTextColor(COL_BUTTON_TEXT);
	m_pGraph->DrawTxt(0, 0, ATC_BACK_TEXT);

	//Create Previous button
	m_pGraph->GetStringPixelSize(ATC_PREV_TEXT, &w, &h);
	m_pGraph->MakeNewBitmap(w, m_iLineHeightPix, &m_bmPrevButton);
	m_pGraph->SetOutputBitmap(&m_bmPrevButton);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_OFF);
	m_pGraph->SetTextColor(COL_BUTTON_TEXT);
	m_pGraph->DrawTxt(0, 0, ATC_PREV_TEXT);
	m_iPrevX = ATC_PREV_COLUMN * m_iCharWidthPix + ATC_LMARGIN_PIX;
	m_iPrevY = m_iHeightPix - ATC_TMARGIN_PIX - m_iLineHeightPix;

	//Create Next button
	m_pGraph->GetStringPixelSize(ATC_NEXT_TEXT, &w, &h);
	m_pGraph->MakeNewBitmap(w, m_iLineHeightPix, &m_bmNextButton);
	m_pGraph->SetOutputBitmap(&m_bmNextButton);
	m_pGraph->FillBitmapWithColor(COL_BUTTON_OFF);
	m_pGraph->SetTextColor(COL_BUTTON_TEXT);
	m_pGraph->DrawTxt(0, 0, ATC_NEXT_TEXT);
	m_iNextX = ATC_NEXT_COLUMN * m_iCharWidthPix + ATC_LMARGIN_PIX;
	m_iNextY = m_iPrevY; 

	return 1;
}

//Add given ATC to the appropriate linked list. Caller needs to call Draw to refresh display if dialog is open 
int CATCDlg::AddATC(WCHAR *FacName, WCHAR *ControllerName, WCHAR *Freq, double dLatDegN, double dLonDegE, WCHAR *ControllerATIS)
{
	//Fill new controller structure
	ControllerStruct *p = new ControllerStruct;
	if (wcslen(FacName) >= ATC_MAX_FIELD_LENGTH)
		wcscpy_s(p->FacName, ATC_MAX_FIELD_LENGTH, L"**INVALID");   //don't make more than 9 chars
	else
		wcscpy_s(p->FacName, FacName);

	if (wcslen(ControllerName) >= ATC_MAX_FIELD_LENGTH)
		wcscpy_s(p->ControllerName, ATC_MAX_FIELD_LENGTH, L"**INVALID");
	else
		wcscpy_s(p->ControllerName, ControllerName);

	if (wcslen(Freq) >= ATC_MAX_FIELD_LENGTH)
		wcscpy_s(p->Freq, ATC_MAX_FIELD_LENGTH, L"**INVALID");
	else
		wcscpy_s(p->Freq, ATC_MAX_FIELD_LENGTH, Freq);
	
	if (wcslen(ControllerATIS) >= ATC_MAX_INFO_LENGTH)
	{
		//Clip it and end with ** instead of just saying invalid
		int i;
		for (i = 0; i < ATC_MAX_INFO_LENGTH - 3; i++)
			p->ControllerInfo[i] = ControllerATIS[i];
		p->ControllerInfo[i++] = (WCHAR)'*'; p->ControllerInfo[i++] = (WCHAR)'*';
		p->ControllerInfo[i] = 0;
	}
	else
		wcscpy_s(p->ControllerInfo, ATC_MAX_INFO_LENGTH, ControllerATIS);

	p->dLatDegN = dLatDegN;
	p->dLonDegE = dLonDegE;
	p->iPageNumber = 0;
	p->iLineNumber = -1;

	//Add to the front of the appropriate list. It's a doubly-linked list in case
	//we want to sort it by distance or name or something later, but for now don't
	//bother.
	if (wcsstr(p->FacName, L"_CTR"))
	{
		if (m_pCenter)
		{
			m_pCenter->pPrev = p;
			p->pNext = m_pCenter;
		}
		m_pCenter = p;
	}
	else if (wcsstr(p->FacName, L"_APP") || wcsstr(p->FacName, L"_DEP"))
	{
		if (m_pTracon)
		{
			m_pTracon->pPrev = p;
			p->pNext = m_pTracon;
		}
		m_pTracon = p;
	}
	else if (wcsstr(p->FacName, L"_TWR"))
	{
		if (m_pLocal)
		{
			m_pLocal->pPrev = p;
			p->pNext = m_pLocal;
		}
		m_pLocal = p;
	}
	else if (wcsstr(p->FacName, L"_GND"))
	{
		if (m_pGround)
		{
			m_pGround->pPrev = p;
			p->pNext = m_pGround;
		}
		m_pGround = p;
	}
	else
	{
		if (m_pClearance)
		{
			m_pClearance->pPrev = p;
			p->pNext = m_pClearance;
		}
		m_pClearance = p;
	}
	if (m_bOpen)
		CreatePages();

	return 1;
}

int CATCDlg::RemoveATC(WCHAR *FacName)
{
	//Find it in our lists
	ControllerStruct *p = NULL, **ppList = NULL;
	if (FindATCInList(FacName, m_pCenter, &p))
		ppList = &m_pCenter;
	else if (FindATCInList(FacName, m_pTracon, &p))
		ppList = &m_pTracon;
	else if (FindATCInList(FacName, m_pLocal, &p))
		ppList = &m_pLocal;
	else if (FindATCInList(FacName, m_pGround, &p))
		ppList = &m_pGround;
	else if (FindATCInList(FacName, m_pClearance, &p))
		ppList = &m_pClearance;
	else
		return 1;  

	//p now points to the controller, and ppList to the Center/tracon/local etc head pointer.
	//First in the list?
	if (!p->pPrev)
	{
		*ppList = p->pNext;
		if (p->pNext)
			p->pNext->pPrev = NULL;
	}
	else
	{
		p->pPrev->pNext = p->pNext;
		if (p->pNext)
			p->pNext->pPrev = p->pPrev;
	}
	delete p;
	
	if (m_bOpen)
		CreatePages();

	return 1;
}

//Given controller facility name and beginning of linked list, return 1 and pointer to the found item,
//else if not found return 0
int CATCDlg::FindATCInList(WCHAR *FacName, ControllerStruct *pList, /*[out]*/ControllerStruct **ppController)
{
	ControllerStruct *p = pList;
	while (p)
	{
		if (wcscmp(FacName, p->FacName) == 0)
		{
			*ppController = p;
			return 1;
		}
		p = p->pNext;
	}
	return 0;
}

//Set/update the current user position for calculating distance from controllers. Remakes bitmaps
//but caller needs to also call draw to redraw to output if we're open. If we're closed we just cache
//it and will re-create pages once we're opened. 
int CATCDlg::SetUserPosition(double dLatDegN, double dLonDegE)
{
	m_dUserLatDegN = dLatDegN;
	m_dUserLonDegE = dLonDegE;
	if (m_bOpen)
		CreatePages();
	return 1;
}

//Create all page bitmaps for current controller lists. 
int CATCDlg::CreatePages()
{
	m_iNumPages = 0;
	m_iNextLineNum = 0;

	//Clear all the existing page bitmaps
	BitmapPageStruct *p = m_pPages;
	while (p)
	{
		m_pGraph->SetOutputBitmap(&p->Bitmap);
		m_pGraph->FillBitmapWithColor(COL_DLG_BACK);
		p = p->pNext;
	}

	//Draw all the lists
	AddListToPages(m_pCenter, ATC_COL_CTR);
	AddListToPages(m_pTracon, ATC_COL_TRA);
	AddListToPages(m_pLocal, ATC_COL_LCL);
	AddListToPages(m_pGround, ATC_COL_GND);
	AddListToPages(m_pClearance, ATC_COL_CLR); 

	//Go through and add page numbers and next/previous buttons as applicable
	m_pGraph->SetFont(m_hFont);
	m_pGraph->SetTextColor(COL_DLG_TEXT);
	WCHAR PageString[16];
	int Width, H, Y = m_iPrevY;
	p = m_pPages;
	for (int PageNum = 1; PageNum <= m_iNumPages && PageNum < 999; PageNum++)
	{
		//Draw page string
		m_pGraph->SetOutputBitmap(&p->Bitmap);
		wsprintf(PageString, L"Page %i/%i", PageNum, m_iNumPages);
		m_pGraph->GetStringPixelSize(PageString, &Width, &H);
		m_pGraph->DrawTxt((m_iWidthPix - Width) / 2, Y, PageString);

		//Draw Previous and next buttons if applicable
		if (PageNum > 1)
			m_pGraph->DrawBitmapToOutputBitmap(&m_bmPrevButton, m_iPrevX, m_iPrevY - 1);
		if (PageNum < m_iNumPages)
			m_pGraph->DrawBitmapToOutputBitmap(&m_bmNextButton, m_iNextX, m_iNextY - 1); 
		p = p->pNext;

	}
	if (m_iCurPage < 1 && m_iNumPages > 0)
		m_iCurPage = 1;
	else if (m_iCurPage > m_iNumPages)
		m_iCurPage = m_iNumPages;
	else if (m_iNumPages == 0)
		m_iCurPage = 0;
	return 1;
}

//Given the front of a list of controllers, draw them to the next open spot
//in the bitmap pages, creating new pages as needed.
int CATCDlg::AddListToPages(ControllerStruct *pCont, COLORREF Color)
{
	if (!pCont)
		return 1;
	ControllerStruct *pNext = pCont;

	//Get last-most page filled out so far (more may be in the list)
	BitmapPageStruct *pPage = m_pPages;
	for (int i = 1; i < m_iNumPages; i++)
		pPage = pPage->pNext;
	
	//No pages in list?
	if (!pPage)
	{
		MakeNewPage(&pPage);
		m_pPages = pPage;
		m_iNextLineNum = 0;
	}
	if (m_iNumPages == 0)
	{
		m_iNumPages = 1;
		m_iCurPage = 1;
	}
	m_pGraph->SetTextColor(Color);
	m_pGraph->SetFont(m_hFont);
	m_pGraph->SetOutputBitmap(&pPage->Bitmap);

	int DistNM;
	WCHAR DistStr[8];

	//Draw next controller in list
	while (pNext)
	{
		//Page full? 
		if (m_iNextLineNum > (m_iHeightChar - 2))   //2 lines at bottom for page num
		{
			//Go to next cached one, or make new one and add to list
			if (pPage->pNext)
				pPage = pPage->pNext;
			else
			{
				BitmapPageStruct *pOld = pPage;
				MakeNewPage(&pPage);
				pOld->pNext = pPage;
			}
			m_iNextLineNum = 0;
			m_iNumPages++;
			m_pGraph->SetOutputBitmap(&pPage->Bitmap);
		}

		int Y = m_iNextLineNum * m_iLineHeightPix + ATC_TMARGIN_PIX;

		//Draw each of the elements
		m_pGraph->DrawTxt(ATC_LMARGIN_PIX + ATC_FAC_COLUMN * m_iCharWidthPix, Y, pNext->FacName);
		m_pGraph->DrawTxt(ATC_LMARGIN_PIX + ATC_FREQ_COLUMN * m_iCharWidthPix, Y, pNext->Freq);
		CalcUserDistance(pNext->dLatDegN, pNext->dLonDegE, &DistNM);
		if (DistNM < 0 || DistNM > 999)
			DistNM = 999;
		wsprintf(DistStr, L"%inm", DistNM);
		int w, h;
		m_pGraph->GetStringPixelSize(DistStr, &w, &h);
		m_pGraph->DrawTxt(ATC_LMARGIN_PIX + ATC_DIST_COLUMN_R * m_iCharWidthPix - w, Y, DistStr);
		m_pGraph->DrawBitmapToOutputBitmap(&m_bmInfoButton, m_iInfoX, Y);

		//Save what page and line this controller is on for detecting info button press
		pNext->iLineNumber = m_iNextLineNum;
		pNext->iPageNumber = m_iNumPages;

		m_iNextLineNum++;
		pNext = pNext->pNext;
	}
	return 1;
}

//Calculate the (integer) distance in nautical miles between user and given point (simple sphere model) 
int CATCDlg::CalcUserDistance(double dLatDegN, double dLonDegE, /*[out]*/int *piDistNM)
{
	static double DEG_TO_RAD = 4.0 * atan(1.0) / 180.0;

	double DeltaLatM = (dLatDegN - m_dUserLatDegN) * METERS_PER_DEG;
	double DeltaLonM = (dLonDegE - m_dUserLonDegE) * cos(m_dUserLatDegN * DEG_TO_RAD) * METERS_PER_DEG;

	*piDistNM = (int)(sqrt(DeltaLatM * DeltaLatM + DeltaLonM * DeltaLonM) * M_TO_NM);

	return 1;
}

int CATCDlg::MakeNewPage(/*[out]*/BitmapPageStruct **ppPage)
{
	BitmapPageStruct *p = new BitmapPageStruct;
	m_pGraph->MakeNewBitmap(m_iWidthPix, m_iHeightPix, &p->Bitmap);
	m_pGraph->SetOutputBitmap(&p->Bitmap);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);
	*ppPage = p;
	return 1;
}

//Given one of the controller linked lists, a page and line number, return 1 and the controllerstruct 
//for the controller in that spot, else 0
int CATCDlg::FindControllerOnPage(ControllerStruct *pList, int iPage, int iLineNum, ControllerStruct **ppControllerStruct)
{
	if (!pList)
		return 0;

	ControllerStruct *pNext = pList;
	while (pNext)
	{
		if (pNext->iLineNumber == iLineNum && pNext->iPageNumber == iPage)
		{
			*ppControllerStruct = pNext;
			return 1;
		}
		pNext = pNext->pNext;
	}
	return 0;
}

//Given controller, create the controller info bitmap
int CATCDlg::CreateControllerInfoPage(ControllerStruct *pController)
{
	//Blank previous bitmap
	m_pGraph->SetOutputBitmap(&m_bmControllerInfo);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);

	//Create top-most line (facility name left justified, controller right justified, freq centered between)
	m_pGraph->SetFont(m_hFont);
	m_pGraph->SetTextColor(ATC_COL_CTR);
	int fw, nw, freqw, h;
	m_pGraph->GetStringPixelSize(pController->FacName, &fw, &h);
	m_pGraph->DrawTxt(ATC_LMARGIN_PIX, ATC_TMARGIN_PIX, pController->FacName);
	m_pGraph->GetStringPixelSize(pController->ControllerName, &nw, &h);
	m_pGraph->DrawTxt(m_iWidthPix - ATC_LMARGIN_PIX - nw, ATC_TMARGIN_PIX, pController->ControllerName);
	m_pGraph->GetStringPixelSize(pController->Freq, &freqw, &h);
	m_pGraph->DrawTxt((m_iWidthPix - ATC_LMARGIN_PIX - nw - fw - freqw) / 2 + fw, ATC_TMARGIN_PIX,
		pController->Freq);
	
	//Go through drawing each char one by one just so we can parse linefeeds (CR ignored)
	int Col = 0, Line = 1;
	m_pGraph->SetFont(m_hPropFont);
	m_pGraph->SetTextColor(COL_DLG_TEXT);
	WCHAR *pNextChar = pController->ControllerInfo, str[2];
	str[1] = (WCHAR)0;
	while (*pNextChar != (WCHAR)0 && Line < m_iInfoHeightChar - 1)
	{
		while (*pNextChar != (WCHAR)0 && Col < m_iInfoWidthChar)
		{
			if (*pNextChar == (WCHAR)10)
				Col = m_iInfoWidthChar;  
			else if (*pNextChar != (WCHAR)13)
			{
				str[0] = *pNextChar;
				m_pGraph->DrawTxt(m_iInfoCharWidthPix * Col + ATC_LMARGIN_PIX, m_iLineHeightPix + 
					ATC_TMARGIN_PIX * 2 + (Line - 1) * m_iInfoCharHeightPix, str);
				Col++;
			}
			pNextChar++;
		}
		Col = 0;
		Line++;
	}

	//Draw back button on bottom
	m_iBackX = (m_iWidthPix - m_bmBackButton.WidthPix) / 2;
	m_iBackY = m_iHeightPix - m_bmBackButton.HeightPix - ATC_TMARGIN_PIX;
	m_pGraph->DrawBitmapToOutputBitmap(&m_bmBackButton, m_iBackX, m_iBackY);
	
	return 1;
}
/////////////
//Base class

int CATCDlg::WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_LBUTTONDOWN)
	{
		int X = GET_X_LPARAM(lParam) - m_iX, Y = GET_Y_LPARAM(lParam) - m_iY;
	
		
		if (m_bShowingControllerInfo)
		{
			//See if back button
			if (X >= m_iBackX && X <= (m_iBackX + m_bmBackButton.WidthPix) &&
				Y >= m_iBackY && Y <= (m_iBackY + m_bmBackButton.HeightPix))
			{
				m_bShowingControllerInfo = false;
				return WINMSG_HANDLED_REDRAW_US;
			}
			return WINMSG_NOT_HANDLED;
		}

		//See if Previous button
		if (m_iCurPage > 1 &&
			Y >= m_iPrevY && Y <= (m_iPrevY + m_bmPrevButton.HeightPix) &&
			X >= m_iPrevX && X <= (m_iPrevX + m_bmPrevButton.WidthPix))
		{
			m_iCurPage--;
			return WINMSG_HANDLED_REDRAW_US;
		}

		//See if next button
		else if (m_iCurPage < m_iNumPages &&
			Y >= m_iNextY && Y <= (m_iNextY + m_bmNextButton.HeightPix) &&
			X >= m_iNextX && X <= (m_iNextX + m_bmNextButton.WidthPix))
		{	
			m_iCurPage++;
			return WINMSG_HANDLED_REDRAW_US;
		}

		//See if in "info" column
		else if (X >= m_iInfoX && X <= (m_iInfoX + m_bmInfoButton.WidthPix))
		{
			//Determine which line number for mouse Y, 0..max screen line
			int LineNum = Y / m_iLineHeightPix;
			
			//Search through all controller lists to find which ControllerStruct this is.
			ControllerStruct *pController = NULL;
			if (FindControllerOnPage(m_pCenter, m_iCurPage, LineNum, &pController) ||
				FindControllerOnPage(m_pTracon, m_iCurPage, LineNum, &pController) ||
				FindControllerOnPage(m_pLocal, m_iCurPage, LineNum, &pController) ||
				FindControllerOnPage(m_pGround, m_iCurPage, LineNum, &pController) ||
				FindControllerOnPage(m_pClearance, m_iCurPage, LineNum, &pController))
			{
				m_bShowingControllerInfo = true;
				CreateControllerInfoPage(pController);
				return WINMSG_HANDLED_REDRAW_US;
			}
		}
	
	}

	return WINMSG_NOT_HANDLED; 
}


int CATCDlg::Update()
{

	return 1;
}

int CATCDlg::Draw(IDirect3DDevice9* pDevice)
{
	if (m_bOpen)
	{
		
		if (m_bShowingControllerInfo)
			m_pGraph->DrawBitmapToOutputBitmap(&m_bmControllerInfo, m_iX, m_iY);
		else
		{
			BitmapPageStruct *p = m_pPages;
			for (int i = 1; i < m_iCurPage; i++)
				p = p->pNext;
			if (p)
				m_pGraph->DrawBitmapToOutputBitmap(&p->Bitmap, m_iX, m_iY);
			else
			{
				int w, h;
				m_pGraph->GetStringPixelSize(ATC_EMPTY_TEXT, &w, &h);
				m_pGraph->SetFont(m_hFont);
				m_pGraph->SetTextColor(COL_DLG_TEXT);
				m_pGraph->DrawTxt(m_iX + (m_iWidthPix - w) / 2, m_iY + (m_iHeightPix - h) / 2, ATC_EMPTY_TEXT);
			}
		}
	}
	return 1;
}

int CATCDlg::Open()
{
	m_bOpen = true;
	CreatePages();
	return 1;
}

int CATCDlg::Close()
{
	m_bOpen = false;
	return 1;
}

int CATCDlg::Shutdown()
{
	DeleteFont(m_hFont);
	DeleteFont(m_hPropFont);
	m_pGraph->DeleteBM(&m_bmInfoButton);
	m_pGraph->DeleteBM(&m_bmPrevButton);
	m_pGraph->DeleteBM(&m_bmNextButton);
	m_pGraph->DeleteBM(&m_bmControllerInfo);
	m_pGraph->DeleteBM(&m_bmBackButton);

	BitmapPageStruct *pPage;
	while (m_pPages)
	{
		pPage = m_pPages;
		m_pGraph->DeleteBM(&pPage->Bitmap);
		m_pPages = m_pPages->pNext;
		delete pPage;
	}

	ControllerStruct *pC;
	while (m_pCenter)
	{
		pC = m_pCenter;
		m_pCenter = m_pCenter->pNext;
		delete pC;
	}
	while (m_pTracon)
	{
		pC = m_pTracon;
		m_pTracon = m_pTracon->pNext;
		delete pC;
	}
	while (m_pLocal)
	{
		pC = m_pLocal;
		m_pLocal = m_pLocal->pNext;
		delete pC;
	}
	while (m_pGround)
	{
		pC = m_pGround;
		m_pGround = m_pGround->pNext;
		delete pC;
	}
	while (m_pClearance)
	{
		pC = m_pClearance;
		m_pClearance = m_pClearance->pNext;
		delete pC;
	}

	m_iNumPages = 0;
	m_iNextLineNum = 0;
	return 1;
}

