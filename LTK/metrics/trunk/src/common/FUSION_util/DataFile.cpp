// DataFile.cpp: implementation of the CDataFile class.
//
//////////////////////////////////////////////////////////////////////
//
// 3/4/2008 changed logic in SetTerminatorLength() to look for CR and LF only. there were problems
// determining the length of the terminator when lines contained tabs (also a control character...9)
//
#include "DataFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDataFile::CDataFile()
{
	m_FileHandle = NULL;
	m_TerminatorLength = 2;
	m_FileName.Empty();
}

CDataFile::CDataFile(LPCTSTR FileName)
{
	m_FileHandle = NULL;
	m_TerminatorLength = 2;
	m_FileName.Empty();

	Open(FileName);
}

CDataFile::~CDataFile()
{
	Close();
}

BOOL CDataFile::ReadASCIILine(char *buf, BOOL SkipBlanks)
{
	// read line from text file opend in BINARY mode
	// deals with UNIX/DOS end-of-line problems
	static int error;
	static int c;
	static int i;

	buf[0] = '\0';
	c = 0;
	error = 0;
	while (1) {
		// reads to first occurance of \r-carriage_return or \n-linefeed...but does not read the \r or \n character itself
		error = fscanf(m_FileHandle, "%[^\r\n]", buf);
		if (error) {
			// a line was read or EOF occurred
			m_CurrentLineNumber ++;

			break;
		}

		if (buf[0] == '\0' && !SkipBlanks)
			break;
	}

	// check for end of file or other error
	if (error == EOF || buf[0] == 26)
		return(FALSE);

	//read the control characters at end of line
	for (i = 0; i < m_TerminatorLength; i ++) {
		c = fgetc(m_FileHandle);
		if (c == EOF)			// next read will fail due to EOF
			break;
	}

	return(TRUE);
}

BOOL CDataFile::NewReadASCIILine(char *buf, BOOL SkipBlanks)
{
	// read line from text file opend in BINARY mode
	// deals with UNIX/DOS end-of-line problems

	// *************************
	// does not work with blank lines in data file and SkipBlanks=FALSE
	// misses the blank line
	//
	int error = 0;
	int c = 0;

	buf[0] = '\0';
	while (1) {
		// reads to first occurance of \r-carriage_return or \n-linefeed...but does not read the \r or \n character itself
		error = fscanf(m_FileHandle, "%[^\r\n]", buf);
		if (error) {
			// a line was read or EOF occurred
			m_CurrentLineNumber ++;

			break;
		}
		else {
			// no fields were assigned...could be a blank line
			// read next character from file and see if it is \r or \n or EOF
			c = fgetc(m_FileHandle);
			if (c != '\r' && c != '\n' && c != EOF)
				fseek(m_FileHandle, -1, SEEK_CUR);
		}
		if (buf[0] == '\0' && !SkipBlanks)
			break;
	}

	// check for end of file or other error
	if (error == EOF || c == EOF || buf[0] == 26)
		return(FALSE);

	//read the linefeed
	c = fgetc(m_FileHandle);
	if (c != '\r' && c != '\n' && c != EOF)
		fseek(m_FileHandle, -1, SEEK_CUR);

	return(TRUE);
}

BOOL CDataFile::ReadDataLine(char *buf, LPCTSTR flagchr, BOOL SkipBlanks)
{
	// added loop exit when we have a blank line and we wantit to get back to calling process
	BOOL flag;
	do {
		flag = ReadASCIILine(buf, SkipBlanks);
		if (flag && buf[0] == '\0' && !SkipBlanks)
			break;
	}
	while (strchr(flagchr, buf[0]) && flag);

	return(flag);
}


BOOL CDataFile::Open(LPCTSTR FileName)
{
	m_CurrentLineNumber = -1;
	m_FileHandle = NULL;


	m_FileHandle = fopen(FileName, READ);
	if (m_FileHandle) {
		m_FileName = FileName;
		SetTerminatorLength();
	}

	return(m_FileHandle != NULL);
}

BOOL CDataFile::IsValid()
{
	return(m_FileHandle != NULL);
}

void CDataFile::Close()
{
	if (m_FileHandle) {
		fclose(m_FileHandle);
		m_FileHandle = NULL;
	}
	m_FileName.Empty();
}


long CDataFile::GetPosition()
{
	if (m_FileHandle)
		return(ftell(m_FileHandle));

	return(-1);
}

BOOL CDataFile::SetPosition(long Offset, int FromWhere)
{
	m_CurrentLineNumber = -1;
	if (m_FileHandle)
		return(fseek(m_FileHandle, Offset, FromWhere) == 0);

	return(FALSE);
}

void CDataFile::Rewind()
{
	m_CurrentLineNumber = -1;

	if (m_FileHandle)
		rewind(m_FileHandle);
}

long CDataFile::GetFileSize()
{
	long length = 0;
	
	if (SetPosition(0l, SEEK_END))
		length = GetPosition();

	Rewind();

	return(length);
}


BOOL CDataFile::ReadAll(char *buf)
{
	if (m_FileHandle) {
		// read all of file...rewind will happen in GetFileSize()
		fread(buf, sizeof(char), GetFileSize(), m_FileHandle);

		Rewind();

		return(TRUE);
	}

	return(FALSE);
}

BOOL CDataFile::ReadRawBytes(char *buf, int cnt, BOOL LessIsOK)
{
	if (m_FileHandle) {
		unsigned int bytes = fread(buf, sizeof(char), cnt, m_FileHandle);
		if (bytes == (unsigned int) cnt || (bytes && LessIsOK))
			return(TRUE);
	}
	return(FALSE);
}

BOOL CDataFile::ReadValue(double *val)
{
	int error = fscanf(m_FileHandle, " %lf", val);
	if (error == 1)
		return(TRUE);

	return(FALSE);
}

void CDataFile::SetTerminatorLength()
{
	// read part of the file and look for sequences of control characters
	char buf[8192];
	int nchars = fread(buf, sizeof(char), 8191, m_FileHandle);
	int cnt = 0;
	int mincnt = 200000;
	int maxcnt = 0;
	BOOL InTerminator = FALSE;

	if (nchars) {
		for (int i = 0; i < nchars; i ++) {
//			if (buf[i] < 32) {
			if (buf[i] == 10 || buf[i] == 13) {
				if (InTerminator)
					cnt ++;
				else {
					InTerminator = TRUE;
					cnt = 1;
				}
			}
			else {
				if (InTerminator) {
					if (cnt > maxcnt)
						maxcnt = cnt;
					if (cnt < mincnt)
						mincnt = cnt;
				}
				InTerminator = FALSE;
			}
		}
		m_TerminatorLength = mincnt;
	}
	else
		m_TerminatorLength = 2;

	Rewind();
}
