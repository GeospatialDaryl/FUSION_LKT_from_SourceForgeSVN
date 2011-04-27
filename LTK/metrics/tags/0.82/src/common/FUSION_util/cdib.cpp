// cdib.cpp
// new version for WIN32
#include "stdafx.h"
#include "cdib.h"

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_SERIAL(CDib, CObject, 0);

CDib::CDib()
{
	m_hFile = NULL;
	m_hBitmap = NULL;
	m_hPalette = NULL;
	m_nBmihAlloc = m_nImageAlloc = noAlloc;
	Empty();
}

CDib::CDib(CSize size, int nBitCount)
{
	m_hFile = NULL;
	m_hBitmap = NULL;
	m_hPalette = NULL;
	m_nBmihAlloc = m_nImageAlloc = noAlloc;
	Empty();
	ComputePaletteSize(nBitCount);
	m_lpBMIH = (LPBITMAPINFOHEADER) new 
		char[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * m_nColorTableEntries];
	m_nBmihAlloc = crtAlloc;
	m_lpBMIH->biSize = sizeof(BITMAPINFOHEADER);
	m_lpBMIH->biWidth = size.cx;
	m_lpBMIH->biHeight = size.cy;
	m_lpBMIH->biPlanes = 1;
	m_lpBMIH->biBitCount = (unsigned short) nBitCount;
	m_lpBMIH->biCompression = BI_RGB;
	m_lpBMIH->biSizeImage = 0;
	m_lpBMIH->biXPelsPerMeter = 11811;		// 300 dpi
	m_lpBMIH->biYPelsPerMeter = 11811;		// 300 dpi
	m_lpBMIH->biClrUsed = m_nColorTableEntries;
	m_lpBMIH->biClrImportant = m_nColorTableEntries;
	ComputeMetrics();
	memset(m_lpvColorTable, 0, sizeof(RGBQUAD) * m_nColorTableEntries);
	m_lpImage = NULL;  // no data yet
}

CDib::~CDib()
{
	Empty();
}

CSize CDib::GetDimensions()
{	
	if (m_lpBMIH == NULL) 
		return(CSize(0, 0));

	return(CSize((int) m_lpBMIH->biWidth, (int) m_lpBMIH->biHeight));
}

void CDib::GetDimensions(CSize& sz)
{	
	if (m_lpBMIH == NULL) {
		sz.cx = 0;
		sz.cy = 0;
	}
	else {
		sz.cx = (int) m_lpBMIH->biWidth;
		sz.cy = (int) m_lpBMIH->biHeight;
	}
}

BOOL CDib::AttachMapFile(const char* strPathname, BOOL bShare) // for reading
{
	// if we open the same file twice, Windows treats it as 2 separate files
	// doesn't work with rare BMP files where # palette entries > biClrUsed
	HANDLE hFile = ::CreateFile(strPathname, GENERIC_WRITE | GENERIC_READ,
		bShare ? FILE_SHARE_READ : 0,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	ASSERT(hFile != INVALID_HANDLE_VALUE);
//	DWORD dwFileSize = ::GetFileSize(hFile, NULL);
	HANDLE hMap = ::CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
//	DWORD dwErr = ::GetLastError();
	if(hMap == NULL) {
		AfxMessageBox("Empty bitmap file");
		return FALSE;
	}
	LPVOID lpvFile = ::MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, 0); // map whole file
	ASSERT(lpvFile != NULL);
	if(((LPBITMAPFILEHEADER) lpvFile)->bfType != 0x4d42) {
		AfxMessageBox("Invalid bitmap file");
		DetachMapFile();
		return FALSE;
	}
	AttachMemory((LPBYTE) lpvFile + sizeof(BITMAPFILEHEADER));
	m_lpvFile = lpvFile;
	m_hFile = hFile;
	m_hMap = hMap;
	return TRUE;
}

BOOL CDib::CopyToMapFile(const char* strPathname)
{
	// copies DIB to a new file, releases prior pointers
	// if you previously used CreateSection, the HBITMAP will be NULL (and unusable)
	BITMAPFILEHEADER bmfh;
	bmfh.bfType = 0x4d42;  // 'BM'
	bmfh.bfSize = m_dwSizeImage + sizeof(BITMAPINFOHEADER) +
			sizeof(RGBQUAD) * m_nColorTableEntries + sizeof(BITMAPFILEHEADER);

	// meaning of bfSize open to interpretation
	bmfh.bfReserved1 = bmfh.bfReserved2 = 0;
	bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +
			sizeof(RGBQUAD) * m_nColorTableEntries;	
	HANDLE hFile = ::CreateFile(strPathname, GENERIC_WRITE | GENERIC_READ, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	ASSERT(hFile != INVALID_HANDLE_VALUE);
	int nSize =  sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +
				sizeof(RGBQUAD) * m_nColorTableEntries +  m_dwSizeImage;
	HANDLE hMap = ::CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, nSize, NULL);
//	DWORD dwErr = ::GetLastError();
	ASSERT(hMap != NULL);
	LPVOID lpvFile = ::MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, 0); // map whole file
	ASSERT(lpvFile != NULL);
	LPBYTE lpbCurrent = (LPBYTE) lpvFile;
	memcpy(lpbCurrent, &bmfh, sizeof(BITMAPFILEHEADER)); // file header
	lpbCurrent += sizeof(BITMAPFILEHEADER);
	LPBITMAPINFOHEADER lpBMIH = (LPBITMAPINFOHEADER) lpbCurrent;
	memcpy(lpbCurrent, m_lpBMIH,
		sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * m_nColorTableEntries); // info
	lpbCurrent += sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * m_nColorTableEntries;
	memcpy(lpbCurrent, m_lpImage, m_dwSizeImage); // bit image
	DWORD dwSizeImage = m_dwSizeImage;
	Empty();
	m_dwSizeImage = dwSizeImage;
	m_nBmihAlloc = m_nImageAlloc = noAlloc;
	m_lpBMIH = lpBMIH;
	m_lpImage = lpbCurrent;
	m_hFile = hFile;
	m_hMap = hMap;
	m_lpvFile = lpvFile;
	ComputePaletteSize(m_lpBMIH->biBitCount);
	ComputeMetrics();
	MakePalette();
	return TRUE;
}

BOOL CDib::AttachMemory(LPVOID lpvMem, BOOL bMustDelete, HGLOBAL hGlobal)
{
	// assumes contiguous BITMAPINFOHEADER, color table, image
	// color table could be zero length
	Empty();
	m_hGlobal = hGlobal;
	if (bMustDelete == FALSE) {
		m_nBmihAlloc = noAlloc;
	}
	else {
		m_nBmihAlloc = ((hGlobal == NULL) ? crtAlloc : heapAlloc);
	}
	try {
		m_lpBMIH = (LPBITMAPINFOHEADER) lpvMem;
		ComputeMetrics();
		ComputePaletteSize(m_lpBMIH->biBitCount);
		m_lpImage = (LPBYTE) m_lpvColorTable + sizeof(RGBQUAD) * m_nColorTableEntries;
		MakePalette();
	}
	catch(CException* pe) {
		AfxMessageBox("AttachMemory error");
		pe->Delete();
		return FALSE;
	}
	return TRUE;
}

UINT CDib::UsePalette(CDC* pDC, BOOL bBackground /* = FALSE */)
{
	if (m_hPalette == NULL) 
		return 0;

	HDC hdc = pDC->GetSafeHdc();
	::SelectPalette(hdc, m_hPalette, bBackground);
	return ::RealizePalette(hdc);
}

BOOL CDib::Draw(CDC* pDC, CPoint origin, CSize size, int BltMode)
{
	if (m_lpBMIH == NULL) 
		return FALSE;

	if (m_hPalette != NULL) {
		::SelectPalette(pDC->GetSafeHdc(), m_hPalette, TRUE);
		::RealizePalette(pDC->GetSafeHdc());
	}
	pDC->SetStretchBltMode(BltMode);
	if (::StretchDIBits(pDC->GetSafeHdc(), origin.x, origin.y, size.cx, size.cy,
		0, 0, m_lpBMIH->biWidth, m_lpBMIH->biHeight,
		m_lpImage, (LPBITMAPINFO) m_lpBMIH, DIB_RGB_COLORS, SRCCOPY) != GDI_ERROR)
		return TRUE;
	return(FALSE);
}

BOOL CDib::DrawPart(CDC* pDC, CPoint origin, CSize size, CPoint iorigin, CSize isize, int BltMode)
{
	if (m_lpBMIH == NULL) 
		return FALSE;

	if (m_hPalette != NULL) {
		::SelectPalette(pDC->GetSafeHdc(), m_hPalette, TRUE);
		::RealizePalette(pDC->GetSafeHdc());
	}
	pDC->SetStretchBltMode(BltMode);
	if (isize.cx + isize.cy < 512) {
		// switch to halftone when images are small...better quality, less pixelation
		SetBrushOrgEx(pDC->GetSafeHdc(), 0, 0, NULL);
	}
	if (::StretchDIBits(pDC->GetSafeHdc(), origin.x, origin.y, size.cx, size.cy,
		iorigin.x, iorigin.y, isize.cx, isize.cy,
		m_lpImage, (LPBITMAPINFO) m_lpBMIH, DIB_RGB_COLORS, SRCCOPY) != GDI_ERROR)
		return TRUE;
	return(FALSE);
}

HBITMAP CDib::CreateSection(CDC* pDC /* = NULL */)
{
	if (m_lpBMIH == NULL)
		return NULL;

	if (m_lpImage != NULL) // can only do this if image doesn't exist
		return NULL; 

	m_hBitmap = ::CreateDIBSection(pDC->GetSafeHdc(), (LPBITMAPINFO) m_lpBMIH,
									DIB_RGB_COLORS,	(LPVOID*) &m_lpImage, NULL, 0);
	ASSERT(m_lpImage != NULL);
	return m_hBitmap;
}

BOOL CDib::MakePalette()
{
	// makes a logical palette (m_hPalette) from the DIB's color table
	// this palette will be selected and realized prior to drawing the DIB
	if (m_nColorTableEntries == 0)
		return FALSE;

	if (m_hPalette != NULL)
		::DeleteObject(m_hPalette);

//	TRACE("CDib::MakePalette -- m_nColorTableEntries = %d\n", m_nColorTableEntries);
	
	LPLOGPALETTE pLogPal = (LPLOGPALETTE) new char[2 * sizeof(WORD) +
							m_nColorTableEntries * sizeof(PALETTEENTRY)];
	pLogPal->palVersion = 0x300;
	pLogPal->palNumEntries = (unsigned short) m_nColorTableEntries;
	LPRGBQUAD pDibQuad = (LPRGBQUAD) m_lpvColorTable;
	for(int i = 0; i < m_nColorTableEntries; i++) {
		pLogPal->palPalEntry[i].peRed = pDibQuad->rgbRed;
		pLogPal->palPalEntry[i].peGreen = pDibQuad->rgbGreen;
		pLogPal->palPalEntry[i].peBlue = pDibQuad->rgbBlue;
		pLogPal->palPalEntry[i].peFlags = 0;
		pDibQuad++;
	}
	m_hPalette = ::CreatePalette(pLogPal);
	delete [] pLogPal;
	return TRUE;
}	

BOOL CDib::SetSystemPalette(CDC* pDC)
{
	// if the DIB doesn't have a color table, we can use the system's halftone palette
	if (m_nColorTableEntries != 0) 
		return FALSE;

	m_hPalette = ::CreateHalftonePalette(pDC->GetSafeHdc());
	return TRUE;
}

HBITMAP CDib::CreateBitmap(CDC* pDC)
{
    if (m_dwSizeImage == 0) 
		return NULL;

    HBITMAP hBitmap = ::CreateDIBitmap(pDC->GetSafeHdc(), m_lpBMIH,
            CBM_INIT, m_lpImage, (LPBITMAPINFO) m_lpBMIH, DIB_RGB_COLORS);
    ASSERT(hBitmap != NULL);
    return hBitmap;
}

BOOL CDib::Compress(CDC* pDC, BOOL bCompress /* = TRUE */)
{
	// 1. makes GDI bitmap from existing DIB
	// 2. makes a new DIB from GDI bitmap with compression
	// 3. cleans up the original DIB
	// 4. puts the new DIB in the object
	if ((m_lpBMIH->biBitCount != 4) && (m_lpBMIH->biBitCount != 8)) // compression supported only for 4 bpp and 8 bpp DIBs
		return FALSE;
		
	if (m_hBitmap) // can't compress a DIB Section!
		return FALSE; 

//	TRACE("Compress: original palette size = %d\n", m_nColorTableEntries); 
	HDC hdc = pDC->GetSafeHdc();
	HPALETTE hOldPalette = ::SelectPalette(hdc, m_hPalette, FALSE);
	HBITMAP hBitmap;  // temporary
	if ((hBitmap = CreateBitmap(pDC)) == NULL) 
		return FALSE;

	int nSize = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * m_nColorTableEntries;
	LPBITMAPINFOHEADER lpBMIH = (LPBITMAPINFOHEADER) new char[nSize];
	memcpy(lpBMIH, m_lpBMIH, nSize);  // new header
	if (bCompress) {
		switch (lpBMIH->biBitCount) {
		case 4:
			lpBMIH->biCompression = BI_RLE4;
			break;
		case 8:
			lpBMIH->biCompression = BI_RLE8;
			break;
		default:
			ASSERT(FALSE);
		}
		// calls GetDIBits with null data pointer to get size of compressed DIB
		if (!::GetDIBits(pDC->GetSafeHdc(), hBitmap, 0, (UINT) lpBMIH->biHeight,
						NULL, (LPBITMAPINFO) lpBMIH, DIB_RGB_COLORS)) {
			AfxMessageBox("Unable to compress this DIB");
			// probably a problem with the color table
	 		::DeleteObject(hBitmap);
			delete [] lpBMIH;
			::SelectPalette(hdc, hOldPalette, FALSE);
			return FALSE; 
		}
		if (lpBMIH->biSizeImage == 0) {
			AfxMessageBox("Driver can't do compression");
	 		::DeleteObject(hBitmap);
			delete [] lpBMIH;
			::SelectPalette(hdc, hOldPalette, FALSE);
			return FALSE; 
		}
		else {
			m_dwSizeImage = lpBMIH->biSizeImage;
		}
	}
	else {
		lpBMIH->biCompression = BI_RGB; // decompress
		// figure the image size from the bitmap width and height
		DWORD dwBytes = ((DWORD) lpBMIH->biWidth * lpBMIH->biBitCount) / 32;
		if(((DWORD) lpBMIH->biWidth * lpBMIH->biBitCount) % 32) {
			dwBytes++;
		}
		dwBytes *= 4;
		m_dwSizeImage = dwBytes * lpBMIH->biHeight; // no compression
		lpBMIH->biSizeImage = m_dwSizeImage;
	} 
	// second GetDIBits call to make DIB
	LPBYTE lpImage = (LPBYTE) new char[m_dwSizeImage];
	VERIFY(::GetDIBits(pDC->GetSafeHdc(), hBitmap, 0, (UINT) lpBMIH->biHeight,
    		lpImage, (LPBITMAPINFO) lpBMIH, DIB_RGB_COLORS));
//    TRACE("dib successfully created - height = %d\n", lpBMIH->biHeight);
	::DeleteObject(hBitmap);
	Empty();
	m_nBmihAlloc = m_nImageAlloc = crtAlloc;
	m_lpBMIH = lpBMIH;
	m_lpImage = lpImage;
	ComputeMetrics();
	ComputePaletteSize(m_lpBMIH->biBitCount);
	MakePalette();
	::SelectPalette(hdc, hOldPalette, FALSE);
//	TRACE("Compress: new palette size = %d\n", m_nColorTableEntries); 
	return TRUE;
}

BOOL CDib::ReadHeader(CFile* pFile)
{
	// 1. read file header to get size of info hdr + color table
	// 2. read info hdr (to get image size) and color table
	// can't use bfSize in file header
	Empty();
	int nCount, nSize;
	BITMAPFILEHEADER bmfh;
	try {
		nCount = pFile->Read((LPVOID) &bmfh, sizeof(BITMAPFILEHEADER));
		if (nCount != sizeof(BITMAPFILEHEADER)) {
			throw new CException;
		}
		if (bmfh.bfType != 0x4d42) {
			throw new CException;
		}
		nSize = bmfh.bfOffBits - sizeof(BITMAPFILEHEADER);
		m_lpBMIH = (LPBITMAPINFOHEADER) new char[nSize];
//		m_nBmihAlloc = m_nImageAlloc = crtAlloc;		// removed m_nImageAlloc 9/7/06
		m_nBmihAlloc = crtAlloc;
		nCount = pFile->Read(m_lpBMIH, nSize); // info hdr & color table
		ComputeMetrics();
		ComputePaletteSize(m_lpBMIH->biBitCount);
		m_lpImage = NULL;
	}
	catch (CException* pe) {
		AfxMessageBox("Read error while reading header");
		pe->Delete();
		return FALSE;
	}
	return TRUE;
}

BOOL CDib::Read(CFile* pFile)
{
	// 1. read file header to get size of info hdr + color table
	// 2. read info hdr (to get image size) and color table
	// 3. read image
	// can't use bfSize in file header
	Empty();
	int nCount, nSize;
	BITMAPFILEHEADER bmfh;
	try {
		nCount = pFile->Read((LPVOID) &bmfh, sizeof(BITMAPFILEHEADER));
		if (nCount != sizeof(BITMAPFILEHEADER)) {
			throw new CException;
		}
		if (bmfh.bfType != 0x4d42) {
			throw new CException;
		}
		nSize = bmfh.bfOffBits - sizeof(BITMAPFILEHEADER);
		m_lpBMIH = (LPBITMAPINFOHEADER) new char[nSize];
		m_nBmihAlloc = m_nImageAlloc = crtAlloc;
		nCount = pFile->Read(m_lpBMIH, nSize); // info hdr & color table
		ComputeMetrics();
		ComputePaletteSize(m_lpBMIH->biBitCount);
		MakePalette();
		m_lpImage = (LPBYTE) new char[m_dwSizeImage];
		nCount = pFile->Read(m_lpImage, m_dwSizeImage); // image only
	}
	catch (CException* pe) {
		AfxMessageBox("Read error");
		pe->Delete();
		return FALSE;
	}
	return TRUE;
}

BOOL CDib::ReadSection(CFile* pFile, CDC* pDC /* = NULL */)
{
	// new function reads BMP from disk and creates a DIB section
	//    allows modification of bitmaps from disk
	// 1. read file header to get size of info hdr + color table
	// 2. read info hdr (to get image size) and color table
	// 3. create DIB section based on header parms
	// 4. read image into memory that CreateDibSection allocates
	Empty();
	int nCount, nSize;
	BITMAPFILEHEADER bmfh;
	try {
		nCount = pFile->Read((LPVOID) &bmfh, sizeof(BITMAPFILEHEADER));
		if (nCount != sizeof(BITMAPFILEHEADER)) {
			throw new CException;
		}
		if (bmfh.bfType != 0x4d42) {
			throw new CException;
		}
		nSize = bmfh.bfOffBits - sizeof(BITMAPFILEHEADER);
		m_lpBMIH = (LPBITMAPINFOHEADER) new char[nSize];
		m_nBmihAlloc = crtAlloc;
		m_nImageAlloc = noAlloc;
		nCount = pFile->Read(m_lpBMIH, nSize); // info hdr & color table
		if (m_lpBMIH->biCompression != BI_RGB) {
			throw new CException;
		}
		ComputeMetrics();
		ComputePaletteSize(m_lpBMIH->biBitCount);
		MakePalette();
		UsePalette(pDC);
		m_hBitmap = ::CreateDIBSection(pDC->GetSafeHdc(), (LPBITMAPINFO) m_lpBMIH,
			DIB_RGB_COLORS,	(LPVOID*) &m_lpImage, NULL, 0);
		ASSERT(m_lpImage != NULL);
		nCount = pFile->Read(m_lpImage, m_dwSizeImage); // image only
	}
	catch(CException* pe) {
		AfxMessageBox("ReadSection error");
		pe->Delete();
		return FALSE;
	}
	return TRUE;
}

BOOL CDib::Write(CFile* pFile)
{
	BITMAPFILEHEADER bmfh;
	bmfh.bfType = 0x4d42;  // 'BM'
	int nSizeHdr = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * m_nColorTableEntries;
	bmfh.bfSize = 0;
//	bmfh.bfSize = sizeof(BITMAPFILEHEADER) + nSizeHdr + m_dwSizeImage;
	// meaning of bfSize open to interpretation (bytes, words, dwords?) -- we won't use it
	bmfh.bfReserved1 = bmfh.bfReserved2 = 0;
	bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +
			sizeof(RGBQUAD) * m_nColorTableEntries;	
	try {
		pFile->Write((LPVOID) &bmfh, sizeof(BITMAPFILEHEADER));
		pFile->Write((LPVOID) m_lpBMIH,  nSizeHdr);
		pFile->Write((LPVOID) m_lpImage, m_dwSizeImage);
	}
	catch (CException* pe) {
		pe->Delete();
//		AfxMessageBox("write error");
		return FALSE;
	}
	return TRUE;
}

void CDib::Serialize(CArchive& ar)
{
	DWORD dwPos;
	dwPos = ar.GetFile()->GetPosition();
//	TRACE("CDib::Serialize -- pos = %d\n", dwPos);
	ar.Flush();
	dwPos = ar.GetFile()->GetPosition();
//	TRACE("CDib::Serialize -- pos = %d\n", dwPos);
	if (ar.IsStoring()) {
		Write(ar.GetFile());
	}
	else {
		Read(ar.GetFile());
	}
}

// helper functions
void CDib::ComputePaletteSize(int nBitCount)
{
	if ((m_lpBMIH == NULL) || (m_lpBMIH->biClrUsed == 0)) {
		switch(nBitCount) {
			case 1:
				m_nColorTableEntries = 2;
				break;
			case 4:
				m_nColorTableEntries = 16;
				break;
			case 8:
				m_nColorTableEntries = 256;
				break;
			case 16:
			case 24:
			case 32:
				m_nColorTableEntries = 0;
				break;
			default:
				ASSERT(FALSE);
		}
	}
	else {
		m_nColorTableEntries = m_lpBMIH->biClrUsed;
	}
	ASSERT((m_nColorTableEntries >= 0) && (m_nColorTableEntries <= 256)); 
}

void CDib::ComputeMetrics()
{
	if (m_lpBMIH->biSize != sizeof(BITMAPINFOHEADER)) {
//		TRACE("Not a valid Windows bitmap -- probably an OS/2 bitmap\n");
		throw new CException;
	}
	m_dwSizeImage = m_lpBMIH->biSizeImage;

	m_dwBytesPerScanLine = ((DWORD) m_lpBMIH->biWidth * m_lpBMIH->biBitCount) / 32;
	if (((DWORD) m_lpBMIH->biWidth * m_lpBMIH->biBitCount) % 32) {
		m_dwBytesPerScanLine++;
	}
	m_dwBytesPerScanLine *= 4;

	if (m_dwSizeImage == 0) {
		m_dwSizeImage = m_dwBytesPerScanLine * m_lpBMIH->biHeight; // no compression
	}
	m_lpvColorTable = (LPBYTE) m_lpBMIH + sizeof(BITMAPINFOHEADER);
}

void CDib::Empty()
{
	// this is supposed to clean up whatever is in the DIB
	DetachMapFile();
	if (m_nBmihAlloc == crtAlloc) {
		delete [] m_lpBMIH;
	}
	else if (m_nBmihAlloc == heapAlloc) {
		::GlobalUnlock(m_hGlobal);
		::GlobalFree(m_hGlobal);
	}
	if (m_nImageAlloc == crtAlloc) 
		delete [] m_lpImage;
	if (m_hPalette != NULL) 
		::DeleteObject(m_hPalette);
	if (m_hBitmap != NULL) 
		::DeleteObject(m_hBitmap);

	m_nBmihAlloc = m_nImageAlloc = noAlloc;
	m_hGlobal = NULL;
	m_lpBMIH = NULL;
	m_lpImage = NULL;
	m_lpvColorTable = NULL;
	m_nColorTableEntries = 0;
	m_dwSizeImage = 0;
	m_lpvFile = NULL;
	m_hMap = NULL;
	m_hFile = NULL;
	m_hBitmap = NULL;
	m_hPalette = NULL;
}

void CDib::DetachMapFile()
{
	if (m_hFile == NULL) 
		return;

	::UnmapViewOfFile(m_lpvFile);
	::CloseHandle(m_hMap);
	::CloseHandle(m_hFile);
	m_hFile = NULL;
}

int CDib::GetColorBits(LPCTSTR szFileName)
{
	// 1. read file header to get size of info hdr + color table
	// 2. read info hdr to get image color depth
	Empty();
	int nCount, nSize;
	int ColorBits = 0;
	BITMAPFILEHEADER bmfh;
	LPBITMAPINFOHEADER lpBMIH = NULL;
	CFile pFile;

	// try to open BMP file
	if (pFile.Open(szFileName, CFile::modeRead)) {
		try {
			nCount = pFile.Read((LPVOID) &bmfh, sizeof(BITMAPFILEHEADER));
			if (nCount != sizeof(BITMAPFILEHEADER)) {
				throw new CException;
			}
			if (bmfh.bfType != 0x4d42) {
				throw new CException;
			}
			nSize = bmfh.bfOffBits - sizeof(BITMAPFILEHEADER);
			lpBMIH = (LPBITMAPINFOHEADER) new char[nSize];
			nCount = pFile.Read(lpBMIH, nSize); // info hdr & color table
			ColorBits = lpBMIH->biBitCount;
			delete [] lpBMIH;
		}
		catch (CException* pe) {
			if (lpBMIH)
				delete [] lpBMIH;
			AfxMessageBox("Image file header read error");
			pe->Delete();
			return(0);
		}
		return(ColorBits);
	}
	return(0);
}

BOOL CDib::CheckFileFormat(LPCTSTR szFileName)
{
	int nCount;
	BITMAPFILEHEADER bmfh;
	CFile pFile;

	// try to open BMP file
	if (pFile.Open(szFileName, CFile::modeRead)) {
		try {
			nCount = pFile.Read((LPVOID) &bmfh, sizeof(BITMAPFILEHEADER));
			if (nCount != sizeof(BITMAPFILEHEADER)) {
				throw new CException;
			}
			if (bmfh.bfType != 0x4d42) {
				throw new CException;
			}
		}
		catch (CException* pe) {
			pe->Delete();
			return(FALSE);
		}
		return(TRUE);
	}
	return(FALSE);
}

BOOL CDib::GetImagePixelWithIndexValidation(int col, int row, COLORREF* color, BYTE* alpha)
{
	if (row < 0 || col < 0 || row >= abs(m_lpBMIH->biHeight) || col >= m_lpBMIH->biWidth)
		return(FALSE);

	*color = GetImagePixel(col, row, alpha);
	return(TRUE);
}

BOOL CDib::GetRawImagePixelWithIndexValidation(int col, int row, COLORREF* color, BYTE* alpha)
{
	if (row < 0 || col < 0 || row >= abs(m_lpBMIH->biHeight) || col >= m_lpBMIH->biWidth)
		return(FALSE);

	*color = GetRawImagePixel(col, row, alpha);
	return(TRUE);
}

COLORREF CDib::GetImagePixel(int col, int row, BYTE* alpha)
{
	COLORREF color = RGB(0, 0, 0);
	int rrow = row;
	int pixoffset = 0;
	WORD val;
	
	// check to see that row and column are in image
	if (row < 0 || col < 0 || row >= abs(m_lpBMIH->biHeight) || col >= m_lpBMIH->biWidth)
		return(color);

	LPRGBQUAD pDibQuad;
	if (m_lpBMIH->biBitCount == 8 || m_lpBMIH->biBitCount == 4 || m_lpBMIH->biBitCount == 1)
		pDibQuad = (LPRGBQUAD) m_lpvColorTable;

	// if image is stored bottom up, adjust row value
	if (m_lpBMIH->biHeight < 0)
		rrow = (m_lpBMIH->biHeight - 1) - row;

	if (m_lpImage) {
		switch (m_lpBMIH->biBitCount) {
			case 1:
				color = m_lpImage[m_dwBytesPerScanLine * rrow + col / 8];
				color = (color & (BYTE) (0x80 >> col % 8)) >> (7 - (col % 8));

				// get actual color from palette
				color = RGB(pDibQuad[color].rgbRed, pDibQuad[color].rgbGreen, pDibQuad[color].rgbBlue);
				break;
			case 4:
				if (col % 2 == 0) {
					color  = (m_lpImage[m_dwBytesPerScanLine * rrow + col / 2] & 0xF0) >> 4;
				}
				else {
					color  = m_lpImage[m_dwBytesPerScanLine * rrow + col / 2] & 0x0F;
				}
				
				// get actual color from palette
				color = RGB(pDibQuad[color].rgbRed, pDibQuad[color].rgbGreen, pDibQuad[color].rgbBlue);
				break;
			case 8:
				color = RGB(pDibQuad[m_lpImage[m_dwBytesPerScanLine * rrow + col]].rgbRed, pDibQuad[m_lpImage[m_dwBytesPerScanLine * rrow + col]].rgbGreen, pDibQuad[m_lpImage[m_dwBytesPerScanLine * rrow + col]].rgbBlue);
				break;
			case 16:
				val = ((m_lpImage[m_dwBytesPerScanLine * rrow + col * 2 + 1]) << 8) + m_lpImage[m_dwBytesPerScanLine * rrow + col * 2];
				color = RGB((val & 0x7c00) >> 10, (val & 0x03e0) >> 5, val & 0x001f);
				break;
			case 24:
				pixoffset = m_dwBytesPerScanLine * rrow + col * 3;
				color = RGB(m_lpImage[pixoffset + 2], m_lpImage[pixoffset + 1], m_lpImage[pixoffset + 0]);
				break;
			case 32:
				pixoffset = m_dwBytesPerScanLine * rrow + col * 4;
				color = RGB(m_lpImage[pixoffset + 2], m_lpImage[pixoffset + 1], m_lpImage[pixoffset + 0]);
				if (alpha)
					*alpha = (BYTE) m_lpImage[pixoffset + 3];
				break;
		}
	}

	return(color);
}

COLORREF CDib::GetRawImagePixel(int col, int row, BYTE* alpha)
{
	// returns color or palette index from image...useful when image contains data
	COLORREF color = RGB(0, 0, 0);
	int rrow = row;
	int pixoffset = 0;
	WORD val;
	
	// check to see that row and column are in image
	if (row < 0 || col < 0 || row >= abs(m_lpBMIH->biHeight) || col >= m_lpBMIH->biWidth)
		return(color);

	LPRGBQUAD pDibQuad;
	if (m_lpBMIH->biBitCount == 8 || m_lpBMIH->biBitCount == 4 || m_lpBMIH->biBitCount == 1)
		pDibQuad = (LPRGBQUAD) m_lpvColorTable;

	// if image is stored bottom up, adjust row value
	if (m_lpBMIH->biHeight < 0)
		rrow = (m_lpBMIH->biHeight - 1) - row;

	if (m_lpImage) {
		switch (m_lpBMIH->biBitCount) {
			case 1:
				color = m_lpImage[m_dwBytesPerScanLine * rrow + col / 8];
				color = (color & (BYTE) (0x80 >> col % 8)) >> (7 - (col % 8));
				break;
			case 4:
				if (col % 2 == 0) {
					color  = (m_lpImage[m_dwBytesPerScanLine * rrow + col / 2] & 0xF0) >> 4;
				}
				else {
					color  = m_lpImage[m_dwBytesPerScanLine * rrow + col / 2] & 0x0F;
				}
				break;
			case 8:
				color = m_lpImage[m_dwBytesPerScanLine * rrow + col];
				break;
			case 16:
				val = ((m_lpImage[m_dwBytesPerScanLine * rrow + col * 2 + 1]) << 8) + m_lpImage[m_dwBytesPerScanLine * rrow + col * 2];
				color = RGB((val & 0x7c00) >> 10, (val & 0x03e0) >> 5, val & 0x001f);
				break;
			case 24:
				pixoffset = m_dwBytesPerScanLine * rrow + col * 3;
				color = RGB(m_lpImage[pixoffset + 2], m_lpImage[pixoffset + 1], m_lpImage[pixoffset + 0]);
				break;
			case 32:
				pixoffset = m_dwBytesPerScanLine * rrow + col * 4;
				color = RGB(m_lpImage[pixoffset + 2], m_lpImage[pixoffset + 1], m_lpImage[pixoffset + 0]);
				if (alpha)
					*alpha = (BYTE) m_lpImage[pixoffset + 3];
				break;
		}
	}

	return(color);
}

void CDib::SetImagePixel(int col, int row, COLORREF color, BYTE alpha)
{
	// set the pixel value...palette index for 1, 4, & 8 bit images, actual color for 16, 24, & 32 bit images
	BYTE r = GetRValue(color);
	BYTE g = GetGValue(color);
	BYTE b = GetBValue(color);
	int rrow = row;
	int pixoffset = 0;
	WORD val = 0;
	BYTE bval;
	
	// check to see that row and column are in image
	if (row < 0 || col < 0 || row >= abs(m_lpBMIH->biHeight) || col >= m_lpBMIH->biWidth)
		return;

	// if image is stored bottom up, adjust row value
	if (m_lpBMIH->biHeight < 0)
		rrow = (m_lpBMIH->biHeight - 1) - row;

	if (m_lpImage) {
		switch (m_lpBMIH->biBitCount) {
			case 1:
				if (color)		// set the bit
					m_lpImage[m_dwBytesPerScanLine * rrow + col / 8] |= (BYTE) (0x80 >> col % 8);
				else			// clear the bit
					m_lpImage[m_dwBytesPerScanLine * rrow + col / 8] &= (BYTE) ((0x80 >> col % 8) ^ 0xFF);
				break;
			case 4:
				if (col % 2 == 0) {
					m_lpImage[m_dwBytesPerScanLine * rrow + col / 2] &= 0x0F;		// clear first 4 bits
					m_lpImage[m_dwBytesPerScanLine * rrow + col / 2] |= (BYTE) ((color << 4) & 0xF0);
				}
				else {
					m_lpImage[m_dwBytesPerScanLine * rrow + col / 2] &= 0xF0;		// clear last 4 bits
					m_lpImage[m_dwBytesPerScanLine * rrow + col / 2] |= (BYTE) (color & 0x0F);
				}
				break;
			case 8:
				m_lpImage[m_dwBytesPerScanLine * rrow + col] = (BYTE) color;
				break;
			case 16:
				val = (r & 0x7c00) << 10 | (g & 0x03e0) << 5 | b & 0x001f;
				bval = (BYTE) (val & 0x00ff);
				m_lpImage[m_dwBytesPerScanLine * rrow + col * 2] = bval;
				bval = (BYTE) (val & 0xff00 >> 8);
				m_lpImage[m_dwBytesPerScanLine * rrow + col * 2 + 1] = bval;
				break;
			case 24:
				pixoffset = m_dwBytesPerScanLine * rrow + col * 3;
				m_lpImage[pixoffset + 0] = b;
				m_lpImage[pixoffset + 1] = g;
				m_lpImage[pixoffset + 2] = r;
				break;
			case 32:
				pixoffset = m_dwBytesPerScanLine * rrow + col * 4;
				m_lpImage[pixoffset + 0] = b;
				m_lpImage[pixoffset + 1] = g;
				m_lpImage[pixoffset + 2] = r;
				m_lpImage[pixoffset + 3] = alpha;
				break;
		}
	}
}
