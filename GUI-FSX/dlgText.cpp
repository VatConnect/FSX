#include "stdafx.h"
#include "Dialogs.h"

CTextDlg::CTextDlg() : m_bOpen(false)
{
}

CTextDlg::~CTextDlg()
{
}

int CTextDlg::Initialize(CFSXGUI *pGUI, CMainDlg *pMainDlg, C2DGraphics *pGraph, int WidthPix, int HeightPix)
{


	return 1;
}

/////////////
//Base class

int CTextDlg::WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam)
{

	return -1;
}

int CTextDlg::Update()
{

	return 0;
}

int CTextDlg::Draw(IDirect3DDevice9* pDevice)
{

	return 0;
}

int CTextDlg::Open()
{

	return 0;
}

int CTextDlg::Close()
{

	return 0;
}

int CTextDlg::Shutdown()
{

	return 0;
}