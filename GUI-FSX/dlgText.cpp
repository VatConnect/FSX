#include "stdafx.h"
#include "Dialogs.h"

#define TEXT_FONT L"Lucida Console"
#define TEXT_TMARGIN_PIX 1     //top margin in pixels
#define TEXT_LMARGIN_PIX 1     //left margin in pixels
#define SCROLLBAR_X_MARGIN_MULTIPLE 2  //number of scrollbar widths on left and right to still consider "grabbing" it
                                       //because slider is really small otherwise

CTextDlg::CTextDlg() : m_bOpen(false), m_bEditHasFocus(false), m_bScrollLocked(false), m_bDraggingScrollbar(false)
{
}

CTextDlg::~CTextDlg()
{
}

int CTextDlg::Initialize(CMainDlg *pMainDlg, HWND hWnd, C2DGraphics *pGraph, int X, int Y, int WidthPix, int HeightPix)
{
	m_pGraph = pGraph;
	m_iX = X;
	m_iY = Y;
	m_iWidthPix = WidthPix;
	m_iHeightPix = HeightPix;
	m_hWnd = hWnd;
	m_pMainDlg = pMainDlg;
	m_iFirstLine = 0;   //circle m_LineBuffer indices 
	m_iNextLine = 0;
	m_iTopScreenLine = 0; 
	m_bEditHasFocus = false;
	m_bScrollLocked = false;
	
	int iCharWidthPix, iCharHeightPix;
	m_pGraph->FindBestFont(TEXT_FONT, FONT_SIZE, false, false, false, &m_hFont);
	m_pGraph->SetFont(m_hFont);
	m_pGraph->GetStringPixelSize(L"W", &iCharWidthPix, &iCharHeightPix);
	m_iTextWidthChar = (m_iWidthPix - TEXT_LMARGIN_PIX) / iCharWidthPix - 1;     //1 on right for scrollbar
	if (m_iTextWidthChar > MAX_TEXT_DLG_COLUMNS - 1)
		m_iTextWidthChar = MAX_TEXT_DLG_COLUMNS - 1;
	m_iTextHeightChar = (m_iHeightPix - TEXT_TMARGIN_PIX) / iCharHeightPix - 1;  //1 at bottom for edit box
	m_iLineHeightPix = iCharHeightPix;

	//Create background bitmap
	m_pGraph->MakeNewBitmap(m_iWidthPix, m_iHeightPix, &m_bitBackground);
	m_pGraph->SetOutputBitmap(&m_bitBackground);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);
	m_pGraph->SetLineColor(COL_SCREEN_OUTLINE);
	m_pGraph->DrawLine(0, m_iHeightPix - m_iLineHeightPix - 1, m_iWidthPix, m_iHeightPix - 
		m_iLineHeightPix - 1, 1); //line above text box
	m_pGraph->DrawLine(m_iWidthPix - iCharWidthPix - 1, 0, m_iWidthPix - iCharWidthPix - 1, 
		m_iHeightPix - m_iLineHeightPix - 1, 1); //line left of scrollbar

	//Create up and down buttons
	BitmapStruct *pUp = new BitmapStruct;
	BitmapStruct *pDown = new BitmapStruct;
	m_pGraph->MakeNewBitmap(iCharWidthPix, iCharHeightPix, pUp);
	m_pGraph->SetOutputBitmap(pUp);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);
	m_pGraph->SetTextColor(COL_DLG_TEXT);
	m_pGraph->DrawTxt(0, 0, L"\x25B2");
	m_butScrollUp.Create(m_pGraph, m_iWidthPix - iCharWidthPix, 0, iCharWidthPix, 
		iCharHeightPix, pUp, NULL);

	m_pGraph->MakeNewBitmap(iCharWidthPix, iCharHeightPix, pDown);
	m_pGraph->SetOutputBitmap(pDown);
	m_pGraph->FillBitmapWithColor(COL_DLG_BACK);
	m_pGraph->SetTextColor(COL_DLG_TEXT);
	m_pGraph->DrawTxt(0, 0, L"\x25BC");
	m_butScrollDown.Create(m_pGraph, m_iWidthPix - iCharWidthPix, m_iHeightPix - 2 * 
		m_iLineHeightPix - 1, iCharWidthPix, iCharHeightPix, pDown, NULL);

	//Determine scrollbar top Y and height
	m_iScrollSpaceTopY = m_butScrollUp.m_iY + m_butScrollUp.m_iH;
	m_iScrollSpaceHeightPix = m_butScrollDown.m_iY - m_iScrollSpaceTopY;
	
	//Create scrollbar bitmap and set initial position just above down button
	m_iScrollWidthPix = iCharWidthPix;
	m_iScrollHeightPix = iCharHeightPix / 2;
	m_pGraph->MakeNewBitmap(m_iScrollWidthPix, m_iScrollHeightPix, &m_bitScrollbar);
	m_pGraph->SetOutputBitmap(&m_bitScrollbar);
	m_pGraph->FillBitmapWithColor(COL_TEXTBOX_SCROLLBAR);
	m_iScrollX = m_iWidthPix - iCharWidthPix;
	m_iScrollY = m_butScrollDown.m_iY - m_iScrollHeightPix;

	//Initialize and position edit box
	m_editTextIn.Create(m_pGraph, 0, m_iHeightPix - iCharHeightPix, m_iWidthPix, 
		iCharHeightPix, COL_DLG_TEXT, COL_EDITBOX_BACK, m_hFont);

	//Create initial output bitmap
	m_pGraph->MakeNewBitmap(m_iWidthPix, m_iHeightPix, &m_bitOutput);
	UpdateOutputBitmap();

	return 1;
}

//Add string to our output screen, could be any length. Break it up into
//actual lines depending on our window size.
int CTextDlg::AddText(WCHAR *pText, COLORREF col)
{
	if (!pText)
		return 1;

	int size = wcslen(pText);
	
	//One liner?
	if (size <= m_iTextWidthChar)
		AddLine(pText, col);
	
	//Needs to be broken up... put 0's at max position for each line and 
	//add substrings individually
	else
	{
		WCHAR Buffer[1024];
		wcscpy_s(Buffer, 1024, pText);
		WCHAR *pStart = Buffer;
		WCHAR CharUnder;
		while (size > 0)
		{
			//Zero out last column
			CharUnder = pStart[m_iTextWidthChar];
			pStart[m_iTextWidthChar] = WCHAR(0);

			//Add the string
			AddLine(pStart, col);

			//Restore character underneath and move up start
			pStart += m_iTextWidthChar;
			*pStart = CharUnder;
			size -= m_iTextWidthChar;
			if (CharUnder == WCHAR(0))
				size = 0;
		}
	}
	UpdateOutputBitmap();
	return 1;
}
int CTextDlg::ClearAll()
{
	m_iFirstLine = 0;
	m_iNextLine = 0;
	m_iTopScreenLine = 0;
	m_bScrollLocked = false;

	return 1; 
}

//Add single line of text, length must be < m_iTextWidthChar
int CTextDlg::AddLine(WCHAR *pText, COLORREF col)
{
	wcscpy_s(m_wcsLineBuffer[m_iNextLine], MAX_TEXT_DLG_COLUMNS, pText);
	m_colLineColors[m_iNextLine] = col;

	m_iNextLine++;
	
	//Buffer rollover?
	if (m_iNextLine == MAX_TEXT_DLG_LINES)
		m_iNextLine = 0;
	if (m_iNextLine == m_iFirstLine)
	{
		m_iFirstLine++;
		if (m_iFirstLine == MAX_TEXT_DLG_LINES)
			m_iFirstLine = 0;
	}
	return 1;
}
//Assemble/refresh output bitmap. Start by copying over blank background,
//then visible text for current scrollbar setting, then scrollbar and edit box. 
//Then updates texture so it's ready for drawing.
int CTextDlg::UpdateOutputBitmap()
{
	m_pGraph->SetOutputBitmap(&m_bitOutput);
	m_pGraph->DrawBitmapToOutputBitmap(&m_bitBackground, 0, 0);
	m_pGraph->DrawBitmapToOutputBitmap(&m_bitScrollbar, m_iScrollX, m_iScrollY);
	m_butScrollUp.Draw();
	m_butScrollDown.Draw();
	m_pGraph->SetFont(m_hFont);

	//Draw text -- if scroll not fixed on a line, draw last m_iTextHeightChar in buffer
	if (!m_bScrollLocked)
	{
		//If not empty buffer
		if (m_iFirstLine != m_iNextLine)
		{
			//Buffer not wrapped around?
			if (m_iNextLine > m_iFirstLine)
			{
				int iTopIndex = m_iNextLine - m_iTextHeightChar;
				int y = TEXT_TMARGIN_PIX;
				if (iTopIndex < 0)
					iTopIndex = 0;
				m_iTopScreenLine = iTopIndex;
				for (int i = iTopIndex; i < m_iNextLine; i++)
				{
					m_pGraph->SetTextColor(m_colLineColors[i]);
					m_pGraph->DrawTxt(TEXT_LMARGIN_PIX, y, m_wcsLineBuffer[i]);
					y += m_iLineHeightPix;
				}
			}
			//Buffer wrapped around (next line above first line)
			else
			{
				int iTopIndex = m_iNextLine - m_iTextHeightChar;
				int y = TEXT_TMARGIN_PIX;
				if (iTopIndex < 0)
				{
					iTopIndex += MAX_TEXT_DLG_LINES;
					m_iTopScreenLine = iTopIndex;

					//Draw from top index to end
					for (int i = iTopIndex; i < MAX_TEXT_DLG_LINES; i++)
					{
						m_pGraph->SetTextColor(m_colLineColors[i]);
						m_pGraph->DrawTxt(TEXT_LMARGIN_PIX, y, m_wcsLineBuffer[i]);
						y += m_iLineHeightPix;
					}
					iTopIndex = 0;
				}
				else
					m_iTopScreenLine = iTopIndex;

				//Draw from iTopIndex to m_iNextLine
				for (int i = iTopIndex; i < m_iNextLine; i++)
				{
					m_pGraph->SetTextColor(m_colLineColors[i]);
					m_pGraph->DrawTxt(TEXT_LMARGIN_PIX, y, m_wcsLineBuffer[i]);
					y += m_iLineHeightPix;
				}
			}
		}
	}
	//Buffer "frozen" starting output at m_iTopScreenLine
	else
	{
		int y = TEXT_TMARGIN_PIX;

		//Buffer wraps around?
		if (m_iNextLine < m_iTopScreenLine)
		{
			bool bNeedToWrap = false;
			int WrapLines = 0;
			int BottomIndex = m_iTopScreenLine + m_iTextHeightChar;
			if (BottomIndex > MAX_TEXT_DLG_LINES)
			{
				bNeedToWrap = true;
				WrapLines = BottomIndex - MAX_TEXT_DLG_LINES;
				BottomIndex = MAX_TEXT_DLG_LINES;
			}
			for (int i = m_iTopScreenLine; i < BottomIndex; i++)
			{
				m_pGraph->SetTextColor(m_colLineColors[i]);
				m_pGraph->DrawTxt(TEXT_LMARGIN_PIX, y, m_wcsLineBuffer[i]);
				y += m_iLineHeightPix;
			}
			if (bNeedToWrap)
			{
				for (int i = 0; i < WrapLines && i < m_iNextLine; i++)
				{
					m_pGraph->SetTextColor(m_colLineColors[i]);
					m_pGraph->DrawTxt(TEXT_LMARGIN_PIX, y, m_wcsLineBuffer[i]);
					y += m_iLineHeightPix;
				}
			}
		}
		//Buffer doesn't wrap
		else
		{
			int BottomIndex = m_iTopScreenLine + m_iTextHeightChar;
			if (BottomIndex > m_iNextLine)
				BottomIndex = m_iNextLine;
			for (int i = m_iTopScreenLine; i < BottomIndex; i++)
			{
				m_pGraph->SetTextColor(m_colLineColors[i]);
				m_pGraph->DrawTxt(TEXT_LMARGIN_PIX, y, m_wcsLineBuffer[i]);
				y += m_iLineHeightPix;
			}
		}
	}
			
	m_editTextIn.Draw();
	m_pGraph->CopyBitmapDCToSurface(&m_bitOutput);
	m_pMainDlg->OnChildInitiatedRedraw(); //refresh parent

	return 1;
}

//Indicate edit box has focus (true) or not (false), redraw edit and update output surface
int CTextDlg::SetEditboxFocus(bool bHasFocus)
{
	m_bEditHasFocus = bHasFocus;
	m_editTextIn.EnableCursor(bHasFocus);
	DrawEditboxOnly();
	m_pMainDlg->GetKeyboardInput(bHasFocus);
	return 1;
}

//Draw just the editbox to output and update output's surface
int CTextDlg::DrawEditboxOnly()
{
	m_pGraph->SetOutputBitmap(&m_bitOutput);
	m_editTextIn.Draw();
	m_pGraph->CopyBitmapDCToSurface(&m_bitOutput);
	return 1;
}

//Position the scrollbar slider to proper position for current m_iTopScreenLine.
//Caller will handle redrawing. Happens when user clicks up or down buttons instead
//of scrollbar, or text added but screen "locked" on other line
int CTextDlg::SetScrollbarToScreenline()
{
	//Bottom-most pixel where we can draw the scrollbar
	int BottomYPix = m_iScrollSpaceTopY + m_iScrollSpaceHeightPix - m_iScrollHeightPix;

	//At bottom?
	if (!m_bScrollLocked)
	{
		m_iScrollY = BottomYPix;
		return 1;
	}

	//Get total number of lines in buffer
	int TotalLines;
	if (m_iNextLine >= m_iFirstLine)
		TotalLines = m_iNextLine - m_iFirstLine;
	else
		TotalLines = MAX_TEXT_DLG_LINES;

	//Get top line number of bottom-most screen
	int TopOfBottom = TotalLines - m_iTextHeightChar - m_iFirstLine;
	if (TopOfBottom <= 0)
	{
		m_iScrollY = BottomYPix;
		return 1;
	}
	
	//Calculate percentage down the display that the scrollbar should be
	float Pct = (float)(m_iTopScreenLine - m_iFirstLine) / (float)TopOfBottom;
	if (Pct > 1.0f)
		Pct = 1.0f;
	
	m_iScrollY = m_iScrollSpaceTopY + (int)(Pct * (float)(BottomYPix - m_iScrollSpaceTopY));
	return 1;
}

//Calculate m_iTopScreenline for current scrollbar slider position. Caller must
//handle redrawing. Happens when user moves scrollbar slider.
int CTextDlg::SetScreenlineToScrollbar()
{
	//Calculate percentage down the display that the scrollbar is
	float Pct = (float)(m_iScrollY - m_iScrollSpaceTopY) / (float)(m_iScrollSpaceHeightPix - m_iScrollHeightPix);
		
	//Get total number of lines in buffer
	int TotalLines;
	if (m_iNextLine >= m_iFirstLine)
		TotalLines = m_iNextLine - m_iFirstLine;
	else
		TotalLines = MAX_TEXT_DLG_LINES;

	//Get top line number of bottom-most screen (corresponding to 1.0 percentage)
	int TopOfBottom = TotalLines - m_iTextHeightChar + m_iFirstLine;
	if (TopOfBottom < 0)
		TopOfBottom = 0;
	else if (TopOfBottom >= MAX_TEXT_DLG_LINES)
		TopOfBottom -= MAX_TEXT_DLG_LINES;
	
	int PrevTop = m_iTopScreenLine;
	m_iTopScreenLine = (int)(Pct * (float)TopOfBottom); 
	if (m_iTopScreenLine < m_iFirstLine)
		m_iTopScreenLine = m_iFirstLine;
	
	//Check if scrolled to bottom (or close enough) and clamp to bottom and undo scroll locked
	if (m_iTopScreenLine >= PrevTop && abs(m_iTopScreenLine - TopOfBottom) < 2)
	{
		m_iTopScreenLine = TopOfBottom;
		m_iScrollY = m_iScrollSpaceTopY + m_iScrollSpaceHeightPix - m_iScrollHeightPix;
		m_bScrollLocked = false;
	}
	else
		m_bScrollLocked = true;

	return 1;
} 

/////////////
//Base class

int CTextDlg::WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam)
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
				WCHAR *pStr;
				m_editTextIn.GetText(&pStr);
				m_pMainDlg->OnSendText(pStr);
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
				return WINMSG_HANDLED_REDRAW_US;
			}
			return WINMSG_HANDLED_NO_REDRAW;
		}

		//else clicked somewhere other than edit box -- if we had focus, remove it
		if (m_bEditHasFocus)
			SetEditboxFocus(false);

		//Check up-arrow button
		if (m_butScrollUp.IsWithin(X, Y))
		{
			//Ignore if still on first screenful of text
			if (m_iFirstLine == 0 && m_iNextLine < m_iTextHeightChar)
				return WINMSG_HANDLED_NO_REDRAW;

			//>= one screenful. If not currently locked, do so
			if (!m_bScrollLocked)
				m_bScrollLocked = true;
            
			if (m_iTopScreenLine != m_iFirstLine)
			{
				m_iTopScreenLine--;
				if (m_iTopScreenLine < 0)
					m_iTopScreenLine += MAX_TEXT_DLG_LINES;
				SetScrollbarToScreenline();
				UpdateOutputBitmap();
				return WINMSG_HANDLED_REDRAW_US;
			}
			return WINMSG_HANDLED_NO_REDRAW;
		}

		//Down-arrow
		if (m_butScrollDown.IsWithin(X, Y))
		{
			//Ignore if still on first screenful of text or not locked
			if (!m_bScrollLocked || (m_iFirstLine == 0 && m_iNextLine < m_iTextHeightChar))
				return WINMSG_HANDLED_NO_REDRAW;
			
			//Ignore and unlock if we're at bottom
			int BotLine = m_iTopScreenLine + m_iTextHeightChar;
			if (BotLine >= MAX_TEXT_DLG_LINES)
				BotLine -= MAX_TEXT_DLG_LINES;
			if (BotLine == m_iNextLine)
			{
				m_bScrollLocked = false;
				return WINMSG_HANDLED_NO_REDRAW;
			}

			m_iTopScreenLine++;
			if (m_iTopScreenLine >= MAX_TEXT_DLG_LINES)
				m_iTopScreenLine -= MAX_TEXT_DLG_LINES;
			SetScrollbarToScreenline();
			UpdateOutputBitmap();
			return WINMSG_HANDLED_REDRAW_US;
		}

		//Scrollbar
		if (X >= m_iScrollX - (SCROLLBAR_X_MARGIN_MULTIPLE * m_iScrollWidthPix) && 
			X < (m_iScrollX + m_iScrollWidthPix + SCROLLBAR_X_MARGIN_MULTIPLE * m_iScrollWidthPix) && 
			Y >= m_iScrollY && Y < (m_iScrollY + m_iScrollHeightPix))
		{
			m_bDraggingScrollbar = true;
			return WINMSG_HANDLED_NO_REDRAW;
		}
	}

	if (m_bDraggingScrollbar)
	{
		if (message == WM_LBUTTONUP)
		{
			m_bDraggingScrollbar = false;
			return WINMSG_HANDLED_NO_REDRAW;
		}
		if (message == WM_MOUSEMOVE)
		{
			int X = GET_X_LPARAM(lParam) - m_iX, Y = GET_Y_LPARAM(lParam) - m_iY;

			//Mouse too far left or right from scrollbar? (2X width buffer either side)
			if (X < (m_iScrollX - SCROLLBAR_X_MARGIN_MULTIPLE * m_iScrollWidthPix) || 
				X > (m_iScrollX + m_iScrollWidthPix + m_iScrollWidthPix * SCROLLBAR_X_MARGIN_MULTIPLE))
				return WINMSG_HANDLED_NO_REDRAW;

			m_iScrollY = Y; 

			//Clamp Y to ends
			if (m_iScrollY < m_iScrollSpaceTopY)
				m_iScrollY = m_iScrollSpaceTopY;
			else if (m_iScrollY > (m_iScrollSpaceTopY + m_iScrollSpaceHeightPix - m_iScrollHeightPix))
				m_iScrollY = (m_iScrollSpaceTopY + m_iScrollSpaceHeightPix - m_iScrollHeightPix);
	
			SetScreenlineToScrollbar();
			UpdateOutputBitmap();

			return WINMSG_HANDLED_REDRAW_US;
		}
	}
	
	return WINMSG_NOT_HANDLED;
}

int CTextDlg::Update()
{
	if(m_bEditHasFocus)
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

int CTextDlg::Draw(IDirect3DDevice9* pDevice)
{
	if (m_bOpen)
		m_pGraph->DrawBitmapToOutputBitmap(&m_bitOutput, m_iX, m_iY);

	return 1;
}

int CTextDlg::Open()
{
	m_bOpen = true;

	return 1;
}

int CTextDlg::Close()
{
	m_bOpen = false;
	return 1;
}

int CTextDlg::Shutdown()
{
	m_pGraph->DeleteBM(&m_bitBackground);
	m_pGraph->DeleteBM(&m_bitScrollbar);
	m_pGraph->DeleteBM(&m_bitOutput);
	m_butScrollUp.Shutdown();
	m_butScrollDown.Shutdown();
	m_editTextIn.Shutdown();

	return 1;
}