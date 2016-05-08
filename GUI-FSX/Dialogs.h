#pragma once

#include "stdafx.h"
#include "C2DGraphics.h"
#include "CPacketSender.h"
#include "CPacketReceiver.h"
#include "CTime.h"

//Constants common to all dialogs
#define FONT_SIZE 8            //DONT GO BELOW 8 
#define TEXT_WIDTH_CHAR 32     //dialog-screen within the dialog. Whole dialog is this max width plus 2 left and 2 right char width borders
#define TEXT_HEIGHT_CHAR 10   
#define OUTLINE_THICKNESS 1

#define COL_BUTTON_OFF RGB(0,64,64)
#define COL_BUTTON_ON RGB(0,128,0)
#define COL_BUTTON_TEXT RGB(220, 220, 220)
#define COL_DLG_BACK RGB(5,5,50)
#define COL_SCREEN_OUTLINE RGB(200, 200, 200)
#define COL_GREEN_STATUS RGB(0, 128, 0)
#define COL_RED_STATUS RGB(200, 0, 0)
#define COL_DLG_TEXT RGB(220, 220, 220)         //dialog fields
#define COL_USER_TEXT RGB(45,233, 45)           //user-entered fields
#define COL_EDITBOX_BACK RGB(15,15,150)
#define COL_TEXTBOX_SCROLLBAR RGB(128,128,128)
#define ALT_KEY_PRESSED (1<<29)                //WM_CHAR flags for alt key and key previously down
#define PREV_PRESSED (1<<30) 

using namespace std;

//DEBUG CFSXGUI -- remove when initialize implemented
class CFSXGUI;

class CMainDlg;

//Base class for dialogs...
typedef class CDialog
{
public:

	//Returns WINMSG_RESULT (see C2DGraphics.h) 
	virtual int WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam) = 0;

	//Regular update
	virtual int Update() = 0;

	//Draw to current given device, or if pDevice NULL then current bitmap output
	virtual int Draw(IDirect3DDevice9* pDevice) = 0;

	//Open self
	virtual int Open() = 0;

	//Close self
	virtual int Close() = 0;

	//Program shutting down (close and clean up)
	virtual int Shutdown() = 0;

	virtual int SwitchToWindowed(HWND hWnd) = 0;

	virtual int SwitchToFullscreen(IDirect3DDevice9* pFullscreenDevice, int WidthPix, int HeightPix) = 0;

} CDialog;


typedef class CLoginDlg : public CDialog
{
public:
	CLoginDlg();
	~CLoginDlg();

	//Base class
	int WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam);
	int Update();
	int Draw(IDirect3DDevice9* pDevice);
	int Open();
	int Close();
	int Shutdown();
	int SwitchToWindowed(HWND hWnd) { return 1; };
	int SwitchToFullscreen(IDirect3DDevice9* pFullscreenDevice, int WidthPix, int HeightPix) { return 1; };

	//Custom
	int Initialize(CMainDlg *pMainDlg, HWND hWnd, C2DGraphics *pGraph, int X, int Y, int WidthPix, int HeightPix);
	int DrawWholeDialogToOutput();
	int SetFocusToEditbox(CEditBox *pEdit);
	int RemoveFocusFromEditbox(CEditBox *pEdit);

protected:
	bool	m_bOpen;   //true if open
	int m_iX;          //location and size within main dialog 
	int m_iY;
	int m_iWidthPix;
	int m_iHeightPix;
	int m_iCoverX;
	int m_iCoverY;

	HFONT m_hFieldnameFont;           //field names
	HFONT m_hDataFont;                //edit box
	HWND  m_hWnd;                     //our (parent) hWnd for grabbing keyboard focus
	CMainDlg *m_pMainDlg;               //parent (frame) dialog for callbacks

	CEditBox *m_pEditWithFocus;       //which edit box has focus (keyboard capture), NULL if none

	BitmapStruct m_bitBack;           //Background with all but field contents
	BitmapStruct m_bitCurrent;        //Latest full dialog (background plus contents)
	BitmapStruct m_bitButSelected;    //checkbox on
	BitmapStruct m_bitButNotSelected; //checkbox off
	BitmapStruct m_bitCover;          //Cover to "erase" callsign and a/c type text in observer mode

	CEditBox m_editServer;
	CEditBox m_editName;
	CEditBox m_editID;
	CEditBox m_editPassword;
	CEditBox m_editCallsign;
	CEditBox m_editACType;
	vector<CEditBox *> m_apEditBoxes; 

	CTwoStateButton m_butPilot;       //checkbox
	CTwoStateButton m_butObserver;    //checkbox
	CMomentaryButton m_butConnect;
	CMomentaryButton m_butServerSelect;

	C2DGraphics   *m_pGraph;

} CLoginDlg;

#define MAX_TEXT_DLG_LINES 50       //Maximum text lines held in the buffer (arbitrary, just a scrollbar usability and memory thing)
#define MAX_TEXT_DLG_COLUMNS 42     //41 characters/line plus terminating zero (arbitrary but matches with 8 pt font)
typedef class CTextDlg : public CDialog
{
public:
	CTextDlg();
	~CTextDlg();

	//Base class
	int WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam);
	int Update();
    int Draw(IDirect3DDevice9* pDevice);
	int Open();
	int Close();
	int Shutdown();
	int SwitchToWindowed(HWND hWnd) { return 1; };
	int SwitchToFullscreen(IDirect3DDevice9* pFullscreenDevice, int WidthPix, int HeightPix) { return 1; };

	//Custom
	int Initialize(CMainDlg *pMainDlg, HWND hFSXWin, C2DGraphics *pGraph, int X, int Y, int WidthPix, int HeightPix);
	int AddText(WCHAR *pText, COLORREF col = COL_DLG_TEXT);
	int ClearAll();

protected:

	WCHAR m_wcsLineBuffer[MAX_TEXT_DLG_LINES][MAX_TEXT_DLG_COLUMNS];   //text buffer (circular) note includes terminating 0
	COLORREF m_colLineColors[MAX_TEXT_DLG_LINES]; //text color of corresponding line in m_wcsLineBuffer
	int m_iFirstLine;        //Index of first line in the circular m_wcsLineBuffer
	int m_iNextLine;         //Index of next open line in m_wcsLineBuffer;
	int m_iTopScreenLine;    //Index of line at the top of the output screen
	bool m_bScrollLocked;    //True if locked on m_iTopScreenLine
	bool	m_bOpen;
	C2DGraphics *m_pGraph;
	int m_iX;
	int m_iY;
	int m_iWidthPix;
	int m_iHeightPix;
	HWND m_hWnd;
	CMainDlg *m_pMainDlg;
	HFONT m_hFont;

	int m_iTextWidthChar;    //width of printable characters
	int m_iTextHeightChar;   //number of text output lines (not including edit box)
	int m_iLineHeightPix;    //height of each text line in pixels

	BitmapStruct m_bitBackground; //blank background
	BitmapStruct m_bitScrollbar;  
	BitmapStruct m_bitOutput;  //Current output

	int m_iScrollWidthPix;   //size of scrollbar bitmap
	int m_iScrollHeightPix;
	int m_iScrollSpaceTopY;      //Highest Y position at top
	int m_iScrollSpaceHeightPix; //Size of available vertical space 
	int m_iScrollX;          //current scrollbar X&Y position (X never changes...)
	int m_iScrollY;
	bool m_bDraggingScrollbar;  

	CMomentaryButton m_butScrollUp;
	CMomentaryButton m_butScrollDown;
	CEditBox m_editTextIn;
	bool m_bEditHasFocus;

	//Internal
	int AddLine(WCHAR *pText, COLORREF col);
	int SetScrollbarToScreenline();
	int SetScreenlineToScrollbar();
	int UpdateOutputBitmap();
	int SetEditboxFocus(bool bHasFocus);
	int DrawEditboxOnly();

} CTextDlg;	

typedef class CSettingsDlg : public CDialog
{
public:
	CSettingsDlg();
	~CSettingsDlg();

	//Base class
	int WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam);
	int Update();
    int Draw(IDirect3DDevice9* pDevice);
	int Open();
	int Close();
	int Shutdown();
	int SwitchToWindowed(HWND hWnd) { return 1; };
	int SwitchToFullscreen(IDirect3DDevice9* pFullscreenDevice, int WidthPix, int HeightPix) { return 1; };

	int Initialize(CFSXGUI *pGUI, CMainDlg *pMainDlg, C2DGraphics *pGraph, int WidthPix, int HeightPix);

protected:
	bool	m_bOpen;

} CSettingsDlg;

typedef class CFlightPlanDlg : public CDialog
{
public:
	CFlightPlanDlg();
	~CFlightPlanDlg();

	//Base class
	int WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam);
	int Update();
    int Draw(IDirect3DDevice9* pDevice);
	int Open();
	int Close();
	int Shutdown();
	int SwitchToWindowed(HWND hWnd) { return 1; };
	int SwitchToFullscreen(IDirect3DDevice9* pFullscreenDevice, int WidthPix, int HeightPix) { return 1; };

	int Initialize(CFSXGUI *pGUI, CMainDlg *pMainDlg, C2DGraphics *pGraph, int WidthPix, int HeightPix);

protected:
	bool	m_bOpen;

} CFlightPlanDlg;

typedef class CATCDlg : public CDialog
{
public:
	CATCDlg();
	~CATCDlg();

	//Base class

	int WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam);
	int Update();
	int Draw(IDirect3DDevice9* pDevice);
	int Open();
	int Close();
	int Shutdown();
	int SwitchToWindowed(HWND hWnd) { return 1; };
	int SwitchToFullscreen(IDirect3DDevice9* pFullscreenDevice, int WidthPix, int HeightPix) { return 1; };

	int Initialize(CFSXGUI *pGUI, CMainDlg *pMainDlg, C2DGraphics *pGraph, int WidthPix, int HeightPix);

protected:
	bool	m_bOpen;

} CATCDlg;

typedef class CWXDlg : public CDialog
{
public:
	CWXDlg();
	~CWXDlg();

	//Base class
	int WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam);
	int Update();
	int Draw(IDirect3DDevice9* pDevice);
	int Open();
	int Close();
	int Shutdown();
	int SwitchToWindowed(HWND hWnd) { return 1; };
	int SwitchToFullscreen(IDirect3DDevice9* pFullscreenDevice, int WidthPix, int HeightPix) { return 1; };

	int Initialize(CFSXGUI *pGUI, CMainDlg *pMainDlg, C2DGraphics *pGraph, int WidthPix, int HeightPix);

protected:
	bool	m_bOpen;

} CWXDlg;

typedef class CMainDlg : public CDialog
{
public:

	CMainDlg();
	~CMainDlg();

	//Base class
	int WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam);
	int Update();
	int Draw(IDirect3DDevice9* pDevice); 
	int Open();
	int Close();
	int Shutdown();

	int SwitchToWindowed(HWND hWnd);
	int SwitchToFullscreen(IDirect3DDevice9* pFullscreenDevice, int Width, int Height);
	int SwitchToMinimized();
	int SwitchToNormal();
	int Initialize(CFSXGUI *pGUI, C2DGraphics *pGraph, HWND hFSXWin, bool bInWindowedMode);
	int DrawWholeDialogToDC();

	//Callbacks from child dialogs
	int OnChildInitiatedRedraw();             //Called if child dialog redrew on its own (versus through win message)
	int GetKeyboardInput(bool bNeedKeyboard); //true to trap keyboard input and forward WM_CHAR messages

	//Child dialog callbacks from user actions
	WINMSG_RESULT OnLoginConnectPressed();   //Connect button from login dialog pressed
	WINMSG_RESULT OnServerSelectPressed();   //Select server in login dialog pressed
	int OnSendText(WCHAR *pStr);             //user wants to send/xmit this string (owned by caller)

protected:

	//Id's for main dialog's buttons
	typedef enum BUTTONID
	{
		BUT_NONE,
		BUT_CONNECT,
		BUT_DISCONNECT,
		BUT_TEXT,
		BUT_FP,
		BUT_ATC,
		BUT_WX,
		BUT_SETTINGS,
		BUT_MIN,
		BUT_MAX,
		BUT_CLOSE
	} BUTTONID;

	//Status of connection
	typedef enum STATUS
	{
		STAT_RED,         //aka not connected
		STAT_GREEN,       //connected
		STAT_BLINKING     //connected with text activity happening
	} STATUS;

	CFSXGUI *m_pGUI;
	C2DGraphics *m_pGraph;
	HFONT m_hFont;
	HWND m_hFSXWin;                    //Primary FSX window
	IDirect3DDevice9* m_pFullscreenDevice;  //Primary device when in fullscreen
	bool  m_bInWindowedMode;          //true if windowed, false if fullscreen
	bool  m_bMinimized;               //true if tiny version, false if regular size
	STATUS m_Status;                  //current connection status

	BitmapStruct m_bitDialogBack;      //Full sized dialog background
	BitmapStruct m_bitMinimizedBack;   //Minimized dialog background
	BitmapStruct m_bitFullOutput;      //Latest fullsized dialog with background plus buttons
	BitmapStruct m_bitMinimizedOutput; //Latest minimized dialog with status light and maximize button
	BitmapStruct m_bitRedCircle;       //Red status "light" in minimized mode
	BitmapStruct m_bitGreenCircle;     //Green status "light" in minimized mode

	RECT	m_rectFSXWin;              //FSX Window's screen position and size in desktop units 
									   //(0,0 not necessarily top-left of draw area)

	int		m_iScreenX;                //Current X & Y of dialog (or minimized dialog), within the drawing/client area of window or fullscreen device (0,0 top left)
	int		m_iScreenY;
	int		m_iWidthPix;               //Current width and height of dialog (either regular, or minimized version)
	int		m_iHeightPix;
	
	int     m_iMaximizedWidthPix;      //Width and Height regular/full-size dialog
	int     m_iMaximizedHeightPix;
	int     m_iMaximizedScreenX;       //Saved position
	int		m_iMaximizedScreenY;

	int		m_iMinimizedWidthPix;      //Width and Height of minimized dialog
	int     m_iMinimizedHeightPix; 
	int	    m_iMinimizedScreenX;
	int		m_iMinimizedScreenY;

	int		m_iCursorScreenX;          //Current cursor X&Y within client area of window or fullscreen device (0,0 top left)
	int		m_iCursorScreenY;			
	bool	m_bDraggingDialog;         //True if currently dragging the dialog 
	bool    m_bHaveMouseCapture;       //True if we've captured mouse
	int		m_iLastDragScreenX;        //if dragging, this was last mouse position
	int		m_iLastDragScreenY;
	bool    m_bBlinkOn;                //Alternates true and false over time 
	double  m_dNextBlinkSwitchTime;    //next Timer time to toggle m_bBlinkOn;
	CTime   m_Timer;

	RECT m_rectChildWindowPos;         //position of child dialogs' "screen"
	vector<CTwoStateButton *> m_apButtons;  
	vector<CDialog *> m_apChildDialogs; 
	BUTTONID		 m_CurButtonLit;           //ID of current button selected/lit/dialog in dialog window (BUT_NONE if none)
	CTwoStateButton* m_pCurButtonLit;          //Current button lit
	CDialog*		 m_pCurDialogOpen;         //Current child dialog displaying 
	

	//Buttons on our outer frame
	CMomentaryButton m_butConnect;             //Connect and disconnect are in same spot but one set to visible and other not
	CMomentaryButton m_butDisconnect;   
	CMomentaryButton m_butATC;
	CMomentaryButton m_butText;
	CMomentaryButton m_butFlightPlan;
	CMomentaryButton m_butWeather;
	CMomentaryButton m_butSettings;
	CMomentaryButton m_butMinimize;
	CMomentaryButton m_butMaximize;
	CMomentaryButton m_butClose;

	//Child dialogs that go on our "screen"
	CLoginDlg		m_dlgLogin;
	CTextDlg		m_dlgText;
	CATCDlg         m_dlgATC;
	CWXDlg		    m_dlgWX;
	CSettingsDlg	m_dlgSettings;
	CFlightPlanDlg	m_dlgFlightPlan;

	int CreateFrame();
	int CreateScreenDialogs();
	int MakeButtonBitmaps(int W, int H, WCHAR *pText, BitmapStruct **ppOn, BitmapStruct **ppOff);
	bool ClampDialogToScreen();
	WINMSG_RESULT ProcessButtonClick(int ButtonID);
	WINMSG_RESULT DeselectButton(int ButtonID);        

} CMainDlg;








