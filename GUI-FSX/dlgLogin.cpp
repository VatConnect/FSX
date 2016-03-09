#include "stdafx.h"
#include "Dialogs.h"

CLoginDlg::CLoginDlg() : m_bOpen(false)
{
}

CLoginDlg::~CLoginDlg()
{
}

int CLoginDlg::Initialize(CFSXGUI *pGUI, CMainDlg *pMainDlg, C2DGraphics *pGraph, int WidthPix, int HeightPix)
{


	return 1;
}

/////////////
//Base class

int CLoginDlg::WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam)
{

	return -1;
}

int CLoginDlg::Update()
{

	return 0;
}

int CLoginDlg::Draw(IDirect3DDevice9* pDevice)
{

	return 0;
}

int CLoginDlg::Open()
{

	return 0;
}

int CLoginDlg::Close()
{

	return 0;
}

int CLoginDlg::Shutdown()
{

	return 0;
}