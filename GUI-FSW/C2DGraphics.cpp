#include "stdafx.h"
#include "C2DGraphics.h"

//All methods return 1 if succeeded, 0 if failed

C2DGraphics::C2DGraphics() : m_hMasterDC(nullptr), m_pOutputBitmap(nullptr), m_hFont(nullptr), m_hPen(nullptr), m_iPenThickness(0), m_pMasterDevice(nullptr)
{
	m_LineColor = RGB(255,0,0);
	m_TextColor = RGB(255,255,255);
}

C2DGraphics::~C2DGraphics()
{
}

//Initialize with (one of) the Direct3D devices. Must be called before any of the other methods.
int C2DGraphics::Initialize(IDirect3DDevice9 *pDevice)
{
	//Create a dummy surface to create a compatible master display context -- this is likely to be the same among all devices
	m_pMasterDevice = pDevice;
	IDirect3DSurface9* pSurface;
	if (FAILED(pDevice->CreateOffscreenPlainSurface(4, 4, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT,&pSurface, nullptr)))
		return 0;
	HDC hSurfDC;
	if (FAILED(pSurface->GetDC(&hSurfDC)))
	{
		pSurface->Release();
		return 0;
	}
	m_hMasterDC = CreateCompatibleDC(hSurfDC);

	//Default bitmap is monochrome, so select in a color one
	HBITMAP MasterBitmap = CreateCompatibleBitmap (hSurfDC, 4, 4 );
	SelectObject(m_hMasterDC, MasterBitmap);

	pSurface->ReleaseDC(hSurfDC);
	pSurface->Release();
	
	return 1;
}

//Load a bitmap from disk. (To reload, first delete the old one then call this). The
//Direct3D surface will be invalid until the first time the bitmap is drawn, when
//the loaded bitmap gets copied into a newly created surface. This is so bitmaps can
//be preloaded before Direct3D graphics are initialized. 
int C2DGraphics::LoadBitmapFromFile(const WCHAR *Filename, BitmapStruct *pBitmap)
{
	if (!pBitmap || pBitmap->hDC != nullptr || !m_hMasterDC)
		return 0;

	//Load it
	HBITMAP hBitmap = (HBITMAP)LoadImage(nullptr, Filename,IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
	if (hBitmap == nullptr)
		return 0;
	
	//Get its size
	BITMAP BM;
	if (!GetObject(hBitmap, sizeof(BITMAP), &BM))
	{
		DeleteObject(hBitmap);
		return 0;
	}
	pBitmap->WidthPix = BM.bmWidth;
	pBitmap->HeightPix = BM.bmHeight;
	
	//Create a device context for it and select the bitmap into it
	pBitmap->hDC = CreateCompatibleDC(m_hMasterDC);
	SelectObject(pBitmap->hDC, hBitmap);
	SetMapMode(pBitmap->hDC, MM_TEXT);

	return 1;
}

//Create a new bitmap of given size, given pointer to new bitmap to fill in
int C2DGraphics::MakeNewBitmap(int Width, int Height, BitmapStruct *pBitmap)
{
	if (pBitmap->hDC || !m_hMasterDC || pBitmap->pSurface)
		return 0;
	pBitmap->bHasTransparency = false;
	HBITMAP NewBitmap = CreateCompatibleBitmap (m_hMasterDC, Width, Height );
   	pBitmap->hDC = CreateCompatibleDC (m_hMasterDC);
	if (!NewBitmap || !pBitmap->hDC)
	{
		if (NewBitmap)
			DeleteObject(NewBitmap);
		if (pBitmap->hDC)
			DeleteObject(pBitmap->hDC);
		return 0;
	}
	SelectObject (pBitmap->hDC, NewBitmap );
	SetMapMode(pBitmap->hDC, MM_TEXT);
	pBitmap->WidthPix = Width;
	pBitmap->HeightPix = Height;
	pBitmap->pDevice = nullptr;
	pBitmap->pSurface = nullptr;

	return 1;
}

//Set the bitmap's transparency to its top-left (0,0) color. Note this
//only works for bitmaps drawn onto other bitmaps. The one drawn to the
//Direct3D surface won't show transparencies or alpha.
int C2DGraphics::EnableBitmapTransparency(BitmapStruct *pBitmap)
{
	pBitmap->bHasTransparency = true;
	pBitmap->TransparentColor = GetPixel(pBitmap->hDC, 0, 0);
	return 1;
}

//Draw bitmap to the current output bitmap's hDC, optional Alpha blend 0.0-1.0 defaults to 1.0 (none)
int C2DGraphics::DrawBitmapToOutputBitmap(BitmapStruct *pBitmap, long X, long Y, float Alpha)
{
	if (!pBitmap->hDC || !m_pOutputBitmap || !m_pOutputBitmap->hDC)
		return 0;

	if (Alpha > 0.99f && !pBitmap->bHasTransparency)
		BitBlt(m_pOutputBitmap->hDC, X, Y, pBitmap->WidthPix, pBitmap->HeightPix, pBitmap->hDC, 0, 0, SRCCOPY);
	else
		DrawBitmapIntoRect(pBitmap, X, Y, pBitmap->WidthPix, pBitmap->HeightPix, Alpha);

	return 1;
}

//Given the bitmap with already-filled surface, and device to draw it on, do so. If surface created for different
//device, reload the surface. Used to re-draw unchanged surface onto back buffer
int C2DGraphics::DrawBitmapSurfaceOnDevice(BitmapStruct *pBitmap, IDirect3DDevice9 *pDevice, long ScreenX, long ScreenY)
{
	//Remake surface if it was originally made for different device (or first time drawing)
	if (pBitmap->pDevice != pDevice)
	{
		if (pBitmap->pSurface)
		{
			pBitmap->pSurface->Release();
			pBitmap->pSurface = nullptr;
		}
		pBitmap->pDevice = pDevice;
	}
	if (!pBitmap->pSurface)
		CopyBitmapDCToSurface(pBitmap);

	//Draw the surface to the backbuffer of the device
	IDirect3DSurface9 *pBackBuffer = nullptr;
	if (FAILED(pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer)))
		return 0;

	RECT r;
	r.left = ScreenX;
	r.top = ScreenY;
	r.right = ScreenX + pBitmap->WidthPix;
	r.bottom = ScreenY + pBitmap->HeightPix;
	pDevice->StretchRect(pBitmap->pSurface, nullptr, pBackBuffer, &r, D3DTEXF_NONE);
	pBackBuffer->Release();
	return 1;
}

//Copy the bitmap in the hDC of the given bitmap onto its corresponding direct3D surface. This is typically done after
//the hDC bitmap is updated.
int C2DGraphics::CopyBitmapDCToSurface(BitmapStruct *pBitmap)
{
	if (!pBitmap->pDevice)
		pBitmap->pDevice = m_pMasterDevice;

	//See if we need to create a new surface on that device 
	if (!pBitmap->pSurface)
	{
		if (FAILED(pBitmap->pDevice->CreateOffscreenPlainSurface(pBitmap->WidthPix, pBitmap->HeightPix, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT,
			&pBitmap->pSurface, nullptr)))
		{
			assert(0);
			return 0;
		}
	}

	//Copy the hDC bitmap to the surface
	HDC hDC;
	if (FAILED(pBitmap->pSurface->GetDC(&hDC)))
		return 0;
	BitBlt(hDC, 0, 0, pBitmap->WidthPix, pBitmap->HeightPix, pBitmap->hDC, 0, 0, SRCCOPY);
	pBitmap->pSurface->ReleaseDC(hDC);

	return 1;
}


int C2DGraphics::DeleteBM(BitmapStruct *pBitmap)
{
	if (pBitmap->hDC)
	{
		HGDIOBJ hObj = GetCurrentObject(pBitmap->hDC, OBJ_BITMAP);
		if (hObj)
			DeleteObject(hObj);
		DeleteDC(pBitmap->hDC);
		pBitmap->hDC = nullptr;
	}			
	if (pBitmap->pDevice)
		pBitmap->pDevice = nullptr;        //we don't own this so didn't addref it
	if (pBitmap->pSurface)
		pBitmap->pSurface->Release();
	pBitmap->pSurface = nullptr;

	return 1;
}

///////////////////////////////////
//Set the text font and size to the given font and characteristics. If okay, returns 1.
//If it's not found, returns 2 and sets an existing font that "best matches" the characteristics.
//If that fails too, returns 0
int C2DGraphics::FindBestFont(WCHAR *FontName, int PointSize,  int bBold, int bItalic, int bUnderline, HFONT *phFont)
{
	//Fill in font characteristics
	LOGFONT Info;
	Info.lfHeight = -(MulDiv(PointSize, GetDeviceCaps(m_hMasterDC, LOGPIXELSY), 72)); 
	Info.lfWidth = 0;
	Info.lfEscapement = 0;
	Info.lfOrientation = 0;
	Info.lfWeight = bBold? 700 : 200;
	Info.lfItalic = (BYTE)bItalic;
	Info.lfUnderline = (BYTE)bUnderline;
	Info.lfStrikeOut = false;
	Info.lfCharSet = DEFAULT_CHARSET; 
	Info.lfOutPrecision = OUT_DEFAULT_PRECIS; //OUT_TT_ONLY_PRECIS;
	Info.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	Info.lfQuality = ANTIALIASED_QUALITY; 
	Info.lfPitchAndFamily = (UCHAR)(DEFAULT_PITCH | FF_DONTCARE);
	wcscpy_s(Info.lfFaceName, 32, FontName);
	
	//Try requested font
	HFONT hFont = CreateFontIndirect(&Info);
	*phFont = hFont;
	if (hFont)
		return 1;

	//Not found, try any meeting the requirements (sans-seriff)
	Info.lfPitchAndFamily = (UCHAR)(DEFAULT_PITCH | FF_MODERN | FF_SWISS);
	Info.lfFaceName[0] = 0;
	hFont = CreateFontIndirect(&Info);
	*phFont = hFont;
	if (!hFont)
		return 0;
	return 2;
}

int C2DGraphics::Shutdown()
{
	if (m_hFont)
	{
		DeleteObject(m_hFont);
		m_hFont = nullptr;
	}
	if (m_hMasterDC)
	{
		DeleteDC(m_hMasterDC);
		m_hMasterDC = nullptr;
	}
	if (m_hPen)
	{
		DeleteObject(m_hPen);
		m_hPen = nullptr;
	}

	return 1;
}


//////////////////////
//Methods to modify bitmap set with SetOutputBitmap()

int C2DGraphics::SetOutputBitmap(BitmapStruct *pBitmap)
{
	m_pOutputBitmap = pBitmap;
	return 1;
}

int C2DGraphics::SetTextColor(COLORREF Color)
{
	m_TextColor = Color;
	return 1;
}

int C2DGraphics::SetFont(HFONT hFont)
{
	m_hFont = hFont;
	return 1;
}

int C2DGraphics::GetFont(HFONT *pFont)
{
	*pFont = m_hFont;
	return 1;
}

//Draw given text in font created by SetFont and SetTextColor, onto
//current output bitmap
int C2DGraphics::DrawTxt(int x, int y, WCHAR *Str)
{
	HDC dc = m_pOutputBitmap->hDC;

	::SetTextColor(dc, m_TextColor);
	SetBkMode(dc, TRANSPARENT);
	SelectObject(dc, m_hFont);
	TextOut(dc, x, y, Str, (int)wcslen(Str));

	return 1;
}

int C2DGraphics::FillBitmapWithColor(COLORREF Color)
{
	if (!m_pOutputBitmap->hDC)
		return 0;
	
	HBRUSH Brush = CreateSolidBrush(Color);
	RECT rect;
	rect.top = 0;
	rect.left = 0;
	rect.bottom = m_pOutputBitmap->HeightPix;
	rect.right = m_pOutputBitmap->WidthPix;
	int res = FillRect(m_pOutputBitmap->hDC, &rect, Brush);
	DeleteObject(Brush);
	return res;
}

int C2DGraphics::GetStringPixelSize(WCHAR *Str, int *Width, int *Height)
{
	
	int len = (int)wcslen(Str);
	SIZE sz;
	HFONT hOldFont = (HFONT)SelectObject(m_hMasterDC, m_hFont);
	if (GetTextExtentPoint32(m_hMasterDC, Str, len, &sz))
	{
		*Width = sz.cx;
		*Height = sz.cy;
	}
	else
	{
		*Width = 0;
		*Height = 0;
	}

	SelectObject(m_hMasterDC, hOldFont);

	return 1;
}

//Draw given source bitmap into current output bitmap, stretching/compressing as needed to fit into 
//the given location. Alpha blending between 0.0 (fully transparent) and 1.0 (fully opaque), though
//it only applies to bitmaps drawn into other bitmaps. 
int C2DGraphics::DrawBitmapIntoRect(BitmapStruct *pBitmap, int x, int y,int Width, int Height, float Alpha)
{
	if (m_pOutputBitmap->hDC == 0)
		return 0;
	//Get source information
    BITMAP  BitmapInfo;
    HGDIOBJ hSourceBitmap = GetCurrentObject(pBitmap->hDC, OBJ_BITMAP);
	if (!hSourceBitmap)
		return 0;
	GetObject(hSourceBitmap,sizeof(BITMAP),(LPSTR)&BitmapInfo);

	HDC hdcSourceDC = pBitmap->hDC;
	int SrcWidth = BitmapInfo.bmWidth;
	int SrcHeight = BitmapInfo.bmHeight;

	SetStretchBltMode(m_pOutputBitmap->hDC, COLORONCOLOR);
	BOOL bRes;
	if (Alpha < 0.999f)
	{
		BitmapStruct TempBitmap;
		TempBitmap.hDC = nullptr;

		if (pBitmap->bHasTransparency)
		{
			//Make copy of surface beneath
			MakeNewBitmap(Width, Height, &TempBitmap);
			if (TempBitmap.hDC != 0)
			{
				bRes = BitBlt(TempBitmap.hDC, 0, 0, Width, Height, pBitmap->hDC, x, y, SRCCOPY);

				SetStretchBltMode(TempBitmap.hDC, COLORONCOLOR);

				//Transparent blit to the copy and set the result as the new source -- surface alpha blended with itself will be unchanged.
				if (TransparentBlt(TempBitmap.hDC, 0, 0, Width, Height, pBitmap->hDC, 0, 0, BitmapInfo.bmWidth,
					BitmapInfo.bmHeight, pBitmap->TransparentColor))
				{
					hdcSourceDC = TempBitmap.hDC;
					SrcWidth = Width;
					SrcHeight = Height;
				}
			}
		}
		
		BLENDFUNCTION bf;
		bf.BlendOp = AC_SRC_OVER;
		bf.BlendFlags = 0;
		bf.SourceConstantAlpha = (BYTE)(Alpha * 256.0f);
		bf.AlphaFormat = 0;
			
		bRes = AlphaBlend(m_pOutputBitmap->hDC, x, y, Width, Height, hdcSourceDC, 0, 0, SrcWidth, SrcHeight, bf);
	
		if (TempBitmap.hDC)
			DeleteBitmap(&TempBitmap);
	}
	else if (pBitmap->bHasTransparency)
	{
		bRes = TransparentBlt(m_pOutputBitmap->hDC, x, y, Width, Height, hdcSourceDC, 0, 0, SrcWidth, 
							  SrcHeight, pBitmap->TransparentColor);
	}
	else
	{
		bRes = StretchBlt(m_pOutputBitmap->hDC, x, y, Width, Height, hdcSourceDC, 0, 0, SrcWidth, SrcHeight, SRCCOPY);
	}
	if (bRes)
		return 1;
	return 0;
}

int C2DGraphics::SetLineColor(COLORREF LineColor)
{
	if (m_hPen && LineColor != m_LineColor)
	{
		DeleteObject(m_hPen);
		m_hPen = nullptr;
	}
	m_LineColor = LineColor;
	return S_OK;
}

//Draw line to output bitmap in given thickness, and color as set by SetLineColor.
int C2DGraphics::DrawLine(int fx, int fy, int tx, int ty, int Thickness)
{
	//Use cached pen if it's same thickness
	if (m_hPen && m_iPenThickness != Thickness)
	{
		DeleteObject(m_hPen);
		m_hPen = nullptr;
	}

	if (!m_hPen)
	{
		m_hPen = CreatePen(PS_SOLID, Thickness, m_LineColor);
		m_iPenThickness = Thickness;
	}

	SelectObject(m_pOutputBitmap->hDC, m_hPen);

	MoveToEx(m_pOutputBitmap->hDC, fx, fy, nullptr);
	::LineTo(m_pOutputBitmap->hDC, tx, ty);

	return 1;
}

//Continue line from last DrawLine() using same pen
int C2DGraphics::LineTo(int tx, int ty)
{
	::LineTo(m_pOutputBitmap->hDC, tx, ty);
	return 1;
}
