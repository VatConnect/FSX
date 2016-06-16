#include "stdafx.h"
#include "Dialogs.h"

#define WX_FONT L"Lucida Console"
#define WX_EDIT_LABEL L"ICAO Airport: "
#define WX_TMARGIN_PIX 1     //top margin in pixels
#define WX_LMARGIN_PIX 1     //left margin in pixels

CWXDlg::CWXDlg() : m_bOpen(false), m_bEditHasFocus(false)
{
}

CWXDlg::~CWXDlg()
{
}

int CWXDlg::Initialize(CMainDlg *pMainDlg, HWND hWnd, C2DGraphics *pGraph, int X, int Y, 
					   int WidthPix, int HeightPix)
{
	m_pGraph = pGraph;
	m_iX = X;
	m_iY = Y;
	m_iWidthPix = WidthPix;
	m_iHeightPix = HeightPix;
	m_hWnd = hWnd; 
	m_pMainDlg = pMainDlg;
	m_bEditHasFocus = false;

	int iCharWidthPix, iCharHeightPix;
	m_pGraph->FindBestFont(WX_FONT, FONT_SIZE, true, false, false, &m_hFont);
	m_pGraph->SetFont(m_hFont);
	m_pGraph->GetStringPixelSize(L"W", &iCharWidthPix, &iCharHeightPix);
	m_iTextWidthChar = (m_iWidthPix - WX_LMARGIN_PIX * 2) / iCharWidthPix;    //2* for right margin 
	m_iTextHeightChar = (m_iHeightPix - WX_TMARGIN_PIX) / iCharHeightPix - 2;  //2 at bottom for edit box
	m_iLineHeightPix = iCharHeightPix;

	//Create background bitmap
	m_pGraph->MakeNewBitmap(m_iWidthPix, m_iHeightPix, &m_bitBackground);
	m_pGraph->SetOutputBitmap(&m_bitBackground);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);
	m_pGraph->SetLineColor(COL_SCREEN_OUTLINE);
	m_pGraph->DrawLine(0, m_iHeightPix - 2 * iCharHeightPix - 1, m_iWidthPix, m_iHeightPix -
		m_iLineHeightPix * 2 - 1, 1);             //line above edit box

	//Create text area bitmap
	m_pGraph->MakeNewBitmap(m_iWidthPix - 4, m_iHeightPix - 2 * m_iLineHeightPix - 3, &m_bitText);
	m_pGraph->SetOutputBitmap(&m_bitText);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);
	
	//Determine edit box and label positions
	int EditY = m_iHeightPix - m_iLineHeightPix * 3 / 2;
	int EditW = iCharWidthPix * 5 + 2;
	int LW, LH, LX;

	//Draw label onto background bitmap
	m_pGraph->GetStringPixelSize(WX_EDIT_LABEL, &LW, &LH);
	LX = (m_iWidthPix - (LW + EditW)) / 2;
	m_pGraph->SetOutputBitmap(&m_bitBackground); 
	m_pGraph->SetTextColor(COL_DLG_TEXT);
	m_pGraph->DrawTxt(LX, EditY, WX_EDIT_LABEL);
	
	//Create edit box
	m_editTextIn.Create(m_pGraph, LX + LW, EditY, EditW, iCharHeightPix + 1, COL_DLG_TEXT, 
		COL_EDITBOX_BACK, m_hFont);
	m_editTextIn.SetMaxChars(4);

	//Create initial output bitmap
	m_pGraph->MakeNewBitmap(m_iWidthPix, m_iHeightPix, &m_bitOutput);
	UpdateOutputBitmap();

	return 1;
}

int CWXDlg::UpdateOutputBitmap()
{
	m_pGraph->SetOutputBitmap(&m_bitOutput);
	m_pGraph->DrawBitmapToOutputBitmap(&m_bitBackground, 0, 0);
	m_pGraph->DrawBitmapToOutputBitmap(&m_bitText, 2, 2);      
	m_editTextIn.Draw();
	m_pGraph->CopyBitmapDCToSurface(&m_bitOutput);
	m_pMainDlg->OnChildInitiatedRedraw(); 

	return 1;
}

//Indicate edit box has focus (true) or not (false), redraw edit and update output surface
int CWXDlg::SetEditboxFocus(bool bHasFocus)
{
	m_bEditHasFocus = bHasFocus;
	m_editTextIn.EnableCursor(bHasFocus);
	DrawEditboxOnly();
	m_pMainDlg->GetKeyboardInput(bHasFocus);
	return 1;
}

//Draw just the editbox to output and update output's surface
int CWXDlg::DrawEditboxOnly()
{
	m_pGraph->SetOutputBitmap(&m_bitOutput);
	m_editTextIn.Draw();
	m_pGraph->CopyBitmapDCToSurface(&m_bitOutput);
	return 1;
}

//Draw the text string to the text area bitmap
int CWXDlg::SetText(WCHAR *pText)
{
	WCHAR Buffer[1024];
	wcscpy_s(Buffer, 1024, pText);

	int Line = 0, Len;
	WCHAR *pNext = Buffer;
	WCHAR CharUnderneath;

	m_pGraph->SetOutputBitmap(&m_bitText);
	m_pGraph->SetFont(m_hFont);
	m_pGraph->SetTextColor(COL_DLG_TEXT);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);

	do
	{
		Len = wcslen(pNext);
		if (Len > 0)
		{
			//Less than one line? 
			if (Len <= m_iTextWidthChar)
			{
				m_pGraph->DrawTxt(1, Line * m_iLineHeightPix, pNext);
				pNext += Len; //will point to terminating zero
			}
			//Greater - zero out string at max
			else
			{
				CharUnderneath = pNext[m_iTextWidthChar];
				pNext[m_iTextWidthChar] = (WCHAR)0;
				m_pGraph->DrawTxt(1, Line * m_iLineHeightPix, pNext);
				pNext[m_iTextWidthChar] = CharUnderneath;
				pNext += m_iTextWidthChar;
				Line++;
			}
		}
	} 
	while (Len > 0 && Line < m_iTextHeightChar);

	UpdateOutputBitmap();
	return 1;
}

/////////////
//Base class

int CWXDlg::WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	static const DWORD CharFilter = ALT_KEY_PRESSED | PREV_PRESSED;

	if (!m_bOpen)
		return WINMSG_NOT_HANDLED;

	if (message == WM_CHAR && m_bEditHasFocus)
	{
		//Ignore alt+key, and only handle first keydown except bksp/char 8
		if (!(lParam & CharFilter) || (wParam == 8 && !(lParam & ALT_KEY_PRESSED)))
		{
			//Return key?
			if (wParam == 13)
			{
				WCHAR *pStr = m_editTextIn.GetText();
				m_pMainDlg->OnRequestWeather(pStr);
				m_editTextIn.ClearText();
				DrawEditboxOnly();
			}
			else
			{
				m_editTextIn.CharIn((TCHAR)wParam);
				DrawEditboxOnly();
			}
			return WINMSG_HANDLED_REDRAW_US;
		}
	}

	if (message == WM_KILLFOCUS || (message == WM_ACTIVATE && wParam == WA_INACTIVE))
	{
		if (m_bEditHasFocus)
		{
			SetEditboxFocus(false);
			m_pMainDlg->OnChildInitiatedRedraw();
			return WINMSG_HANDLED_REDRAW_US;
		}
		return WINMSG_NOT_HANDLED;
	}

	if (message == WM_LBUTTONDOWN)
	{
		int X = GET_X_LPARAM(lParam) - m_iX, Y = GET_Y_LPARAM(lParam) - m_iY;

		//See if click on edit box 
		if (m_editTextIn.IsWithin(X, Y))
		{
			if (!m_bEditHasFocus)
			{
				SetEditboxFocus(true);
				m_bEditHasFocus = true;
				return WINMSG_HANDLED_REDRAW_US;
			}
			return WINMSG_HANDLED_NO_REDRAW;
		}

		//else clicked somewhere other than edit box -- if we had focus, remove it
		if (m_bEditHasFocus)
		{
			SetEditboxFocus(false);
			m_bEditHasFocus = true;
			DrawEditboxOnly();
			m_pMainDlg->OnChildInitiatedRedraw();
		}
	}

	return -1;
}

//Handle editbox cursor blink
int CWXDlg::Update()
{
	if (m_bEditHasFocus)
	{
		if (m_editTextIn.Update())
		{
			DrawEditboxOnly();
			m_pMainDlg->OnChildInitiatedRedraw();
			return WINMSG_HANDLED_REDRAW_US;
		}
	}
	return WINMSG_NOT_HANDLED;
}

int CWXDlg::Draw(IDirect3DDevice9* pDevice)
{
	if (m_bOpen)
		m_pGraph->DrawBitmapToOutputBitmap(&m_bitOutput, m_iX, m_iY);

	return 0;
}

int CWXDlg::Open()
{
	m_bOpen = true;
	SetEditboxFocus(true);     //automatically focus on editbox 

	return 0;
}

int CWXDlg::Close()
{
	m_bOpen = false;

	//Don't need to remove editbox focus because clicking the "close" button
	//already did it.

	return 0;
}

int CWXDlg::Shutdown()
{
	m_pGraph->DeleteBM(&m_bitBackground);
	m_pGraph->DeleteBM(&m_bitText);
	m_pGraph->DeleteBM(&m_bitOutput);
	m_editTextIn.Shutdown();

	return 1;
}