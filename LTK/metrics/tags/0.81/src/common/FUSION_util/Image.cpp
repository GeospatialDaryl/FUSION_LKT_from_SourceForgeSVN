// Image.cpp: implementation of the CImage class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
//#include "resource.h"
#include "math.h"
#include "Image.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
//#define new DEBUG_NEW
#endif

// TIFF stuff
#define HDIB HANDLE
#define IS_WIN30_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPINFOHEADER))
#define CVT(x)      (((x) * 255) / ((1 << 16) - 1))

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CImage::CImage()
{
	m_ShowProgress = FALSE;
}

CImage::~CImage()
{

}

int CImage::DetectImageFileFormat(LPCTSTR szFileName)
{
	FILE *f;
	char tbuf[64];
	char szts[_MAX_PATH];

	// simple test looking at extension
	strcpy(szts, szFileName);
	AnsiLower(szts);

	if (strstr(szts, ".bmp"))
		return(BMP_FORMAT);
	else if (strstr(szts, ".pcx"))
		return(PCX_FORMAT);
	else if (strstr(szts, ".tif"))
		return(TIFF_FORMAT);
	else if (strstr(szts, ".tiff"))
		return(TIFF_FORMAT);
	else if (strstr(szts, ".ppm"))
		return(PPM_FORMAT);
	else if (strstr(szts, ".pbm"))
		return(PPM_FORMAT);
	else if (strstr(szts, ".jpg"))
		return(JPEG_FORMAT);
	else if (strstr(szts, ".jpeg"))
		return(JPEG_FORMAT);

	// simple test failed...look for magic numbers
	f = fopen((LPCTSTR) szFileName, "rb");
	if (f != (FILE *) NULL) {
		// read head of file
		fread(tbuf, sizeof(char), 64, f);
		fclose(f);

		// look for magic numbers
		if (tbuf[0] == 0x0a)
			return(PCX_FORMAT);

		if (tbuf[0] == 0x42 && tbuf[1] == 0x4d)
			return(BMP_FORMAT);

		if (tbuf[0] == 0xff && tbuf[1] == 0xd8)
			return(JPEG_FORMAT);

		if (tbuf[0] != 'P' || tbuf[1] != '6')
			return(PPM_FORMAT);
	}
	return(-1);
}

BOOL CImage::ReadBMPImageHeader(LPCTSTR szFileName)
{
	CFile file;

	// try to open BMP file
	if (file.Open(szFileName, CFile::modeRead)) {
		SetupProgressDlg(100);

		// try to read DIB
		if (ReadHeader(&file) == TRUE) {
			file.Close();

			return(TRUE);
		}
	}

	return(FALSE);
}

BOOL CImage::ReadBMPImage(LPCTSTR szFileName)
{
	CFile file;

	// try to open BMP file
	if (file.Open(szFileName, CFile::modeRead)) {
		SetupProgressDlg(100);

		// try to read DIB
		if (Read(&file) == TRUE) {
			file.Close();

			return(TRUE);
		}
	}

	return(FALSE);
}

BOOL CImage::ReadJPEGImageHeader(LPCTSTR szFileName)
{
	FILE *hFile;

	if ((hFile = fopen(szFileName, "rb")) != (FILE *) NULL) {
		struct jpeg_decompress_struct cinfo;
		struct jpeg_error_mgr jerr;

		// Initialize the JPEG decompression object with default error handling
		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_decompress(&cinfo);

		// Specify data source for decompression
		jpeg_stdio_src(&cinfo, hFile);

		// Read file header, set default decompression parameters
		(void) jpeg_read_header(&cinfo, TRUE);

		// use RGB color space
		cinfo.out_color_space = JCS_RGB;

		// Start decompressor
//		(void) jpeg_start_decompress(&cinfo);

		// Allocate memory for header
		LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER) new BITMAPINFOHEADER;

		// fill out the BITMAPINFOHEADER entries
		lpbi->biSize = (DWORD) sizeof(BITMAPINFOHEADER);
		lpbi->biWidth = (LONG) cinfo.image_width;
		lpbi->biHeight = (LONG) cinfo.image_height;
		lpbi->biPlanes = (WORD) 1;
		lpbi->biBitCount = (WORD) 24;
		lpbi->biCompression = BI_RGB;
		lpbi->biSizeImage = (DWORD) ((((lpbi->biWidth * (DWORD)lpbi->biBitCount) + 31) & ~31) >> 3) * lpbi->biHeight;
		lpbi->biXPelsPerMeter = 0L;
		lpbi->biYPelsPerMeter = 0L;
		lpbi->biClrUsed = (DWORD) 0;
		lpbi->biClrImportant = 0;

		// get a proper-sized buffer for header, color table and bits
		int nSize = lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);

		m_lpBMIH = (LPBITMAPINFOHEADER) new char[nSize];

		m_nBmihAlloc = m_nImageAlloc = crtAlloc;

		m_lpBMIH->biSize = lpbi->biSize;
		m_lpBMIH->biWidth = lpbi->biWidth;
		m_lpBMIH->biHeight = lpbi->biHeight;
		m_lpBMIH->biPlanes = lpbi->biPlanes;
		m_lpBMIH->biBitCount = lpbi->biBitCount;
		m_lpBMIH->biCompression = lpbi->biCompression;
		m_lpBMIH->biSizeImage = lpbi->biSizeImage;
		m_lpBMIH->biXPelsPerMeter = lpbi->biXPelsPerMeter;
		m_lpBMIH->biYPelsPerMeter = lpbi->biYPelsPerMeter;
		m_lpBMIH->biClrUsed = lpbi->biClrUsed;
		m_lpBMIH->biClrImportant = lpbi->biClrImportant;

		delete lpbi;

		ComputeMetrics();
		ComputePaletteSize(m_lpBMIH->biBitCount);

		m_lpImage = NULL;

		// read was successful...cleanup and exit
//		(void) jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);

		fclose(hFile);
		return(TRUE);
	}

	return(FALSE);
}

BOOL CImage::ReadJPEGImage(LPCTSTR szFileName, BOOL CreatePalette)
{
	// NOTE about IJG JPEG code: Default byte order for RGB is BGR. Change order in jmorecfg.h by modifying the definitions of RGB_RED, RGB_GREEN, and RGB_BLUE

	FILE *hFile;

	if ((hFile = fopen(szFileName, "rb")) != (FILE *) NULL) {
		DWORD linewidth;
		DWORD i;
		unsigned char* offBits;
		
		struct jpeg_decompress_struct cinfo;
		struct jpeg_error_mgr jerr;

		// Initialize the JPEG decompression object with default error handling
		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_decompress(&cinfo);

		// Specify data source for decompression
		jpeg_stdio_src(&cinfo, hFile);

		// Read file header, set default decompression parameters
		(void) jpeg_read_header(&cinfo, TRUE);

		// use RGB color space
		cinfo.out_color_space = JCS_RGB;

		if (CreatePalette) {
			cinfo.quantize_colors = TRUE;
			cinfo.two_pass_quantize = TRUE;		// enable for slightly better color map...slower
			cinfo.desired_number_of_colors = 256;
			cinfo.colormap = NULL;
		}

		// Start decompressor
		(void) jpeg_start_decompress(&cinfo);

		SetupProgressDlg((int) cinfo.image_height);

		// Allocate memory for header
		LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER) new BITMAPINFOHEADER;

		// fill out the BITMAPINFOHEADER entries
		lpbi->biSize = (DWORD) sizeof(BITMAPINFOHEADER);
		lpbi->biWidth = (LONG) cinfo.image_width;
		lpbi->biHeight = (LONG) cinfo.image_height;
		lpbi->biPlanes = (WORD) 1;
		lpbi->biBitCount = (WORD) 24;
		lpbi->biCompression = BI_RGB;
		lpbi->biSizeImage = (DWORD) ((((lpbi->biWidth * (DWORD)lpbi->biBitCount) + 31) & ~31) >> 3) * lpbi->biHeight;
		lpbi->biXPelsPerMeter = 0L;
		lpbi->biYPelsPerMeter = 0L;
		lpbi->biClrUsed = (DWORD) 0;
		lpbi->biClrImportant = 0;

		if (CreatePalette) {
			lpbi->biBitCount = (WORD) 8;
			lpbi->biClrUsed = (DWORD) cinfo.actual_number_of_colors;
			lpbi->biClrImportant = cinfo.actual_number_of_colors;
			lpbi->biSizeImage = (DWORD) ((((lpbi->biWidth * (DWORD)lpbi->biBitCount) + 31) & ~31) >> 3) * lpbi->biHeight;
		}

		// get a proper-sized buffer for header, color table and bits
		int nSize = lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);

		m_lpBMIH = (LPBITMAPINFOHEADER) new char[nSize];

		m_nBmihAlloc = m_nImageAlloc = crtAlloc;

		m_lpBMIH->biSize = lpbi->biSize;
		m_lpBMIH->biWidth = lpbi->biWidth;
		m_lpBMIH->biHeight = lpbi->biHeight;
		m_lpBMIH->biPlanes = lpbi->biPlanes;
		m_lpBMIH->biBitCount = lpbi->biBitCount;
		m_lpBMIH->biCompression = lpbi->biCompression;
		m_lpBMIH->biSizeImage = lpbi->biSizeImage;
		m_lpBMIH->biXPelsPerMeter = lpbi->biXPelsPerMeter;
		m_lpBMIH->biYPelsPerMeter = lpbi->biYPelsPerMeter;
		m_lpBMIH->biClrUsed = lpbi->biClrUsed;
		m_lpBMIH->biClrImportant = lpbi->biClrImportant;

		delete lpbi;

		ComputeMetrics();
		ComputePaletteSize(m_lpBMIH->biBitCount);

		// set up palette
		if (CreatePalette) {
			for (i = 0; i < m_lpBMIH->biClrUsed; i ++) {
				*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD)) = cinfo.colormap[0][i];
				*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD) + 1) = cinfo.colormap[1][i];
				*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD) + 2) = cinfo.colormap[2][i];
			}
		}
		
		// create palette
		MakePalette();

		// compute width of 1 scan line
		linewidth = (DWORD) ((((m_lpBMIH->biWidth * (DWORD) m_lpBMIH->biBitCount) + 31) & ~31) >> 3);

		// create memory for image pixels
		m_lpImage = (LPBYTE) new char[m_dwSizeImage];

		// offset to the bits from start of DIB header
		offBits = m_lpImage;

		// set offset to end last row of pixel values since bitmaps start at the bottom
		offBits += (linewidth) * ((DWORD) m_lpBMIH->biHeight - 1);

		// read scan lines and move to bitmap
		while (cinfo.output_scanline < cinfo.output_height) {
			jpeg_read_scanlines(&cinfo, &offBits, 1);

			// testing code
//			for (i = 0; i < linewidth; i += 3) {
//				tbyte = offBits[i + 2];
//				offBits[i + 2] = offBits[i];
//				offBits[i] = tbyte;
//			}
			// end testing code

			offBits -= linewidth;

			UpdateProgressDlg((int) cinfo.output_scanline);
		}

		// read was successful...cleanup and exit
		(void) jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);

		fclose(hFile);
		return(TRUE);
	}

	return(FALSE);
}

BOOL CImage::ReadPCXImageHeader(LPCTSTR szFileName)
{
	FILE *hFile;

	if ((hFile = fopen(szFileName, "rb")) != (FILE *) NULL) {
		PCX_HEADER pcx_header;

		// read PCX header from beginning of file
		fseek(hFile, 0L, SEEK_SET);
		fread((LPSTR)&pcx_header, sizeof(PCX_HEADER), 1, hFile);

		// make sure we have a PCX file
		if (pcx_header.mfg != PCX_HEADER_MARKER)
			return(NULL);

		SetupProgressDlg((int) (pcx_header.ymax - pcx_header.ymin + 1));

		// Allocate memory for header
		LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER) new BITMAPINFOHEADER;

		// fill out the BITMAPINFOHEADER entries
		lpbi->biSize = (DWORD) sizeof(BITMAPINFOHEADER);
		lpbi->biWidth = (LONG) pcx_header.xmax - pcx_header.xmin + 1;
		lpbi->biHeight = (LONG) pcx_header.ymax - pcx_header.ymin + 1;
		lpbi->biPlanes = (WORD) pcx_header.nplanes;
		lpbi->biBitCount = (WORD) pcx_header.bpp;
		lpbi->biCompression = BI_RGB;
		lpbi->biSizeImage = (DWORD) ((((lpbi->biWidth * (DWORD)lpbi->biBitCount) + 31) & ~31) >> 3) * lpbi->biHeight;
		lpbi->biXPelsPerMeter = 0L;
		lpbi->biYPelsPerMeter = 0L;
		lpbi->biClrUsed = (DWORD) (1 << pcx_header.bpp);
		lpbi->biClrImportant = lpbi->biClrUsed;

		// get a proper-sized buffer for header, color table and bits
		int nSize = lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);

		m_lpBMIH = (LPBITMAPINFOHEADER) new char[nSize];

		m_nBmihAlloc = m_nImageAlloc = crtAlloc;

		m_lpBMIH->biSize = lpbi->biSize;
		m_lpBMIH->biWidth = lpbi->biWidth;
		m_lpBMIH->biHeight = lpbi->biHeight;
		m_lpBMIH->biPlanes = lpbi->biPlanes;
		m_lpBMIH->biBitCount = lpbi->biBitCount;
		m_lpBMIH->biCompression = lpbi->biCompression;
		m_lpBMIH->biSizeImage = lpbi->biSizeImage;
		m_lpBMIH->biXPelsPerMeter = lpbi->biXPelsPerMeter;
		m_lpBMIH->biYPelsPerMeter = lpbi->biYPelsPerMeter;
		m_lpBMIH->biClrUsed = lpbi->biClrUsed;
		m_lpBMIH->biClrImportant = lpbi->biClrImportant;

		delete lpbi;

		ComputeMetrics();
		ComputePaletteSize(m_lpBMIH->biBitCount);

		m_lpImage = NULL;

		// read was successful...exit
		fclose(hFile);
		return(TRUE);
	}
	return(FALSE);
}

BOOL CImage::ReadPCXImage(LPCTSTR szFileName)
{
	FILE *hFile;

	if ((hFile = fopen(szFileName, "rb")) != (FILE *) NULL) {
		PCX_HEADER pcx_header;
		int	i, j;
		DWORD linewidth;
		unsigned char* offBits;

		// read PCX header from beginning of file
		fseek(hFile, 0L, SEEK_SET);
		fread((LPSTR)&pcx_header, sizeof(PCX_HEADER), 1, hFile);

		// make sure we have a PCX file
		if (pcx_header.mfg != PCX_HEADER_MARKER)
			return(NULL);

		SetupProgressDlg((int) (pcx_header.ymax - pcx_header.ymin + 1));

		// Allocate memory for header
		LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER) new BITMAPINFOHEADER;

		// fill out the BITMAPINFOHEADER entries
		lpbi->biSize = (DWORD) sizeof(BITMAPINFOHEADER);
		lpbi->biWidth = (LONG) pcx_header.xmax - pcx_header.xmin + 1;
		lpbi->biHeight = (LONG) pcx_header.ymax - pcx_header.ymin + 1;
		lpbi->biPlanes = (WORD) pcx_header.nplanes;
		lpbi->biBitCount = (WORD) pcx_header.bpp;
		lpbi->biCompression = BI_RGB;
		lpbi->biSizeImage = (DWORD) ((((lpbi->biWidth * (DWORD)lpbi->biBitCount) + 31) & ~31) >> 3) * lpbi->biHeight;
		lpbi->biXPelsPerMeter = 0L;
		lpbi->biYPelsPerMeter = 0L;
		lpbi->biClrUsed = (DWORD) (1 << pcx_header.bpp);
		lpbi->biClrImportant = lpbi->biClrUsed;

		// get a proper-sized buffer for header, color table and bits
		int nSize = lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);

		m_lpBMIH = (LPBITMAPINFOHEADER) new char[nSize];

		m_nBmihAlloc = m_nImageAlloc = crtAlloc;

		m_lpBMIH->biSize = lpbi->biSize;
		m_lpBMIH->biWidth = lpbi->biWidth;
		m_lpBMIH->biHeight = lpbi->biHeight;
		m_lpBMIH->biPlanes = lpbi->biPlanes;
		m_lpBMIH->biBitCount = lpbi->biBitCount;
		m_lpBMIH->biCompression = lpbi->biCompression;
		m_lpBMIH->biSizeImage = lpbi->biSizeImage;
		m_lpBMIH->biXPelsPerMeter = lpbi->biXPelsPerMeter;
		m_lpBMIH->biYPelsPerMeter = lpbi->biYPelsPerMeter;
		m_lpBMIH->biClrUsed = lpbi->biClrUsed;
		m_lpBMIH->biClrImportant = lpbi->biClrImportant;

		delete lpbi;

		ComputeMetrics();
		ComputePaletteSize(m_lpBMIH->biBitCount);

		// set the color table...if 16-colors, palette entries are in header
		if (pcx_header.bpp < 8) {
			j = 0;
			for (i = 0; i < 16; i ++) {
				// have to swap red and blue components and set extra byte
				*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD)) = (BYTE) pcx_header.palette[i * 3 + 2];
				*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD) + 1) = (BYTE) pcx_header.palette[i * 3 + 1];
				*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD) + 2) = (BYTE) pcx_header.palette[i * 3];
			}
		}
		else {
			// palette entries at end of file
			char c;

			// seek to start of color table
			fseek(hFile, -769L, SEEK_END);

			// read the magic byte
			fread(&c, sizeof(BYTE), 1, hFile);

			// if there is a color table...read it
			if (c == 12) {
				for (i = 0; i < 256; i ++) {
					fread(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD), sizeof(BYTE), 3, hFile);

					// swap blue and red entries
					c = *(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD));
					*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD)) = *(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD) + 2);
					*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD) + 2) = c;
				}
			}

			// seek to start of image bytes
			fseek(hFile, sizeof(PCX_HEADER), SEEK_SET);
		}
		
		// create palette
		MakePalette();

		// compute width of 1 scan line
		linewidth = (DWORD) ((((m_lpBMIH->biWidth * (DWORD) m_lpBMIH->biBitCount) + 31) & ~31) >> 3);

		// create memory for image pixels
		m_lpImage = (LPBYTE) new char[m_dwSizeImage];

		// offset to the bits from start of DIB header
		offBits = m_lpImage;

		// set offset to end last row of pixel values since bitmaps start at the bottom
		offBits += (linewidth) * ((DWORD) m_lpBMIH->biHeight - 1);

		// read scan lines and move to bitmap
		for (i = 0; i < pcx_header.ymax - pcx_header.ymin + 1; i ++) {
			if (PCXReadNextLine(&pcx_header, offBits, hFile) == NULL)
				break;;

			offBits -= linewidth;

			UpdateProgressDlg(i);
		}

		// read was successful...exit
		fclose(hFile);
		return(TRUE);
	}
	return(FALSE);
}

BOOL CImage::ReadTIFFImageHeader(LPCTSTR szFileName)
{
    TIFF*			tif;
    unsigned long	imageLength; 
    unsigned long	imageWidth; 
    unsigned short	BitsPerSample;
    unsigned long	TIFFLineSize;
    unsigned short	SamplePerPixel;
    unsigned long	RowsPerStrip;  
    short			PhotometricInterpretation;
    
    tif = TIFFOpen(szFileName, "r");
    
    if (tif) {
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &imageWidth);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imageLength);  
		TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &BitsPerSample);
		TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &RowsPerStrip);  
		TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &PhotometricInterpretation);

		// set up progress indicator
		SetupProgressDlg((int) imageLength);
           
		TIFFLineSize = TIFFScanlineSize(tif); //Number of byte in one line

		SamplePerPixel = (int) (TIFFLineSize/imageWidth);

		// Allocate memory for header
		LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER) new BITMAPINFOHEADER;

		// fill out the BITMAPINFOHEADER entries
		lpbi->biSize = (DWORD) sizeof(BITMAPINFOHEADER);
		lpbi->biWidth = (LONG) imageWidth;
		lpbi->biHeight = (LONG) imageLength;
		lpbi->biPlanes = (WORD) 1;
		lpbi->biBitCount = (WORD) SamplePerPixel * BitsPerSample;
		lpbi->biCompression = BI_RGB;
		lpbi->biSizeImage = (DWORD) ((((lpbi->biWidth * (DWORD)lpbi->biBitCount) + 31) & ~31) >> 3) * lpbi->biHeight;
		lpbi->biXPelsPerMeter = 0L;
		lpbi->biYPelsPerMeter = 0L;
		if (PhotometricInterpretation == 2)
			lpbi->biClrUsed = (DWORD) 0;
		else
			lpbi->biClrUsed = (DWORD) (1 << BitsPerSample);
		lpbi->biClrImportant = lpbi->biClrUsed;

		// get a proper-sized buffer for header, color table and bits
		int nSize = lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);

		m_lpBMIH = (LPBITMAPINFOHEADER) new char[nSize];

		m_nBmihAlloc = m_nImageAlloc = crtAlloc;

		m_lpBMIH->biSize = lpbi->biSize;
		m_lpBMIH->biWidth = lpbi->biWidth;
		m_lpBMIH->biHeight = lpbi->biHeight;
		m_lpBMIH->biPlanes = lpbi->biPlanes;
		m_lpBMIH->biBitCount = lpbi->biBitCount;
		m_lpBMIH->biCompression = lpbi->biCompression;
		m_lpBMIH->biSizeImage = lpbi->biSizeImage;
		m_lpBMIH->biXPelsPerMeter = lpbi->biXPelsPerMeter;
		m_lpBMIH->biYPelsPerMeter = lpbi->biYPelsPerMeter;
		m_lpBMIH->biClrUsed = lpbi->biClrUsed;
		m_lpBMIH->biClrImportant = lpbi->biClrImportant;

		delete lpbi;

		ComputeMetrics();
		ComputePaletteSize(m_lpBMIH->biBitCount);
		TIFFClose(tif);

		m_lpImage = NULL;

		return(TRUE);
	}
	return(FALSE);
}

BOOL CImage::ReadTIFFImage(LPCTSTR szFileName, int ColorOption)
{
    TIFF*			tif;
    unsigned long	imageLength; 
    unsigned long	imageWidth; 
    unsigned long	tileLength = 0; 
    unsigned long	tileWidth = 0; 
    unsigned short	BitsPerSample;
    unsigned long	TIFFLineSize;
    unsigned short	SamplePerPixel;
    unsigned long	RowsPerStrip;  
	unsigned long	StripByteCounts;
	unsigned long	StripOffsets;
	int				StripsPerImage;
    short			PhotometricInterpretation;
    short			PlanarConfiguration;
    long			nrow;
	unsigned long	row;
    char*			buf;          
	unsigned char*	offBits;
    HGLOBAL			hStrip;
    int				i, l;
	DWORD			linewidth;
    
    tif = TIFFOpen(szFileName, "r");
    
    if (tif) {
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &imageWidth);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imageLength);  
		TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &BitsPerSample);
		TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &RowsPerStrip);  
		TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &PhotometricInterpretation);
		TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &PlanarConfiguration);
		TIFFGetField(tif, TIFFTAG_STRIPBYTECOUNTS, &StripByteCounts);
		TIFFGetField(tif, TIFFTAG_STRIPOFFSETS, &StripOffsets);

		if (TIFFIsTiled(tif)) {
			TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tileWidth);
			TIFFGetField(tif, TIFFTAG_TILELENGTH, &tileLength);  
		}

		// set up progress indicator
		SetupProgressDlg((int) imageLength);
           
		TIFFLineSize = TIFFScanlineSize(tif); //Number of byte in one line

		SamplePerPixel = (int) (TIFFLineSize/imageWidth);

		StripsPerImage = TIFFNumberOfStrips(tif);

		// if RGB image with 8 bits/sample and PlanarConfiguration=2, RGB values are interleaved
		if (BitsPerSample == 8 && PhotometricInterpretation == 2 && PlanarConfiguration == 2) {
			SamplePerPixel = (unsigned short) (StripsPerImage / imageLength);
		}

		// Allocate memory for header
		LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER) new BITMAPINFOHEADER;

		// fill out the BITMAPINFOHEADER entries
		lpbi->biSize = (DWORD) sizeof(BITMAPINFOHEADER);
		lpbi->biWidth = (LONG) imageWidth;
		lpbi->biHeight = (LONG) imageLength;
		lpbi->biPlanes = (WORD) 1;
		lpbi->biBitCount = (WORD) SamplePerPixel * BitsPerSample;
		lpbi->biCompression = BI_RGB;
		lpbi->biSizeImage = (DWORD) ((((lpbi->biWidth * (DWORD)lpbi->biBitCount) + 31) & ~31) >> 3) * lpbi->biHeight;
		lpbi->biXPelsPerMeter = 0L;
		lpbi->biYPelsPerMeter = 0L;
		if (PhotometricInterpretation == 2)
			lpbi->biClrUsed = (DWORD) 0;
		else
			lpbi->biClrUsed = (DWORD) (1 << BitsPerSample);
		lpbi->biClrImportant = lpbi->biClrUsed;

		// get a proper-sized buffer for header, color table and bits
		int nSize = lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);

		m_lpBMIH = (LPBITMAPINFOHEADER) new char[nSize];

		m_nBmihAlloc = m_nImageAlloc = crtAlloc;

		m_lpBMIH->biSize = lpbi->biSize;
		m_lpBMIH->biWidth = lpbi->biWidth;
		m_lpBMIH->biHeight = lpbi->biHeight;
		m_lpBMIH->biPlanes = lpbi->biPlanes;
		m_lpBMIH->biBitCount = lpbi->biBitCount;
		m_lpBMIH->biCompression = lpbi->biCompression;
		m_lpBMIH->biSizeImage = lpbi->biSizeImage;
		m_lpBMIH->biXPelsPerMeter = lpbi->biXPelsPerMeter;
		m_lpBMIH->biYPelsPerMeter = lpbi->biYPelsPerMeter;
		m_lpBMIH->biClrUsed = lpbi->biClrUsed;
		m_lpBMIH->biClrImportant = lpbi->biClrImportant;

		delete lpbi;

		ComputeMetrics();
		ComputePaletteSize(m_lpBMIH->biBitCount);

		// set up palette
		//PhotometricInterpretation = 0 image is Grayscale...needs palette with color ranging from white to black
		//PhotometricInterpretation = 1 image is Grayscale...needs palette with color ranging from black to white
		//PhotometricInterpretation = 2 image is RGB
		//PhotometricInterpretation = 3 image have a color palette              
		if (PhotometricInterpretation == 3 || PhotometricInterpretation == 1 || PhotometricInterpretation == 0) {
			uint16* red;
			uint16* green;
			uint16* blue;
			short i;
			int   Palette16Bits = FALSE;          

			if (PhotometricInterpretation == 3) {
				TIFFGetField(tif, TIFFTAG_COLORMAP, &red, &green, &blue); 

				//Is the palette 16 or 8 bits ?
				if (TIFFCheckCMap(1 << BitsPerSample, red, green, blue) == 16) 
					Palette16Bits = TRUE;
				else
					Palette16Bits = FALSE;
			}

			//load the palette in the DIB
			for (i = (1 << BitsPerSample) - 1; i >= 0; i --) {             
				if (Palette16Bits) {
					*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD)) = (BYTE) CVT(blue[i]);
					*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD) + 1) = (BYTE) CVT(green[i]);
					*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD) + 2) = (BYTE) CVT(red[i]);
				}
				else {
					if (PhotometricInterpretation == 3) {
						*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD)) = (BYTE) blue[i];
						*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD) + 1) = (BYTE) green[i];
						*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD) + 2) = (BYTE) red[i];
					}
					else if (PhotometricInterpretation == 1) {
						*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD)) = (BYTE) i;
						*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD) + 1) = (BYTE) i;
						*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD) + 2) = (BYTE) i;
					}
					else if (PhotometricInterpretation == 0) {
						*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD)) = (BYTE) (255 - i);
						*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD) + 1) = (BYTE) (255 - i);
						*(((LPSTR) m_lpBMIH) + m_lpBMIH->biSize + i * sizeof(RGBQUAD) + 2) = (BYTE) (255 - i);
					}
				}
			}  
		}
		
		// create palette
		MakePalette();

		// compute width of 1 scan line
		linewidth = (DWORD) ((((m_lpBMIH->biWidth * (DWORD) m_lpBMIH->biBitCount) + 31) & ~31) >> 3);

		// create memory for image pixels
		m_lpImage = (LPBYTE) new char[m_dwSizeImage];

		if (m_lpImage) {
			// see if image is tiled
			if (TIFFIsTiled(tif)) {
				unsigned long irow, icol;

				// special case to load entire image into RGBA block and then move RGB to final image
				// this could work for all TIFF flavors but it requires a 32-bit image for all image types
				// including 8-bit paletted images. This would increase the memory usage unnecessarily (IMHO)
				uint32* raster = (uint32*) malloc(imageWidth * imageLength * sizeof (uint32));
//				uint32 * raster = (uint32*) GlobalAlloc(GMEM_FIXED, (imageWidth * imageLength * sizeof (uint32)));
				if (raster) {
					if (TIFFReadRGBAImage(tif, imageWidth, imageLength, raster, 0) != -1) {
						// transfer RGB info to final image

						// offset to the bits from start of DIB header
						offBits = m_lpImage;
//						offBits += linewidth * ((DWORD) m_lpBMIH->biHeight - 1);
						// now offBits points to the bottom line
						
						// see if we have 4 bands
						if (SamplePerPixel == 4 && ColorOption == 2) {
							for (irow = 0; irow < imageLength; irow ++) {
								for (icol = 0; icol < imageWidth; icol ++) {
									offBits[icol * SamplePerPixel + 0] = (unsigned char) ((raster[irow * imageWidth + icol] & 0x0000FF00) >> 8);	// blue
									offBits[icol * SamplePerPixel + 1] = (unsigned char) ((raster[irow * imageWidth + icol] & 0x000000FF));			// green
									offBits[icol * SamplePerPixel + 2] = (unsigned char) ((raster[irow * imageWidth + icol] & 0xFF000000) >> 24);	// red...IR band
								}
								offBits += linewidth;
							}
						}
						else {
							for (irow = 0; irow < imageLength; irow ++) {
								for (icol = 0; icol < imageWidth; icol ++) {
									offBits[icol * SamplePerPixel + 0] = (unsigned char) ((raster[irow * imageWidth + icol] & 0x00FF0000) >> 16);	// blue
									offBits[icol * SamplePerPixel + 1] = (unsigned char) ((raster[irow * imageWidth + icol] & 0x0000FF00) >> 8);	// green
									offBits[icol * SamplePerPixel + 2] = (unsigned char) ((raster[irow * imageWidth + icol] & 0x000000FF));			// red
								}
								offBits += linewidth;
							}
						}

//						GlobalFree(raster);
						free(raster);
					}
				}
				else {
					AfxMessageBox("Image is stored in tiled format and is too large to load...try again after closing other programs");
				}
			}
			else {
				// offset to the bits from start of DIB header
				offBits = m_lpImage;
				offBits += linewidth * ((DWORD) m_lpBMIH->biHeight - 1);
				//now offBits points to the bottom line

				int StripSize = TIFFStripSize(tif);
				hStrip = GlobalAlloc(GHND, StripSize);
				buf = (char*) GlobalLock(hStrip);           

				int Strip;
				if (buf) {
					if (PhotometricInterpretation == 2 && PlanarConfiguration == 2) {
						// assume we have 3 bands with all of band 1, then all of band 2, then all of band 3
						for (int band = 0; band < 3; band ++) {
							// offset to the bits from start of DIB header
							offBits = m_lpImage;
							offBits += linewidth * ((DWORD) m_lpBMIH->biHeight - 1);
							//now offBits points to the bottom line

							//read the tiff lines and save them in the DIB
							//with RGB mode, we have to change the order of the 3 samples RGB<=> BGR
							for (row = 0; row < imageLength; row += RowsPerStrip) {     
								nrow = (row + RowsPerStrip > imageLength ? imageLength - row : RowsPerStrip);
								Strip = TIFFComputeStrip(tif, row, band);
								if (TIFFReadEncodedStrip(tif, Strip, buf, nrow * TIFFLineSize) != -1) {
									for (l = 0; l < nrow; l++) {
										if (PhotometricInterpretation == 2 && PlanarConfiguration == 2) {
											for (i = 0; i < (int) (imageWidth); i++) {
												offBits[i * SamplePerPixel + (2 - band)] = buf[l * TIFFLineSize + i]; 
											}
										}

										offBits -= linewidth;
									}
								}
								else
									break;

								UpdateProgressDlg(row);
							}
						}
					}
					else {
						//read the tiff lines and save them in the DIB
						//with RGB mode, we have to change the order of the 3 samples RGB<=> BGR
						for (row = 0; row < imageLength; row += RowsPerStrip) {     
							nrow = (row + RowsPerStrip > imageLength ? imageLength - row : RowsPerStrip);
							if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, row, 0), buf, nrow * TIFFLineSize) != -1) {
								for (l = 0; l < nrow; l++) {
									if (SamplePerPixel == 3) {
										for (i = 0; i < (int) (imageWidth); i++) {
											offBits[i * SamplePerPixel + 0] = buf[l * TIFFLineSize + i * SamplePerPixel + 2]; 
											offBits[i * SamplePerPixel + 1] = buf[l * TIFFLineSize + i * SamplePerPixel + 1];
											offBits[i * SamplePerPixel + 2] = buf[l * TIFFLineSize + i * SamplePerPixel + 0];
										}
									}
									else {
										memcpy(offBits, &buf[(int) (l * TIFFLineSize)], (int)imageWidth * SamplePerPixel); 
									}

									offBits -= linewidth;
								}
							}
							else
								break;

							UpdateProgressDlg(row);
						}
					}
				}
				GlobalUnlock(hStrip);
				GlobalFree(hStrip);
			}
			TIFFClose(tif);

			return(TRUE);
		}
		else {
			TIFFClose(tif);

			return(FALSE);
		}
	}
	return(FALSE);
}

BOOL CImage::ReadPPMImageHeader(LPCTSTR szFileName)
{
	FILE *hFile;

	if ((hFile = fopen(szFileName, "rb")) != (FILE *) NULL) {
		long img_width, img_height;
		BYTE max_intensity;
		char tbuf[128];

		// read header info
		fseek(hFile, 0L, SEEK_SET);

		// read 2 characters from file
		fread(tbuf, sizeof(BYTE), 2, hFile);
		tbuf[2] = '\0';

		// make sure we have a PPM file
		if (tbuf[0] != 'P' || tbuf[1] != '6')
			return(NULL);

		// parse header info
		img_width = PBMReadInteger(hFile);
		img_height = PBMReadInteger(hFile);
		max_intensity = PBMReadInteger(hFile);

		SetupProgressDlg((int) img_height);

		// Allocate memory for header
		LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER) new BITMAPINFOHEADER;

		// fill out the BITMAPINFOHEADER entries
		lpbi->biSize = (DWORD) sizeof(BITMAPINFOHEADER);
		lpbi->biWidth = img_width % 2 ? img_width + 1 : img_width;
		lpbi->biHeight = img_height;
		lpbi->biPlanes = (WORD) 1;
		lpbi->biBitCount = (WORD) 24;
		lpbi->biCompression = BI_RGB;
		lpbi->biSizeImage = (DWORD) ((((lpbi->biWidth * (DWORD)lpbi->biBitCount) + 31) & ~31) >> 3) * lpbi->biHeight;
		lpbi->biXPelsPerMeter = 0L;
		lpbi->biYPelsPerMeter = 0L;
		lpbi->biClrUsed = (DWORD) 0;
		lpbi->biClrImportant = 0;

		// get a proper-sized buffer for header, color table and bits
		int nSize = lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);

		m_lpBMIH = (LPBITMAPINFOHEADER) new char[nSize];

		m_nBmihAlloc = m_nImageAlloc = crtAlloc;

		m_lpBMIH->biSize = lpbi->biSize;
		m_lpBMIH->biWidth = lpbi->biWidth;
		m_lpBMIH->biHeight = lpbi->biHeight;
		m_lpBMIH->biPlanes = lpbi->biPlanes;
		m_lpBMIH->biBitCount = lpbi->biBitCount;
		m_lpBMIH->biCompression = lpbi->biCompression;
		m_lpBMIH->biSizeImage = lpbi->biSizeImage;
		m_lpBMIH->biXPelsPerMeter = lpbi->biXPelsPerMeter;
		m_lpBMIH->biYPelsPerMeter = lpbi->biYPelsPerMeter;
		m_lpBMIH->biClrUsed = lpbi->biClrUsed;
		m_lpBMIH->biClrImportant = lpbi->biClrImportant;

		delete lpbi;

		ComputeMetrics();
		ComputePaletteSize(m_lpBMIH->biBitCount);

		m_lpImage = NULL;

		// read was successful...exit
		fclose(hFile);
		return(TRUE);
	}
	return(FALSE);
}

BOOL CImage::ReadPPMImage(LPCTSTR szFileName)
{
	FILE *hFile;

	if ((hFile = fopen(szFileName, "rb")) != (FILE *) NULL) {
		int	i, j;
		long img_width, img_height;
		BYTE max_intensity;
		DWORD linewidth;
		unsigned char* offBits;
		char tbuf[128];

		// read header info
		fseek(hFile, 0L, SEEK_SET);

		// read 2 characters from file
		fread(tbuf, sizeof(BYTE), 2, hFile);
		tbuf[2] = '\0';

		// make sure we have a PPM file
		if (tbuf[0] != 'P' || tbuf[1] != '6')
			return(NULL);

		// parse header info
		img_width = PBMReadInteger(hFile);
		img_height = PBMReadInteger(hFile);
		max_intensity = PBMReadInteger(hFile);

		SetupProgressDlg((int) img_height);

		// Allocate memory for header
		LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER) new BITMAPINFOHEADER;

		// fill out the BITMAPINFOHEADER entries
		lpbi->biSize = (DWORD) sizeof(BITMAPINFOHEADER);
		lpbi->biWidth = img_width % 2 ? img_width + 1 : img_width;
		lpbi->biHeight = img_height;
		lpbi->biPlanes = (WORD) 1;
		lpbi->biBitCount = (WORD) 24;
		lpbi->biCompression = BI_RGB;
		lpbi->biSizeImage = (DWORD) ((((lpbi->biWidth * (DWORD)lpbi->biBitCount) + 31) & ~31) >> 3) * lpbi->biHeight;
		lpbi->biXPelsPerMeter = 0L;
		lpbi->biYPelsPerMeter = 0L;
		lpbi->biClrUsed = (DWORD) 0;
		lpbi->biClrImportant = 0;

		// get a proper-sized buffer for header, color table and bits
		int nSize = lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);

		m_lpBMIH = (LPBITMAPINFOHEADER) new char[nSize];

		m_nBmihAlloc = m_nImageAlloc = crtAlloc;

		m_lpBMIH->biSize = lpbi->biSize;
		m_lpBMIH->biWidth = lpbi->biWidth;
		m_lpBMIH->biHeight = lpbi->biHeight;
		m_lpBMIH->biPlanes = lpbi->biPlanes;
		m_lpBMIH->biBitCount = lpbi->biBitCount;
		m_lpBMIH->biCompression = lpbi->biCompression;
		m_lpBMIH->biSizeImage = lpbi->biSizeImage;
		m_lpBMIH->biXPelsPerMeter = lpbi->biXPelsPerMeter;
		m_lpBMIH->biYPelsPerMeter = lpbi->biYPelsPerMeter;
		m_lpBMIH->biClrUsed = lpbi->biClrUsed;
		m_lpBMIH->biClrImportant = lpbi->biClrImportant;

		delete lpbi;

		ComputeMetrics();
		ComputePaletteSize(m_lpBMIH->biBitCount);

		// create palette
		MakePalette();

		// compute width of 1 scan line
		linewidth = (DWORD) ((((m_lpBMIH->biWidth * (DWORD) m_lpBMIH->biBitCount) + 31) & ~31) >> 3);

		// create memory for image pixels
		m_lpImage = (LPBYTE) new char[m_dwSizeImage];

		// offset to the bits from start of DIB header
		offBits = m_lpImage;

		// set offset to end last row of pixel values since bitmaps start at the bottom
		offBits += (linewidth) * ((DWORD) m_lpBMIH->biHeight - 1);

		// read scan lines and move to bitmap
		for (i = 0; i < img_height; i ++) {
			for (j = 0; j < img_width; j ++) {
				offBits[j * 3 + 2] = (BYTE) fgetc(hFile);
				offBits[j * 3 + 1] = (BYTE) fgetc(hFile);
				offBits[j * 3] = (BYTE) fgetc(hFile);
			}

			offBits -= linewidth;

			UpdateProgressDlg(i);
		}

		// read was successful...exit
		fclose(hFile);
		return(TRUE);
	}
	return(FALSE);
}

BOOL CImage::ReadImageHeader(LPCTSTR szFileName)
{
	BOOL retcode;

	// clear existing info...if any
	Empty();

	// try to detect format
	int ImageFormat = DetectImageFileFormat(szFileName);

	if (ImageFormat >= 0) {
		AfxGetApp()->BeginWaitCursor();

		switch (ImageFormat) {
			case BMP_FORMAT:
				retcode = ReadBMPImageHeader(szFileName);
				break;
			case PCX_FORMAT:
				retcode = ReadPCXImageHeader(szFileName);
				break;
			case TIFF_FORMAT:
				retcode = ReadTIFFImageHeader(szFileName);
				break;
			case PPM_FORMAT:
				retcode = ReadPPMImageHeader(szFileName);
				break;
			case JPEG_FORMAT:
				retcode = ReadJPEGImageHeader(szFileName);

				break;
			default:
				retcode = FALSE;
		}

		if (retcode) {
			if (m_lpBMIH->biHeight >= 0)
				m_BottomUp = TRUE;
			else
				m_BottomUp = FALSE;
		}

		AfxGetApp()->EndWaitCursor();

		return(retcode);
	}
	else
		return(FALSE);
}

BOOL CImage::ReadImage(LPCTSTR szFileName, BOOL CreatePalette, BOOL ShowProgress, int ColorOption)
{
	BOOL retcode;

	// clear existing info...if any
	Empty();

	// try to detect format
	int ImageFormat = DetectImageFileFormat(szFileName);

	m_ShowProgress = ShowProgress;

	if (ImageFormat >= 0) {
		AfxGetApp()->BeginWaitCursor();

#ifdef 	USEPROGRESS
		if (m_ShowProgress) {
			m_ProgressDlg = (CImageProgress*) new CImageProgress;
			m_ProgressDlg->m_FileName = szFileName;
			m_ProgressDlg->UpdateData(FALSE);
		}
#endif
		CDC pDC;
		HDC hdc = ::GetDC(GetDesktopWindow());
		pDC.Attach(hdc);//= AfxGetApp()->GetMainWnd()->GetDC();
		switch (ImageFormat) {
			case BMP_FORMAT:
#ifdef 	USEPROGRESS
				if (m_ShowProgress) {
					m_ProgressDlg->m_FileFormat = _T("Windows bitmap");
					m_ProgressDlg->UpdateData(FALSE);
				}
#endif

				retcode = ReadBMPImage(szFileName);
				break;
			case PCX_FORMAT:
#ifdef 	USEPROGRESS
				if (m_ShowProgress) {
					m_ProgressDlg->m_FileFormat = _T("ZSoft PCX");
					m_ProgressDlg->UpdateData(FALSE);
				}
#endif

				retcode = ReadPCXImage(szFileName);
				break;
			case TIFF_FORMAT:
#ifdef 	USEPROGRESS
				if (m_ShowProgress) {
					m_ProgressDlg->m_FileFormat = _T("TIFF");
					m_ProgressDlg->UpdateData(FALSE);
				}
#endif

				retcode = ReadTIFFImage(szFileName, ColorOption);
				break;
			case PPM_FORMAT:
#ifdef 	USEPROGRESS
				if (m_ShowProgress) {
					m_ProgressDlg->m_FileFormat = _T("Portable bitmap");
					m_ProgressDlg->UpdateData(FALSE);
				}
#endif

				retcode = ReadPPMImage(szFileName);
				break;
			case JPEG_FORMAT:
#ifdef 	USEPROGRESS
				if (m_ShowProgress) {
					m_ProgressDlg->m_FileFormat = _T("JPEG");
					m_ProgressDlg->UpdateData(FALSE);
				}
#endif
				if (pDC.GetDeviceCaps(RASTERCAPS) & RC_PALETTE)
					retcode = ReadJPEGImage(szFileName, TRUE);
				else
					retcode = ReadJPEGImage(szFileName, FALSE);

				break;
			default:
				retcode = FALSE;
		}
		pDC.Detach();
		::ReleaseDC(GetDesktopWindow(), hdc);

		if (retcode) {
			if (m_lpBMIH->biHeight >= 0)
				m_BottomUp = TRUE;
			else
				m_BottomUp = FALSE;
		}

#ifdef 	USEPROGRESS
		if (m_ShowProgress) {
			m_ProgressDlg->DestroyWindow();
			delete m_ProgressDlg;
		}
#endif

		AfxGetApp()->EndWaitCursor();

		return(retcode);
	}
	else
		return(FALSE);
}

unsigned char* CImage::PCXReadNextLine(PCX_HEADER *hdr, unsigned char *line, FILE *hFile)
{
	static int c, len;
	static char* dp;
	static WORD linesize;
	static WORD i;
	static WORD j;

	linesize = hdr->nplanes * hdr->bpl;
	dp = (char*) line;
	for (i = 0; i < linesize; ) {
		if ((c = fgetc(hFile)) == EOF)
			return(NULL);

		if ((c & PCX_COMPRESSED) == PCX_COMPRESSED) {
			len = (c & PCX_MASK);

			if ((c = fgetc(hFile)) == EOF)
				return(NULL);

			for (j = 0; j < len; j ++)
				*dp++ = (char) c;

			i += len;
		}
		else {
			*dp++ = (char) c;
			i ++;
		}
	}
	return(line);
}

int CImage::PBMReadInteger(FILE *infile)
{
	// Read an unsigned decimal integer from the PPM file
	// Swallows one trailing character after the integer
	// Note that on a 16-bit-int machine, only values up to 64k can be read.
	// This should not be a problem in practice.
	// taken from IJG JPEG library...needed because we don't know the structure of the header (CR/LF problem)

	register int ch;
	register int val;

	// Skip any leading whitespace
	val = 0;
	do {
		ch = fgetc(infile);
		if (ch == EOF) {
			val = -1;
			break;
		}
	} while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');

	if (ch < '0' || ch > '9')
		val = -1;

	if (!val) {
		val = ch - '0';
		while ((ch = fgetc(infile)) >= '0' && ch <= '9') {
			val *= 10;
			val += ch - '0';
		}
	}
	return val;
}

int CImage::TIFFCheckCMap(int n, uint16* r, uint16* g, uint16* b)
{
	while (n-- > 0)
		if (*r++ >= 256 || *g++ >= 256 || *b++ >= 256)
		return (16);

	return (8);
}

void CImage::SetupProgressDlg(int TotalCount)
{
#ifdef 	USEPROGRESS
	if (m_ShowProgress) {
		m_ProgressDlg->m_TotalCount = TotalCount;
		m_ProgressDlg->ShowWindow(SW_SHOW);
	}
#endif
}

void CImage::UpdateProgressDlg(int CurrentCount)
{
#ifdef 	USEPROGRESS
	if (m_ShowProgress) {
		m_ProgressDlg->UpdateProgress(CurrentCount);
	}
#endif
}

BOOL CImage::IsValid()
{
	if (CDib::m_lpBMIH == NULL || m_lpImage == NULL)		// added second condition 1/26/2005
		return(FALSE);

	return(TRUE);
}

BOOL CImage::IsHeaderValid()
{
	if (CDib::m_lpBMIH == NULL)
		return(FALSE);

	return(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CImageProgress dialog


#ifdef 	USEPROGRESS
CImageProgress::CImageProgress(CWnd* pParent /*=NULL*/)
	: CDialog(CImageProgress::IDD, pParent)
{
	//{{AFX_DATA_INIT(CImageProgress)
	m_FileName = _T("");
	m_FileFormat = _T("");
	//}}AFX_DATA_INIT

	m_TotalCount = 100;

	Create(IDD, pParent);
}


void CImageProgress::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CImageProgress)
	DDX_Text(pDX, IDC_PROGRESS_FILENAME, m_FileName);
	DDX_Text(pDX, IDC_PROGRESS_FILEFORMAT, m_FileFormat);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CImageProgress, CDialog)
	//{{AFX_MSG_MAP(CImageProgress)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CImageProgress message handlers

BOOL CImageProgress::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CProgressCtrl* prog = (CProgressCtrl*) GetDlgItem(IDC_PROGRESS_BAR);
	prog->SetRange32(0, m_TotalCount);
	prog->SetPos(0);
	
	return TRUE;
}

void CImageProgress::UpdateProgress(int CurrentCount)
{
	CProgressCtrl* prog = (CProgressCtrl*) GetDlgItem(IDC_PROGRESS_BAR);
	prog->SetPos(CurrentCount);

	prog->Invalidate(FALSE);
}
#endif

BOOL CImage::CheckFileFormat(LPCTSTR szFileName)
{
	FILE *f;
	char tbuf[10];

	// look for magic numbers
	f = fopen(szFileName, "rb");
	if (f != (FILE *) NULL) {
		// read head of file
		fread(tbuf, sizeof(char), 10, f);
		fclose(f);

		// look for magic numbers
		if (tbuf[0] == 0x0a)
			return(TRUE);

		if (tbuf[0] == 0x42 && tbuf[1] == 0x4d)
			return(TRUE);

		if (tbuf[0] == 0xff && tbuf[1] == 0xd8)
			return(TRUE);

		if (tbuf[0] != 'P' || tbuf[1] != '6')
			return(TRUE);
	}
	return(FALSE);
}

void CImage::CreateImage(CSize size, int nBitCount)
{
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

	// set up palette if monochrome bitmap, 4 bits/pixel, 8 bits/pixel
	LPRGBQUAD pDibQuad = (LPRGBQUAD) m_lpvColorTable;
	if (m_nColorTableEntries == 2) {
		pDibQuad[0].rgbRed = 0;
		pDibQuad[0].rgbGreen = 0;
		pDibQuad[0].rgbBlue = 0;
		pDibQuad[1].rgbRed = 255;
		pDibQuad[1].rgbGreen = 255;
		pDibQuad[1].rgbBlue = 255;
	}
	else if (m_nColorTableEntries == 16) {
		// create standard DOS color palette
		pDibQuad[0].rgbRed = 0;
		pDibQuad[0].rgbGreen = 0;
		pDibQuad[0].rgbBlue = 0;
		pDibQuad[1].rgbRed = 0;
		pDibQuad[1].rgbGreen = 0;
		pDibQuad[1].rgbBlue = 128;
		pDibQuad[2].rgbRed = 0;
		pDibQuad[2].rgbGreen = 128;
		pDibQuad[2].rgbBlue = 0;
		pDibQuad[3].rgbRed = 0;
		pDibQuad[3].rgbGreen = 128;
		pDibQuad[3].rgbBlue = 128;
		pDibQuad[4].rgbRed = 128;
		pDibQuad[4].rgbGreen = 0;
		pDibQuad[4].rgbBlue = 0;
		pDibQuad[5].rgbRed = 128;
		pDibQuad[5].rgbGreen = 0;
		pDibQuad[5].rgbBlue = 128;
		pDibQuad[6].rgbRed = 128;
		pDibQuad[6].rgbGreen = 64;
		pDibQuad[6].rgbBlue = 0;
		pDibQuad[7].rgbRed = 192;
		pDibQuad[7].rgbGreen = 192;
		pDibQuad[7].rgbBlue = 192;
		pDibQuad[8].rgbRed = 128;
		pDibQuad[8].rgbGreen = 128;
		pDibQuad[8].rgbBlue = 128;
		pDibQuad[9].rgbRed = 0;
		pDibQuad[9].rgbGreen = 0;
		pDibQuad[9].rgbBlue = 255;
		pDibQuad[10].rgbRed = 0;
		pDibQuad[10].rgbGreen = 255;
		pDibQuad[10].rgbBlue = 0;
		pDibQuad[11].rgbRed = 0;
		pDibQuad[11].rgbGreen = 255;
		pDibQuad[11].rgbBlue = 255;
		pDibQuad[12].rgbRed = 255;
		pDibQuad[12].rgbGreen = 0;
		pDibQuad[12].rgbBlue = 0;
		pDibQuad[13].rgbRed = 255;
		pDibQuad[13].rgbGreen = 0;
		pDibQuad[13].rgbBlue = 255;
		pDibQuad[14].rgbRed = 255;
		pDibQuad[14].rgbGreen = 255;
		pDibQuad[14].rgbBlue = 0;
		pDibQuad[15].rgbRed = 255;
		pDibQuad[15].rgbGreen = 255;
		pDibQuad[15].rgbBlue = 255;
	}
	else if (m_nColorTableEntries == 256) {
		// create grayscale palette
		for (int i = 0; i < 256; i ++) {
			pDibQuad[i].rgbRed = i;
			pDibQuad[i].rgbGreen = i;
			pDibQuad[i].rgbBlue = i;
		}
	}
	
	m_lpImage = NULL;  // no data yet

	CreateSection();
}

void CImage::AdjustDisplayArea(CPoint &Origin, CSize &DisplaySize)
{
	Origin.x = 0;
	Origin.y = 0;
	CSize imgsize = GetDimensions();
	double imgaspect = (double) imgsize.cx / (double) imgsize.cy;
	double winaspect = (double) DisplaySize.cx / (double) DisplaySize.cy;
	if (imgaspect > winaspect) {
		imgsize.cy = (int) ((double) DisplaySize.cx / imgaspect);
		imgsize.cx = DisplaySize.cx;
		Origin.y = (DisplaySize.cy - imgsize.cy) / 2;
	}
	else {
		imgsize.cy = DisplaySize.cy;
		imgsize.cx = (int) ((double) DisplaySize.cy * imgaspect);
		Origin.x = (DisplaySize.cx - imgsize.cx) / 2;
	}
	DisplaySize = imgsize;
}

BOOL CImage::WriteImage(LPCTSTR szFileName, int Format, int Quality)
{
	BOOL RetCode = FALSE;
	CFile file;
	FILE* outfile;

	// save new image
	switch (Format) {
		case BMP_FORMAT:
		default:
			if (file.Open(szFileName, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary)) {
				RetCode = Write(&file);
				file.Close();
			}
			break;
		case PCX_FORMAT:
		case TIFF_FORMAT:
		case PPM_FORMAT:
			RetCode = FALSE;
			break;
		case JPEG_FORMAT:
			// only support 24 bit images
			if (m_lpBMIH->biBitCount == 24) {
				DWORD linewidth;
				unsigned char* offBits;

				// open file
				outfile = fopen(szFileName, "wb");
				if (outfile) {
					struct jpeg_compress_struct cinfo;
					struct jpeg_error_mgr jerr;

					// Initialize the JPEG decompression object with default error handling
					cinfo.err = jpeg_std_error(&jerr);
					jpeg_create_compress(&cinfo);

					// set image parameters
					CSize sz = GetDimensions();
					cinfo.image_width = sz.cx; 	/* image width and height, in pixels */
					cinfo.image_height = sz.cy;
					cinfo.input_components = 3;	/* # of color components per pixel */
					cinfo.in_color_space = JCS_RGB; /* colorspace of input image */

					jpeg_set_defaults(&cinfo);
					jpeg_set_quality(&cinfo, Quality, TRUE);

					// Specify data source for decompression
					jpeg_stdio_dest(&cinfo, outfile);

					// start compression engine
					jpeg_start_compress(&cinfo, TRUE);

					// compute width of 1 scan line
					linewidth = (DWORD) ((((m_lpBMIH->biWidth * (DWORD) m_lpBMIH->biBitCount) + 31) & ~31) >> 3);

					// offset to the bits from start of DIB header
					offBits = m_lpImage;

					// set offset to end last row of pixel values since bitmaps start at the bottom
					offBits += (linewidth) * ((DWORD) m_lpBMIH->biHeight - 1);

					// write scanlines
					while (cinfo.next_scanline < cinfo.image_height) {
						jpeg_write_scanlines(&cinfo, &offBits, 1);

						offBits -= linewidth;
					}

					// finish compression
					jpeg_finish_compress(&cinfo);

					fclose(outfile);

					// clean up
					jpeg_destroy_compress(&cinfo);

					RetCode = TRUE;
				}
			}
			break;
	}
	return(RetCode);
}
