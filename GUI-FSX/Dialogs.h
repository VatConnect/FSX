#pragma once

#include "stdafx.h"
#include "C2DGraphics.h"
#include "CTime.h"

using namespace std;

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

	//Draw onto given device, or if pDevice NULL then current bitmap output
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

	int Initialize(CFSXGUI *pGUI, CMainDlg *pMainDlg, C2DGraphics *pGraph, int WidthPix, int HeightPix);

protected:
	bool	m_bOpen; 

} CLoginDlg;

//AKA "TEXT" window
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

	int Initialize(CFSXGUI *pGUI, CMainDlg *pMainDlg, C2DGraphics *pGraph, int WidthPix, int HeightPix);

protected:
	bool	m_bOpen;

} CChatDlg;	

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

	//Buttons on our outer frame
	CMomentaryButton m_butConnect;
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
	CChatDlg		m_dlgText;
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








