#include "stdafx.h"
#include "Dialogs.h"

CWXDlg::CWXDlg() : m_bOpen(false)
{
}

CWXDlg::~CWXDlg()
{
}

int CWXDlg::Initialize(CFSXGUI *pGUI, CMainDlg *pMainDlg, C2DGraphics *pGraph, int WidthPix, int HeightPix)
{


	return 1;
}

/////////////
//Base class

int CWXDlg::WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam)
{

	return -1;
}

int CWXDlg::Update()
{

	return 0;
}

int CWXDlg::Draw(IDirect3DDevice9* pDevice)
{

	return 0;
}

int CWXDlg::Open()
{

	return 0;
}

int CWXDlg::Close()
{

	return 0;
}

int CWXDlg::Shutdown()
{

	return 0;
}