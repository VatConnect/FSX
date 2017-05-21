#include "stdafx.h"
#include "Dialogs.h"

CSettingsDlg::CSettingsDlg() : m_bOpen(false)
{
}

CSettingsDlg::~CSettingsDlg()
{
}

int CSettingsDlg::Initialize(CFSXGUI *pGUI, CMainDlg *pMainDlg, C2DGraphics *pGraph, int WidthPix, int HeightPix)
{


	return 1;
}

/////////////
//Base class

int CSettingsDlg::WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam)
{

	return -1;
}

int CSettingsDlg::Update()
{

	return 0;
}

int CSettingsDlg::Draw(IDirect3DDevice9* pDevice)
{

	return 0;
}

int CSettingsDlg::Open()
{

	return 0;
}

int CSettingsDlg::Close()
{

	return 0;
}

int CSettingsDlg::Shutdown()
{

	return 0;
}