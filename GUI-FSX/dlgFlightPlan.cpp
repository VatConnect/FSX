#include "stdafx.h"
#include "Dialogs.h"

CFlightPlanDlg::CFlightPlanDlg() : m_bOpen(false)
{
}

CFlightPlanDlg::~CFlightPlanDlg()
{
}

int CFlightPlanDlg::Initialize(CFSXGUI *pGUI, CMainDlg *pMainDlg, C2DGraphics *pGraph, int WidthPix, int HeightPix)
{


	return 1;
}

/////////////
//Base class

int CFlightPlanDlg::WindowsMessage(UINT message, WPARAM wParam, LPARAM lParam)
{

	return -1;
}

int CFlightPlanDlg::Update()
{

	return 0;
}

int CFlightPlanDlg::Draw(IDirect3DDevice9* pDevice)
{

	return 0;
}

int CFlightPlanDlg::Open()
{

	return 0;
}

int CFlightPlanDlg::Close()
{

	return 0;
}

int CFlightPlanDlg::Shutdown()
{

	return 0;
}