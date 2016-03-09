#pragma once
#include <d3d9.h>
#include <atlbase.h>
#include <WinGDI.h>
#pragma comment(lib, "Msimg32.lib")  //for alphablend and transparentblit

typedef struct BitmapStruct
{
	HDC hDC;                         //memory DC bitmap is loaded into (for modifying)
	IDirect3DSurface9* pSurface;     //The surface, could be NULL if not asked to draw yet -- get GDI wsurface with pSurface->GetDC() then ->ReleaseDC when done.
	IDirect3DDevice9* pDevice;       //The D3D device the surface was made for -- we do NOT do addref and release because it's owned by someone else
	long WidthPix;
	long HeightPix;
	bool bHasTransparency;           //True if has a transparent color set
	COLORREF TransparentColor;      
	BitmapStruct() : hDC(NULL), pSurface(NULL), pDevice(NULL), bHasTransparency(false){};
} BitmapStruct;

typedef class C2DGraphics
{
public:

	C2DGraphics();
	~C2DGraphics();

	int Initialize(IDirect3DDevice9 *pDevice);
	int LoadBitmapFromFile(const WCHAR *Filename, BitmapStruct *pBitmap);
	int MakeNewBitmap(int Width, int Height, BitmapStruct *pBitmap);
	int EnableBitmapTransparency(BitmapStruct *pBitmap);
	int DrawBitmapSurfaceOnDevice(BitmapStruct *pBitmap, IDirect3DDevice9 *pDevice, long ScreenX, long ScreenY);
	int CopyBitmapDCToSurface(BitmapStruct *pBitmap);
	int DrawBitmapToOutputBitmap(BitmapStruct *pBitmap, long X, long Y, float Alpha = 1.0f);
	int DeleteBM(BitmapStruct *pBitmap);
	int Shutdown();

	//Drawing functions that modify/draw onto bitmap set with "SetOutputBitmap"
	int SetOutputBitmap(BitmapStruct *pBitmap);
	int FillBitmapWithColor(COLORREF Color);
	int DrawBitmapIntoRect(BitmapStruct *pBitmap, int x, int y,int Width, int Height, float Alpha);
	int SetFont(HFONT hFont);
	int SetTextColor(COLORREF Color);
	int GetStringPixelSize(WCHAR *Str, int *Width, int *Height);
	int SetLineColor(COLORREF Color);
	int DrawLine(int fx, int fy, int tx, int ty, int Thickness);
	int LineTo(int tx, int ty);
	
	//Text functions for any bitmap
	int FindBestFont(WCHAR *FontName, int PointSize, int bBold, int bItalic, int bUnderline, HFONT *phFont);
	int DrawTxt(int x, int y, WCHAR *Str);

protected:

	HFONT m_hFont;                  //Our text font 
	COLORREF m_TextColor;
	HDC m_hMasterDC;                //cached reference DC based on desktop window
	IDirect3DDevice9 *m_pMasterDevice; //cached master device
	BitmapStruct *m_pOutputBitmap;  //Current "output" bitmap -- what drawing functions draw onto
	HPEN m_hPen;                    //Current pen for line drawing
	int m_iPenThickness;        
	COLORREF m_LineColor;           //Current line color
	int m_iLastLineX;               //Last "to" line draw point (for calling successive LineTo's)
	int m_iLastLineY;

} C2DGraphics;

//////////////
//Controls

//return code for dialogs and controls handling WindowsMessage
typedef enum WINMSG_RESULT
{
	WINMSG_CLOSED = -1,
	WINMSG_NOT_HANDLED,
	WINMSG_HANDLED_NO_REDRAW,
	WINMSG_HANDLED_REDRAW_US,
	WINMSG_HANDLED_REDRAW_ALL
} WINMSG_RESULT;

//Base class for on/off buttons (toggle and momentary)
typedef class CTwoStateButton
{
public:

	C2DGraphics*   m_pGraph;
	BitmapStruct*  m_pbitOn;
	BitmapStruct*  m_pbitOff;
	int            m_iX;
	int            m_iY;
	int            m_iW;
	int            m_iH;
	bool           m_bIsOn;
	bool		   m_bOnlyPushOn;

	CTwoStateButton() : m_bIsOn(false), m_bOnlyPushOn(false), m_pbitOn(NULL), m_pbitOff(NULL), m_pGraph(NULL){};

	int Create(C2DGraphics *pGraph, int x, int y, int Width, int Height,
		BitmapStruct *pbitOn, BitmapStruct *pbitOff, bool bOnlyPushOn = false) 
	{
		m_pGraph = pGraph; m_pbitOn = pbitOn; m_pbitOff = pbitOff; m_iX = x;
		m_iY = y; m_iW = Width; m_iH = Height; 
		return 1;
	};
	int WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam) { return WINMSG_NOT_HANDLED; };
	int Draw() 
	{ 
		if (!m_pGraph || !m_pbitOn || !m_pbitOff)
			return 0;
		return m_pGraph->DrawBitmapToOutputBitmap(m_bIsOn? m_pbitOn : m_pbitOff, m_iX, m_iY);
	};
	int SetPosition(int x, int y) {m_iX = x; m_iY = y; return 1;};
	int SetOn(bool bOn) {m_bIsOn = bOn; return 1;};
	bool IsWithin(int x, int y) { return (x >= m_iX && x <= (m_iX + m_iW) && y >= m_iY && y <= (m_iY + m_iH)); };

} CTwoStateButton;

//A button that alternates between on and off each time it's pushed 
typedef class CToggleButton : public CTwoStateButton
{
public:
	void Push() { m_bIsOn ^= 1;};
    bool IsPushed(){return m_bIsOn;};
	void SetState(bool bPushed){m_bIsOn = bPushed;};
 	
} CToggleButton;

//A button that is only on while being pressed, and pops back off when mouseclick released 
typedef class CMomentaryButton : public CTwoStateButton
{
public:
	 void Push(){m_bIsOn = 1;};
	 void Release(){m_bIsOn = 0;};

} CMomentaryButton;

typedef class CEditBox
{
#define MAX_EDIT_LEN 1024
public:
     C2DGraphics*   m_Graph;
     BitmapStruct*  m_pbitOn;
     BitmapStruct*  m_pbitOff;
     BitmapStruct   m_bitBack;
     COLORREF       m_TextColor;
     HFONT          m_hFont;
     DWORD          m_dwLastBlinkTime;
     int            m_iX;
     int            m_iY;
     int            m_iW;
     int            m_iH;
     bool           m_bCursorEnabled;
     bool           m_bCursorOn;

     TCHAR          m_str[MAX_EDIT_LEN];
     int            m_iNextChar;

     CEditBox() : m_iNextChar(0), m_bCursorOn(true), m_bCursorEnabled(true){m_str[0] = 0;};

     //Set back color to -1 for transparent
     HRESULT Create(C2DGraphics *pGraph, int x, int y, int Width, int Height,
                       COLORREF TextColor = 0, COLORREF BackColor = RGB(255,255,255), HFONT fon = 0)
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

          m_dwLastBlinkTime = GetTickCount();

          return S_OK;

     }

     HRESULT Draw()
     {
          if (!m_Graph || m_iW <= 0)
               return E_FAIL;

		  if (m_hFont)
			  m_Graph->SetFont(m_hFont);

          //Move start pointer so total width plus cursor will fit
          if (m_iNextChar < MAX_EDIT_LEN - 1 && m_bCursorEnabled)
          {
               m_str[m_iNextChar] = '_';
               m_str[m_iNextChar + 1] = 0;
          }
          int w, h, p = 0;
          do
          {
               m_Graph->GetStringPixelSize(&m_str[p++], &w, &h);
          } while (w > m_iW && m_str[p] != 0 && p < MAX_EDIT_LEN);
          p--;
          m_str[m_iNextChar] = 0;

          m_Graph->DrawBitmapIntoRect(&m_bitBack, m_iX, m_iY, m_iW, m_iH, 1.0);
          if (m_hFont)
               m_Graph->SetFont(m_hFont);

          m_Graph->SetTextColor(m_TextColor);
          m_Graph->DrawTxt(m_iX, m_iY, &m_str[p]);

          if (m_bCursorEnabled && m_bCursorOn)
          {
               m_Graph->GetStringPixelSize(&m_str[p], &w, &h);
               m_Graph->DrawTxt(m_iX + w, m_iY - 1, _T("_"));
          }

          return S_OK;
     }

     //return true if cursor has changed (i.e. needs to be redrawn)
     bool Update()
     {
          if (m_bCursorEnabled && (GetTickCount() - m_dwLastBlinkTime) >= 500)
          {
               m_bCursorOn ^= 1;
               m_dwLastBlinkTime = GetTickCount();
               return true;
          }
          return false;
     }

     void EnableCursor(bool bEnabled)
     {
          m_bCursorEnabled = bEnabled;
          m_bCursorOn = 1;
          m_dwLastBlinkTime = GetTickCount();

          return;
     }

     //return true if accepted (i.e. needs to be redrawn)
     bool CharIn(TCHAR Char)
     {
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
               EnableCursor(false);
          }
          else
          {
               if (m_iNextChar > MAX_EDIT_LEN - 2)
                    return false;
               if (Char == '0')
                    Char = (TCHAR)'Ø';
               m_str[m_iNextChar++] = Char;
               m_str[m_iNextChar] = (TCHAR)0;
          }
          //reset cursor blinks
          m_dwLastBlinkTime = GetTickCount();
          m_bCursorOn= 1;
          return true;
     }

     void PasteText(TCHAR *pStr)
     {
          if (!pStr)
               return;
          int L = _tcslen(pStr);
          for (int i=0; i < L && m_iNextChar < MAX_EDIT_LEN - 1; i++)
          {
               m_str[m_iNextChar++] = pStr[i];
          }
          m_str[m_iNextChar] = 0;
     }

     void GetText(TCHAR **pStr)
     {
          *pStr = m_str;
     }

     void ClearText()
     {
          m_iNextChar = 0;
          m_str[0] = 0;
     }


} CEditBox;

typedef class CListBox
{
#define LB_MAX			64							  //Max elements (arbitrary)
#define LB_THICKNESS     2                            //line thickness

public:

	CComBSTR				m_cbsListText[LB_MAX];    //Actual text
	int						m_iNumInList;			  //Total number in the list
	int						m_iSelText;               //Current selected     

	C2DGraphics*			m_pGraph;
	BitmapStruct			m_bitButton;			 //The down-button (unlit)
	//BitmapStruct			m_bitButtonLit;
	BitmapStruct			m_bitSel[LB_MAX];		 //Bitmaps for each text line if selected
	BitmapStruct			m_bitNotSel[LB_MAX];     //Bitmaps if not selected, list open (text against background)
	BitmapStruct			m_bitTitle[LB_MAX];      //Bitmap in title line (may be transparent back...)
	int						m_iX;
	int						m_iY;
	int						m_iWidthPix;
	int						m_iLineHeight;
	int						m_iButtonX;
	HFONT					m_hFont;

	long					m_bMainBackTransparent; //if false, top line is RegBack
	COLORREF				m_colRegText;			//non-selected color
	COLORREF				m_colRegBack;
	COLORREF				m_colSelText;			//selected color
	COLORREF				m_colSelBack;
	COLORREF				m_colBorder;			//border line and button

	long					m_bButtonDown;          //True if button down
	long					m_bListOpen;            //True if list is open



	CListBox() : m_iNumInList(0), m_iSelText(0), m_hFont(NULL), m_pGraph(NULL),
				   m_bButtonDown(false), m_bListOpen(false)
	{};

	~CListBox()
	{
		ClearList();
	}
	
	int GetNumInList()
	{
		return m_iNumInList;
	}

	BSTR GetSelText()
	{
		if (m_iNumInList == 0)
			return NULL;

		return (SysAllocString(m_cbsListText[m_iSelText]));
	}

	bool SetSelectedText(BSTR String)
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
	bool Create(int X, int Y, int Width, HFONT hFont, bool bMainBackTransparent,
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
	void ClearList()
	{
		for (int i=0; i < m_iNumInList; i++)
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
	bool AddToList(BSTR Text)
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

	void Close()
	{
		m_bListOpen = false;
		return;
	}

	void Draw()
	{
		if (!m_pGraph)
			return;

		//Draw selected text
		if (m_iNumInList > 0)
		{
			m_pGraph->DrawBitmapIntoRect(&m_bitTitle[m_iSelText], m_iX, m_iY, m_bitTitle[m_iSelText].WidthPix,
				m_bitTitle[m_iSelText].HeightPix, 1.0 );
		}

		//Draw main border
		m_pGraph->SetLineColor(m_colBorder);
		m_pGraph->DrawLine(m_iX, m_iY, m_iX + m_iWidthPix - 1, m_iY, LB_THICKNESS);
		m_pGraph->LineTo(m_iX + m_iWidthPix - 1, m_iY + m_iLineHeight);
		m_pGraph->LineTo(m_iX, m_iY + m_iLineHeight);
		m_pGraph->LineTo(m_iX, m_iY);
		m_pGraph->DrawLine(m_iX + m_iButtonX - 1, m_iY, m_iX + m_iButtonX - 1, m_iY + m_iLineHeight, LB_THICKNESS);

		//Draw button
		m_pGraph->DrawBitmapIntoRect(&m_bitButton, m_iX + m_iButtonX + 2, m_iY +6, m_iWidthPix - m_iButtonX - 4, 
			m_iLineHeight - 8, 1.0f);

		//Draw list if open
		if (m_bListOpen)
		{
			//Draw Each line
			int Y = m_iY + m_iLineHeight;
			for (int i=0; i < m_iNumInList; i++)
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
	int WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam)
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
	int GetListLineForXY(int X, int Y)
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

} CListBox;

//////////////////////////////////////


