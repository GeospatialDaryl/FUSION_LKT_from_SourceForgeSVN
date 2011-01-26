// Image.h: interface for the CImage class.
//
// changes:
//
//	9/26/2003	RJM
//
//		Fixed problems reading TIFF grayscale images...was not creating a palette to represent the image
//		symptom was that the image loaded but displayed as a solid light-gray box instead of the BW image
//
//		Fix involved better understanding and use of the PhotometricInterpretation tag since it tells us
//		that the image is grayscale (2 types with black-white and white-black palette).
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMAGE_H__2D066361_31A3_11D3_A124_080E08C10B02__INCLUDED_)
#define AFX_IMAGE_H__2D066361_31A3_11D3_A124_080E08C10B02__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cdib.h"
extern "C" {
//#include "..\common\image\jpeglib.h"
#include "..\..\common\jpeg-7\jpeglib.h"
#include "..\..\common\image\tiffio.h"
}

//#define		USEPROGRESS

#define	BMP_FORMAT			0
#define	PCX_FORMAT			1
#define	TIFF_FORMAT			2
#define	PPM_FORMAT			3
#define	JPEG_FORMAT			4

// PCX file related constants and header structure
#define	PCX_HEADER_MARKER 0x0a
#define	PCX_COMPRESSED	0xc0
#define	PCX_MASK			0x3f

typedef struct {
	BYTE mfg;
	BYTE version;
	BYTE encoding;
	BYTE bpp;
	WORD xmin;
	WORD ymin;
	WORD xmax;
	WORD ymax;
	WORD hres;
	WORD vres;
	BYTE palette[48];
	BYTE reserved;
	BYTE nplanes;
	WORD bpl;
	WORD pinfo;
	BYTE filler[58];
} PCX_HEADER;

/////////////////////////////////////////////////////////////////////////////
// CImageProgress dialog
#ifdef 	USEPROGRESS
class CImageProgress : public CDialog
{
// Construction
public:
	CImageProgress(CWnd* pParent = NULL);   // standard constructor
	void UpdateProgress(int CurrentCount);
	int m_TotalCount;

// Dialog Data
	//{{AFX_DATA(CImageProgress)
	enum { IDD = IDD_IMAGELOADPROGRESS };
	CString	m_FileName;
	CString	m_FileFormat;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImageProgress)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CImageProgress)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif // #ifdef 	USEPROGRESS

class CImage : public CDib  
{
public:
	BOOL m_BottomUp;
	BOOL m_ShowProgress;
	void AdjustDisplayArea(CPoint& Origin, CSize& DisplaySize);
	BOOL CheckFileFormat(LPCTSTR szFileName);
	BOOL IsValid();
	BOOL IsHeaderValid();
	void CreateImage(CSize size, int nBitCount);
	BOOL WriteImage(LPCTSTR szFileName, int Format = BMP_FORMAT, int Quality = 95);
	BOOL ReadImage(LPCTSTR szFileName, BOOL CreatePalette = FALSE, BOOL ShowProgress = FALSE, int ColorOption = 1);
	BOOL ReadImageHeader(LPCTSTR szFileName);
	BOOL ReadPPMImage(LPCTSTR szFileName);
	BOOL ReadTIFFImage(LPCTSTR szFileName, int ColorOption = 1);
	BOOL ReadPCXImage(LPCTSTR szFileName);
	BOOL ReadJPEGImage(LPCTSTR szFileName, BOOL CreatePalette);
	BOOL ReadBMPImage(LPCTSTR szFileName);
	BOOL ReadPPMImageHeader(LPCTSTR szFileName);
	BOOL ReadTIFFImageHeader(LPCTSTR szFileName);
	BOOL ReadPCXImageHeader(LPCTSTR szFileName);
	BOOL ReadJPEGImageHeader(LPCTSTR szFileName);
	BOOL ReadBMPImageHeader(LPCTSTR szFileName);
	int DetectImageFileFormat(LPCTSTR szFileName);
	CImage();
	virtual ~CImage();

private:
	void UpdateProgressDlg(int CurrentCount);
	void SetupProgressDlg(int TotalCount);
#ifdef USEPROGRESS
	CImageProgress* m_ProgressDlg;
#endif  // #ifdef USEPROGRESS
	int TIFFCheckCMap(int n, uint16* r, uint16* g, uint16* b);
	int PBMReadInteger(FILE* infile);
	unsigned char* PCXReadNextLine(PCX_HEADER* hdr, unsigned char* line, FILE* hFile);
};

#endif // !defined(AFX_IMAGE_H__2D066361_31A3_11D3_A124_080E08C10B02__INCLUDED_)
