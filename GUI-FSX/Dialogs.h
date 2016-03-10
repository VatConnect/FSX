#pragma once

#include "stdafx.h"
#include "C2DGraphics.h"

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
	int Initialize(CFSXGUI *pGUI, C2DGraphics *pGraph, HWND hFSXWin, bool bInWindowedMode);

	int DrawWholeDialogToDC();

protected:

	CFSXGUI *m_pGUI;
	C2DGraphics *m_pGraph;
	HFONT m_hFont;
	HWND m_hFSXWin;                    //Primary FSX window
	IDirect3DDevice9* m_pFullscreenDevice;  //Primary device when in fullscreen
	bool  m_bInWindowedMode;          //true if windowed, false if fullscreen

	BitmapStruct m_bitDialogBack;
	BitmapStruct m_bitCurrentOutput;   //Latest "drawn" image including all children

	RECT	m_rectFSXWin;              //FSX Window's screen position and size in desktop units 
									   //(0,0 not necessarily top-left of draw area)

	int		m_iScreenX;                //Current X & Y of dialog, within the drawing/client area of window or fullscreen device (0,0 top left)
	int		m_iScreenY;
	int		m_iWidthPix;
	int		m_iHeightPix;
	int		m_iCursorScreenX;          //Current cursor X&Y within client area of window or fullscreen device (0,0 top left)
	int		m_iCursorScreenY;			
	bool	m_bDraggingDialog;         //True if currently dragging the dialog 
	int		m_iLastDragScreenX;        //if dragging, this was last mouse position
	int		m_iLastDragScreenY;

	RECT m_rectChildWindowPos;         //position of child dialogs' "screen"
	vector<CTwoStateButton *> m_apButtons;  
	vector<CDialog *> m_apChildDialogs; 

	//Controls on our outer frame
	CMomentaryButton m_butConnect;
	CMomentaryButton m_butDisconnect;
	CMomentaryButton m_butATC;
	CMomentaryButton m_butText;
	CMomentaryButton m_butFlightPlan;
	CMomentaryButton m_butWeather;
	CMomentaryButton m_butSettings;
	CMomentaryButton m_butMinimize;
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

} CMainDlg;








