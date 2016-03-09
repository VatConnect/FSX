#include "stdafx.h"
#include "Dialogs.h"

CATCDlg::CATCDlg() : m_bOpen(false)
{
}

CATCDlg::~CATCDlg()
{
}

int CATCDlg::Initialize(CFSXGUI *pGUI, CMainDlg *pMainDlg, C2DGraphics *pGraph, int WidthPix, int HeightPix)
{


	return 1;
}

/////////////
//Base class

int CATCDlg::WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam)
{

	return -1;
}

int CATCDlg::Update()
{

	return 0;
}

int CATCDlg::Draw(IDirect3DDevice9* pDevice)
{

	return 0;
}

int CATCDlg::Open()
{

	return 0;
}

int CATCDlg::Close()
{

	return 0;
}

int CATCDlg::Shutdown()
{

	return 0;
}

