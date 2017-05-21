#pragma once

//DEBUG
#include <d3d9.h>
#include <d3d10_1.h>



#include <d3d11.h>
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
	int GetFont(HFONT *phFont);
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

	int			   ButtonID;         

	C2DGraphics*   m_pGraph;
	BitmapStruct*  m_pbitOn;
	BitmapStruct*  m_pbitOff;
	int            m_iX;
	int            m_iY;
	int            m_iW;
	int            m_iH;
	bool           m_bIsOn;
	bool		   m_bOnlyPushOn;
	bool		   m_bIsVisible;

	CTwoStateButton() : m_bIsOn(false), m_bOnlyPushOn(false), m_pbitOn(NULL), m_pbitOff(NULL), m_pGraph(NULL),
		m_bIsVisible(true){};

	int Create(C2DGraphics *pGraph, int x, int y, int Width, int Height, BitmapStruct *pbitOn, BitmapStruct *pbitOff, bool bOnlyPushOn = false);
	int WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam); 
	int Draw();
	int SetPosition(int x, int y);
	int SetOn(bool bOn); 
	int SetVisible(bool bVisible);
	bool IsOn();
	bool IsWithin(int x, int y);
	int Shutdown();

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
#define MAX_EDIT_LEN 1024         //Maximum length of editbox in characters, arbitrary

public:
     C2DGraphics*   m_Graph;
     BitmapStruct*  m_pbitOn;
     BitmapStruct*  m_pbitOff;
     BitmapStruct   m_bitBack;
     COLORREF       m_TextColor;
     HFONT          m_hFont;
     ULONGLONG      m_LastBlinkTime;
     int            m_iX;
     int            m_iY;
     int            m_iW;          //Total width of box: if m_iLines > 1, this is total pixel width of all lines added together
	                               //  but it's drawn according to m_iRealW;
	 int            m_iRealW;      //Real width of one line. if m_iNumLines == 1 this is same as m_iW
     int            m_iH;
     bool           m_bCursorEnabled;
     bool           m_bCursorOn;
	 bool			m_bMasked;     //true to show everything as *'s
	 bool			m_bHidden;     //true to not draw or accept input
	 bool			m_bEditsLocked; //true to draw but not accept editing

     TCHAR          m_str[MAX_EDIT_LEN];
	 TCHAR			m_MaskedStr[MAX_EDIT_LEN];
	 TCHAR			m_Cursor[2];
     int            m_iNextChar;
	 int			m_iMaxChar; 
	 int			m_iNumLines;     //1..n

     CEditBox() : m_iNextChar(0), m_bCursorOn(true), m_bCursorEnabled(false), m_bHidden(false), m_iMaxChar(MAX_EDIT_LEN),
		m_pbitOn(NULL), m_pbitOff(NULL), m_bEditsLocked(false){m_str[0] = 0;};

	 HRESULT Create(C2DGraphics *pGraph, int x, int y, int Width, int Height, COLORREF TextColor = 0,
		 COLORREF BackColor = RGB(255, 255, 255), HFONT fon = 0, bool bMasked = false);
	 void    Shutdown();
	 HRESULT Draw();
	 bool	 Update();
	 void    EnableCursor(bool bEnabled);
	 bool	 CharIn(TCHAR Char);
	 void    SetText(TCHAR *pStr);
	 void    AppendText(TCHAR *pStr);
	 TCHAR*  GetText();
	 int	 GetTextLength();
	 void    ClearText();
	 bool    IsWithin(int X, int Y);
	 void    SetHidden(bool bHidden);
	 void    SetMaxChars(int iMaxChars);
	 void    DisableEdits(bool bDisabled);

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
				   m_bButtonDown(false), m_bListOpen(false){};
	~CListBox() { ClearList(); };
	

	bool Create(int X, int Y, int Width, HFONT hFont, bool bMainBackTransparent,
		COLORREF RegTextColor, COLORREF RegBackColor, COLORREF SelTextColor,
		COLORREF SelBackColor, COLORREF BorderColor, C2DGraphics *pGraph,
		TCHAR *ButtonBitmapFilename);
	int  GetNumInList();
	BSTR GetSelText();
	bool SetSelectedText(BSTR String);
	void ClearList();
	bool AddToList(BSTR Text);
	void Close();
	void Draw();
	int  WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam);
	int  GetListLineForXY(int X, int Y);

} CListBox;

//////////////////////////////////////


