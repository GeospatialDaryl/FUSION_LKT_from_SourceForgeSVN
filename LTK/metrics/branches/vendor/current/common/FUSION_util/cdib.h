// cdib.h declaration for Inside Visual C++ CDib class

#ifndef _CDIB
#define _CDIB

class CDib : public CObject
{
	DECLARE_SERIAL(CDib)
public:
	LPVOID m_lpvColorTable;
	HBITMAP m_hBitmap;
	LPBYTE m_lpImage;  // starting address of DIB bits
	LPBITMAPINFOHEADER m_lpBMIH; //  buffer containing the BITMAPINFOHEADER
	DWORD m_dwBytesPerScanLine;	// length of scan line with padding
private:
	HGLOBAL m_hGlobal; // For external windows we need to free;
	                   //  could be allocated by this class or allocated externally
protected:
	enum Alloc {noAlloc, crtAlloc, heapAlloc};
	Alloc m_nBmihAlloc;
	Alloc m_nImageAlloc;
	DWORD m_dwSizeImage; // of bits -- not BITMAPINFOHEADER or BITMAPFILEHEADER
	int m_nColorTableEntries;
	
	HANDLE m_hFile;
	HANDLE m_hMap;
	LPVOID m_lpvFile;
	HPALETTE m_hPalette;
public:
	COLORREF GetImagePixel(int col, int row, BYTE* alpha);
	COLORREF GetRawImagePixel(int col, int row, BYTE* alpha);
	BOOL GetImagePixelWithIndexValidation(int col, int row, COLORREF* color, BYTE* alpha);
	BOOL GetRawImagePixelWithIndexValidation(int col, int row, COLORREF* color, BYTE* alpha);
	void SetImagePixel(int col, int row, COLORREF color, BYTE alpha = 255);
	int GetColorBits(LPCTSTR szFileName);
	BOOL CheckFileFormat(LPCTSTR szFileName);
	CDib();
	CDib(CSize size, int nBitCount);	// builds BITMAPINFOHEADER
	~CDib();
	int GetSizeImage() {return m_dwSizeImage;}
	int GetSizeHeader()
		{return sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * m_nColorTableEntries;}
	CSize GetDimensions();
	void GetDimensions(CSize& sz);
	BOOL AttachMapFile(const char* strPathname, BOOL bShare = FALSE);
	BOOL CopyToMapFile(const char* strPathname);
	BOOL AttachMemory(LPVOID lpvMem, BOOL bMustDelete = FALSE, HGLOBAL hGlobal = NULL);
	BOOL Draw(CDC* pDC, CPoint origin, CSize size, int BltMode = COLORONCOLOR);  // until we implemnt CreateDibSection
	BOOL DrawPart(CDC* pDC, CPoint origin, CSize size, CPoint iorigin, CSize isize, int BltMode = COLORONCOLOR);
	HBITMAP CreateSection(CDC* pDC = NULL);
	UINT UsePalette(CDC* pDC, BOOL bBackground = FALSE);
	BOOL MakePalette();
	BOOL SetSystemPalette(CDC* pDC);
	BOOL Compress(CDC* pDC, BOOL bCompress = TRUE); // FALSE means decompress
	HBITMAP CreateBitmap(CDC* pDC);
	BOOL Read(CFile* pFile);
	BOOL ReadHeader(CFile* pFile);
	BOOL ReadSection(CFile* pFile, CDC* pDC = NULL);
	BOOL Write(CFile* pFile);
	void Serialize(CArchive& ar);
	void Empty();
private:
	void DetachMapFile();

public:
	void ComputePaletteSize(int nBitCount);
	void ComputeMetrics();
};
#endif // _CDIB
