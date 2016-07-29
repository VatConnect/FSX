#include "stdafx.h"
#include "C2DGraphics.h"

//////////////////////////
// CEditBox

//This control gets complicated because there are multiple ways to draw depending on 
//state: Single-line edit box with focus that scrolls horizontally so last-most text is at the
//right-most edge of box; single-line edit box that doesn't have focus where start of contents
//are drawn at left-most edge of box; then multi-line edit box that was implemented as a single-
//line edit box split up into the multiple lines (which also creates non-standard scrolling when
//the end of the box is reached -- everything scrolls left one character instead of going to 
//a new line like a user would expect). Ctrl-V/Paste is supported, but should really also support
//arrow keys, clicking on a character, and inserting/deleting from the middle of the contents 
//instead of just the end. It's also complicated because it tries to support non-proportional width
//fonts so it has to measure pixel width a lot to determine where to clip the contents to fit in 
//the box. Bottom line is this should really be re-done if there is heavy usage of it.


//Set back color to -1 for transparent. For Width note text is drawn two pixels in. bMasked if text to be shown as *'s.
//Note if height is enough to fit more than one line of text in the given font, it'll switch to "multi-line edit box"
//(see above notes).
HRESULT CEditBox::Create(C2DGraphics *pGraph, int x, int y, int Width, int Height, COLORREF TextColor, 
						 COLORREF BackColor, HFONT fon, bool bMasked)
{
	m_Graph = pGraph;
	m_iX = x;
	m_iY = y;
	m_iW = Width;
	m_iH = Height;
	m_hFont = fon;
	m_TextColor = TextColor;
	m_str[0] = 0;
	m_iNextChar = 0;
	m_bMasked = bMasked;
	m_Cursor[0] = '_';
	m_Cursor[1] = 0;

	m_Graph->MakeNewBitmap(Width, Height, &m_bitBack);
	m_Graph->SetOutputBitmap(&m_bitBack);
	if (BackColor == -1)
	{
		m_Graph->FillBitmapWithColor(0);
		m_Graph->EnableBitmapTransparency(&m_bitBack);

	}
	else
	{
		m_Graph->FillBitmapWithColor(BackColor);
	}
	m_iRealW = m_iW;

	//Determine number of lines of output. 
	HFONT OrigFont;
	m_Graph->GetFont(&OrigFont);
	m_Graph->SetFont(m_hFont);
	int w, h;
	m_Graph->GetStringPixelSize(L"W", &w, &h);
	m_iNumLines = m_iH / h;
	if (m_iNumLines > 1)
		m_iW = m_iW * m_iNumLines;
	m_Graph->SetFont(OrigFont);
	m_dwLastBlinkTime = GetTickCount();

	return S_OK;

}
void CEditBox::Shutdown()
{
	if (!m_Graph)
		return;
	if (m_pbitOn)
	{
		m_Graph->DeleteBM(m_pbitOn);
		delete m_pbitOn;
		m_pbitOn = NULL;
	}
	if (m_pbitOff)
	{
		m_Graph->DeleteBM(m_pbitOff);
		delete m_pbitOff;
		m_pbitOff = NULL;
	}
	m_Graph->DeleteBM(&m_bitBack);
	return;
}

HRESULT CEditBox::Draw()
{
	if (!m_Graph || m_iW <= 0)
		return E_FAIL;
	if (m_bHidden)
		return S_OK;

	if (m_hFont)
		m_Graph->SetFont(m_hFont);

	int w, h, p = 0, LeftJustifiedEOL = m_iNextChar;
	TCHAR CharUnderLeftJustifiedEOL = (TCHAR)0;

	//Box active? (showing cursor)
	if (m_bCursorEnabled)
	{
		//Add cursor character or space to end of string depending if 
		//it's blinking on or off. We will remove it at end
		if (m_iNextChar < (MAX_EDIT_LEN - 2))
		{
			TCHAR c = m_bCursorOn ? m_Cursor[0] : (TCHAR)' ';
			m_MaskedStr[m_iNextChar] = c;
			m_str[m_iNextChar++] = c;
			m_str[m_iNextChar] = (TCHAR)0;
			m_MaskedStr[m_iNextChar] = (TCHAR)0;
		}

		//Keep moving ahead start of string so the last-most portion will fit in the box
		//(i.e. right-justified). We do it this way (checking pixel width) because the font
		//may not be proportionally spaced.
		do
		{
			m_Graph->GetStringPixelSize(&m_str[p++], &w, &h);
		} while (w > m_iW && m_str[p] != 0 && p < (m_iMaxChar + 1));
		p--;
	}
	//Box not active, so find and zero-out end of string that fits starting at beginning of
	//string (left-justify). p (start of string we print at start of box) is left at 0.
	else
	{
		//Start end of string at end of full string and keep reducing it by one character
		//until we find the longest string that fits
		LeftJustifiedEOL = m_iNextChar;
		do
		{
			//Put a zero at EOL spot and check string length
			CharUnderLeftJustifiedEOL = m_str[LeftJustifiedEOL];
			m_str[LeftJustifiedEOL] = (TCHAR)0;
			m_Graph->GetStringPixelSize(m_str, &w, &h);

			//Restore original string
			m_str[LeftJustifiedEOL] = CharUnderLeftJustifiedEOL;
			LeftJustifiedEOL--;
		} while (w > m_iW && LeftJustifiedEOL > 0);
		LeftJustifiedEOL++;

		//Now leftjustifiedEOL points to end of string that fits -- zero it out and 
		//we will restore it after done
		CharUnderLeftJustifiedEOL = m_str[LeftJustifiedEOL];
		m_str[LeftJustifiedEOL] = 0;
	}

	//Draw background
	if (!m_bEditsLocked)
		m_Graph->DrawBitmapIntoRect(&m_bitBack, m_iX, m_iY, m_iW, m_iH, 1.0);
	
	if (m_hFont)
		m_Graph->SetFont(m_hFont);
	m_Graph->SetTextColor(m_TextColor);

	int YPos = m_iY;
	int LineHeightPix = h;
	int CurLine = 1;
	int EndOfString; //end of line index for multi-line boxes

	EndOfString = m_iNextChar;
	int EOL = EndOfString;
	TCHAR CharUnderEOL = (TCHAR)0;

	//p is start index of string to print, EOL is end, but the box could
	//have multiple lines so we need to break it up further by each line's
	//real width (m_iRealW)
	while (CurLine <= m_iNumLines && EOL > p)
	{
		//If not last line, determine one line width and set EOL to end of that line.
		//p points to start of the string, EOL to the end. 
		if (CurLine < m_iNumLines)
		{
			//Keep shrinking EOL until it fits on Real Width of line
			do
			{
				m_Graph->GetStringPixelSize(&m_str[p], &w, &h);
				if (w > m_iRealW)
				{
					m_str[EOL] = CharUnderEOL;
					EOL--;
					CharUnderEOL = m_str[EOL];
					m_str[EOL] = (TCHAR)0;
				}
			} while (w > m_iRealW && EOL > p);
		}

		//Output that line
		if (m_bMasked)
		{
			int i = m_iNextChar;
			m_MaskedStr[i] = (TCHAR)0;
			i--;
			if (m_bCursorEnabled)
				i--;
			while (i >= 0)
				m_MaskedStr[i--] = '*';
			m_Graph->DrawTxt(m_iX + 2, YPos + 1, &m_MaskedStr[p]);
		}
		else
		{
			m_Graph->DrawTxt(m_iX + 2, YPos + 1, &m_str[p]);
		}

		//Go to next line
		CurLine++;
		if (EOL > p)
		{
			m_str[EOL] = CharUnderEOL;
			p = EOL;
			YPos += LineHeightPix;
			EOL = EndOfString;
			CharUnderEOL = (TCHAR)0;
		}
	}

	//If box active, we added cursor above so now remove it
	if (m_bCursorEnabled)
	{
		if (EndOfString > 0)
		{
			EndOfString--;
			m_str[EndOfString] = (TCHAR)0;
			m_MaskedStr[EndOfString] = (TCHAR)0;
			m_iNextChar--;
		}
	}
	//Else box not active so we are left-justified. Restore zero-ed out character
	else
		m_str[LeftJustifiedEOL] = CharUnderLeftJustifiedEOL;

	return S_OK;
}

//return true if cursor has changed (i.e. needs to be redrawn)
bool CEditBox::Update()
{
	if (m_bCursorEnabled && (GetTickCount() - m_dwLastBlinkTime) >= 500)
	{
		m_bCursorOn ^= 1;
		m_dwLastBlinkTime = GetTickCount();
		return true;
	}
	return false;
}

void CEditBox::EnableCursor(bool bEnabled)
{
	if (m_bEditsLocked)
		m_bCursorEnabled = false;
	else
	{
		m_bCursorEnabled = bEnabled;
		m_bCursorOn = bEnabled ? 1 : 0;
		m_dwLastBlinkTime = GetTickCount();
	}
	return;
}

//return true if accepted (i.e. needs to be redrawn)
bool CEditBox::CharIn(TCHAR Char)
{
	if (m_bHidden || m_bEditsLocked)
		return false;

	//Ctrl-V? Note we assume TCHAR defined as WCHAR
	if (Char == 22)
	{
		if (OpenClipboard(NULL))
		{
			HANDLE hClip = GetClipboardData(CF_UNICODETEXT);
			if (hClip)
			{
				WCHAR *pText = (WCHAR *)GlobalLock(hClip);

				//Prevent paste over maximum length
				if ((int)wcslen(pText) + m_iNextChar <= m_iMaxChar)
					AppendText(pText);

				GlobalUnlock(hClip);
			}
			CloseClipboard();
		}
		return true;
	}

	//backspace
	if (Char == (TCHAR)8)
	{
		if (m_iNextChar > 0)
		{
			m_iNextChar--;
			m_str[m_iNextChar] = 0;
		}
	}
	else if (Char == (TCHAR)13)
	{
		//let owner decide this...EnableCursor(false);
	}
	else
	{
		if (m_iNextChar >= m_iMaxChar)
			return false;
		m_str[m_iNextChar++] = Char;
		m_str[m_iNextChar] = (TCHAR)0;
	}
	//reset cursor blinks
	m_dwLastBlinkTime = GetTickCount();
	m_bCursorOn = 1;
	return true;
}

void CEditBox::SetText(TCHAR *pStr)
{
	if (!pStr || m_bEditsLocked)
		return;
	int L = _tcslen(pStr);
	m_iNextChar = 0;
	for (int i = 0; i < L && m_iNextChar < m_iMaxChar; i++)
	{
		m_str[m_iNextChar++] = pStr[i];
	}
	m_str[m_iNextChar] = 0;
}

void CEditBox::AppendText(TCHAR *pStr)
{
	if (!pStr || m_bEditsLocked)
		return;
	int Len = _tcslen(pStr);
	if ((Len + m_iNextChar) > (MAX_EDIT_LEN - 1))
		return;
	_tcscpy_s(&m_str[m_iNextChar], MAX_EDIT_LEN - 1 - m_iNextChar, pStr);
	m_iNextChar += Len;
	return;
}

//Note returns pointer to our member string, so caller should copy it somewhere
//else. Do not modify it.
TCHAR* CEditBox::GetText()
{
	return m_str;
}

int CEditBox::GetTextLength()
{
	return wcslen(m_str);
}

void CEditBox::ClearText()
{
	m_iNextChar = 0;
	m_str[0] = 0;
}

bool CEditBox::IsWithin(int X, int Y)
{
	if (m_bEditsLocked)
		return false;
	if (X >= m_iX && X <= (m_iX + m_iW) && Y >= m_iY && Y <= (m_iY + m_iH))
		return true;
	return false;
}

void CEditBox::SetHidden(bool bHidden)
{
	m_bHidden = bHidden;
	return;
}

void CEditBox::DisableEdits(bool bDisable)
{
	m_bEditsLocked = bDisable;
	return;
}

//Clamp input to max number of characters (not including terminating 0). 
//Caller should make sure box width is enough to show one extra character 
//(for cursor) or it will look weird if box is full and user clicks on 
//it and no cursor appears.
void CEditBox::SetMaxChars(int iMaxChars)
{
	m_iMaxChar = iMaxChars;
	if (m_iMaxChar > MAX_EDIT_LEN - 2)
		m_iMaxChar = MAX_EDIT_LEN - 2;
	return;
}

//////////////////////////
//  CListBox

int CListBox::GetNumInList()
{
	return m_iNumInList;
}

BSTR CListBox::GetSelText()
{
	if (m_iNumInList == 0)
		return NULL;

	return (SysAllocString(m_cbsListText[m_iSelText]));
}

bool CListBox::SetSelectedText(BSTR String)
{
	for (int i = 0; i < m_iNumInList; i++)
	{
		if (m_cbsListText[i] == String)
		{
			m_iSelText = i;
			return true;
		}
	}
	//Not found -- add to list
	if (AddToList(String))
	{
		m_iSelText = m_iNumInList - 1;
		return true;
	}
	return false;
}


//Create and return true if OK 
bool CListBox::Create(int X, int Y, int Width, HFONT hFont, bool bMainBackTransparent,
	COLORREF RegTextColor, COLORREF RegBackColor, COLORREF SelTextColor,
	COLORREF SelBackColor, COLORREF BorderColor, C2DGraphics *pGraph,
	TCHAR *ButtonBitmapFilename)
{
	ClearList();
	if (!pGraph || !hFont)
		return false;

	m_pGraph = pGraph;
	m_iX = X - 2;
	m_iY = Y - 2;
	m_iWidthPix = Width;
	m_hFont = hFont;
	m_bMainBackTransparent = bMainBackTransparent;
	m_colRegText = RegTextColor;
	m_colRegBack = RegBackColor;
	m_colSelText = SelTextColor;
	m_colSelBack = SelBackColor;
	m_colBorder = BorderColor;

	//Load button and determine line height and button X
	if (!m_pGraph->LoadBitmapFromFile(ButtonBitmapFilename, &m_bitButton))
		return false;

	m_pGraph->EnableBitmapTransparency(&m_bitButton);
	m_pGraph->SetFont(m_hFont);
	int w, h;
	m_pGraph->GetStringPixelSize(L"M", &w, &m_iLineHeight);
	m_iLineHeight += 4;
	w = m_bitButton.WidthPix;
	h = m_bitButton.HeightPix;
	m_iButtonX = m_iWidthPix - w;

	return true;
}

//Delete everything from the list
void CListBox::ClearList()
{
	for (int i = 0; i < m_iNumInList; i++)
	{
		m_cbsListText[i].Empty();
		if (m_pGraph)
		{
			m_pGraph->DeleteBM(&m_bitSel[i]);
			m_pGraph->DeleteBM(&m_bitNotSel[i]);
			m_pGraph->DeleteBM(&m_bitTitle[i]);
		}
	}
	m_iNumInList = 0;
	m_iSelText = 0;
	m_bButtonDown = false;
	m_bListOpen = false;

	//m_pGraph = NULL;
	return;
}

//Add text to the end of the list and create its surfaces, true if added OK (false otherwise)
bool CListBox::AddToList(BSTR Text)
{
	USES_CONVERSION;
	if (m_iNumInList >= LB_MAX || !m_pGraph)
		return false;

	m_cbsListText[m_iNumInList] = Text;

	//Create unselected bitmap
	m_pGraph->MakeNewBitmap(m_iWidthPix, m_iLineHeight, &m_bitNotSel[m_iNumInList]);
	m_pGraph->SetOutputBitmap(&m_bitNotSel[m_iNumInList]);
	m_pGraph->FillBitmapWithColor(m_colRegBack);
	m_pGraph->SetFont(m_hFont);
	m_pGraph->SetTextColor(m_colRegText);
	m_pGraph->SetOutputBitmap(&m_bitNotSel[m_iNumInList]);
	m_pGraph->DrawTxt(8, 2, OLE2W(m_cbsListText[m_iNumInList]));

	//Create title text bitmap
	/*TODO
	m_pGraph->MakeBitmapFromOtherBitmap(m_bitNotSel[m_iNumInList], 0, 0, m_iWidthPix, m_iLineHeight,
	&m_bitTitle[m_iNumInList]);
	if (m_bMainBackTransparent)
	m_pGraph->SetBitmapTransparency(m_bitTitle[m_iNumInList], false, m_colRegBack);
	*/

	//Create selected
	m_pGraph->MakeNewBitmap(m_iWidthPix, m_iLineHeight, &m_bitSel[m_iNumInList]);
	m_pGraph->SetOutputBitmap(&m_bitSel[m_iNumInList]);
	m_pGraph->FillBitmapWithColor(m_colSelBack);
	m_pGraph->SetTextColor(m_colSelText);
	m_pGraph->SetOutputBitmap(&m_bitSel[m_iNumInList]);
	m_pGraph->DrawTxt(8, 2, OLE2W(m_cbsListText[m_iNumInList]));
	m_iNumInList++;
	return true;
}

void CListBox::Close()
{
	m_bListOpen = false;
	return;
}

void CListBox::Draw()
{
	if (!m_pGraph)
		return;

	//Draw selected text
	if (m_iNumInList > 0)
	{
		m_pGraph->DrawBitmapIntoRect(&m_bitTitle[m_iSelText], m_iX, m_iY, m_bitTitle[m_iSelText].WidthPix,
			m_bitTitle[m_iSelText].HeightPix, 1.0);
	}

	//Draw main border
	m_pGraph->SetLineColor(m_colBorder);
	m_pGraph->DrawLine(m_iX, m_iY, m_iX + m_iWidthPix - 1, m_iY, LB_THICKNESS);
	m_pGraph->LineTo(m_iX + m_iWidthPix - 1, m_iY + m_iLineHeight);
	m_pGraph->LineTo(m_iX, m_iY + m_iLineHeight);
	m_pGraph->LineTo(m_iX, m_iY);
	m_pGraph->DrawLine(m_iX + m_iButtonX - 1, m_iY, m_iX + m_iButtonX - 1, m_iY + m_iLineHeight, LB_THICKNESS);

	//Draw button
	m_pGraph->DrawBitmapIntoRect(&m_bitButton, m_iX + m_iButtonX + 2, m_iY + 6, m_iWidthPix - m_iButtonX - 4,
		m_iLineHeight - 8, 1.0f);

	//Draw list if open
	if (m_bListOpen)
	{
		//Draw Each line
		int Y = m_iY + m_iLineHeight;
		for (int i = 0; i < m_iNumInList; i++)
		{
			if (i == m_iSelText)
				m_pGraph->DrawBitmapIntoRect(&m_bitSel[i], m_iX + 1, Y + 1, m_bitSel[i].WidthPix, m_bitSel[i].HeightPix, 1.0f);
			else
				m_pGraph->DrawBitmapIntoRect(&m_bitNotSel[i], m_iX + 1, Y + 1, m_bitNotSel[i].WidthPix, m_bitNotSel[i].HeightPix, 1.0f);
			Y += m_iLineHeight;
		}

		//Draw border
		m_pGraph->DrawLine(m_iX, m_iY + m_iLineHeight, m_iX, Y, LB_THICKNESS);
		m_pGraph->LineTo(m_iX + m_iWidthPix, Y);
		m_pGraph->LineTo(m_iX + m_iWidthPix, m_iY + m_iLineHeight);
		m_pGraph->LineTo(m_iX, m_iY + m_iLineHeight);
	}

	return;
}

//Returns appropriate WINMSG_XXX
int CListBox::WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	int X, Y, Line;

	switch (message)
	{

	case WM_LBUTTONDOWN:

		X = LOWORD(lParam);
		Y = HIWORD(lParam);
		Line = GetListLineForXY(X, Y);

		//Toggling button or clicking off?
		if (Line == -100 || (m_bListOpen && Line == -99))
		{
			m_bListOpen ^= 1;
			if (!m_bListOpen)
				return WINMSG_CLOSED;
			return WINMSG_HANDLED_REDRAW_US;
		}
		//Outside box
		if (Line < 1)
			return WINMSG_NOT_HANDLED;
		//Inside box -- make it the selected and close the box
		m_iSelText = Line - 1;
		m_bListOpen = 0;
		return WINMSG_HANDLED_REDRAW_US;

	case WM_MOUSEMOVE:

		if (!m_bListOpen)
			return WINMSG_NOT_HANDLED;

		X = LOWORD(lParam);
		Y = HIWORD(lParam);
		Line = GetListLineForXY(X, Y);
		if (Line > -99 && Line != 0)
		{
			if (Line < 0)
				Line *= -1;
			if (m_iSelText != (Line - 1))
			{
				m_iSelText = Line - 1;
				return WINMSG_HANDLED_REDRAW_US;
			}
		}
		return WINMSG_NOT_HANDLED;

	case WM_KEYDOWN:

		if (!m_bListOpen)
			return WINMSG_NOT_HANDLED;

		//Close list (escape key)
		if ((int)wParam == VK_ESCAPE)
		{
			m_bListOpen = 0;
			return WINMSG_CLOSED;
		}
		return WINMSG_NOT_HANDLED;
	}

	return WINMSG_NOT_HANDLED;
}

///////////////Internal
//Return the box line number given screen X,Y -- -99 is top box, -100 is top button, otherwise
//line 0..n-1 if box is open and position within the box, or -1..-n if box open but cursor not in box
//(i.e. just line), 0 if list closed and not within the box
int CListBox::GetListLineForXY(int X, int Y)
{
	//Within lateral confines?
	if (X >= m_iX && X <= m_iX + m_iWidthPix)
	{
		//Within top line?
		if (Y >= m_iY && Y <= m_iY + m_iLineHeight)
		{
			//text area 
			if (X < m_iButtonX)
				return -99;
			//else button
			return -100;
		}
		else if (!m_bListOpen)
			return 0;

		//List is open -- above top line?
		else if (Y <= m_iY + m_iLineHeight)
			return -1;
		//Beyond bottom
		else if (Y > (m_iY + m_iLineHeight + m_iNumInList * m_iLineHeight))
			return -m_iNumInList;

		//In one of the lines
		int L = ((Y - 1 - m_iY - m_iLineHeight) / m_iLineHeight) + 1;
		if (L > (m_iNumInList + 1))
			L = (m_iNumInList + 1);
		return L;
	}
	//Outside and box closed
	else if (!m_bListOpen)
		return 0;

	//Outside and box open, just check Y value for line
	int L = ((Y - m_iY - m_iLineHeight) / m_iLineHeight) + 1;
	if (L < 0)
		L = -1;
	else if (L > m_iNumInList)
		L = -m_iNumInList;
	else
		L = -L;
	return L;
}

//////////////////////////
//	CTwoStateButton

int CTwoStateButton::Create(C2DGraphics *pGraph, int x, int y, int Width, int Height,
							BitmapStruct *pbitOn, BitmapStruct *pbitOff, bool bOnlyPushOn)
{
	m_pGraph = pGraph; m_pbitOn = pbitOn; m_pbitOff = pbitOff; m_iX = x;
	m_iY = y; m_iW = Width; m_iH = Height;
	return 1;
}

int CTwoStateButton::WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	return WINMSG_NOT_HANDLED; 
}

int CTwoStateButton::Draw()
{
	if (!m_pGraph || !m_pbitOn)
		return 0;
	if (!m_bIsVisible)
		return 1;
	return m_pGraph->DrawBitmapToOutputBitmap(m_bIsOn ? m_pbitOn : (m_pbitOff ? m_pbitOff : m_pbitOn), m_iX, m_iY);
}

int CTwoStateButton::SetPosition(int x, int y)
{
	m_iX = x;
	m_iY = y;
	return 1;
}

int CTwoStateButton::SetOn(bool bOn)
{
	m_bIsOn = bOn;
	return 1;
}

int CTwoStateButton::SetVisible(bool bVisible) 
{ 
	m_bIsVisible = bVisible; 
	return 1; 
}

bool CTwoStateButton::IsOn() 
{ 
	return m_bIsOn; 
}

bool CTwoStateButton::IsWithin(int x, int y)
{ 
	return m_bIsVisible && (x >= m_iX && x <= (m_iX + m_iW) && y >= m_iY && y <= (m_iY + m_iH)); 
}

int CTwoStateButton::Shutdown()
{
	if (!m_pGraph)
		return 1;
	if (m_pbitOn) {
		m_pGraph->DeleteBM(m_pbitOn); m_pbitOn = NULL;
	}
	if (m_pbitOff) {
		m_pGraph->DeleteBM(m_pbitOff); m_pbitOff = NULL;
	}
	return 1;
}