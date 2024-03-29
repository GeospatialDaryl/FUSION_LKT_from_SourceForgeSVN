// LASFormatFile.h: interface for the CLASFormatFile class.
//
//////////////////////////////////////////////////////////////////////
//
// 1/17/2012
// modified the logic used to open LAS files so I can assign a larger buffer for reading point data. in use, the
// buffer is managed by the calling code. previous versions tried to set a larger buffer but the buffer was only 
// being set whe the file was opened to determine the file format and not when reading point data. The larger buffer
// doesn't seem to make much difference in the initial tests.
//

#if !defined(AFX_LASFORMATFILE_H__8A7C6B08_13EB_46AE_A854_B37E90F3FB09__INCLUDED_)
#define AFX_LASFORMATFILE_H__8A7C6B08_13EB_46AE_A854_B37E90F3FB09__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define		HEADERSIZE			227
#define		HEADERSIZEV12		227
#define		HEADERSIZEV13		235

// record types for CopyVariableRecord()
#define		ALLRECORDS			0x7FFF
#define		PROJECTIONRECORDS	0x01
#define		UNKNOWNRECORDTYPE	0x40

typedef  unsigned long ulong;
typedef  unsigned short ushort;
typedef  unsigned char uchar;
typedef	 unsigned char bitbyte;

typedef  struct {
	unsigned char	ClassNumber;					// value used in point records
	char			Description[16];				// description of class
} CLASSIFICATION;

typedef  struct {
	unsigned char	FileMarkerNumber;				// value used in point records
	char			FileName[257];					// description of flightline
} FLIGHTLINE;

typedef struct {
	ushort			FileSourceID;					// flight line number or other group indicator
	ushort			Reserved;						// extra bytes
} V11FILESOURCEID;

union FileSrcID {
	V11FILESOURCEID V11;							// version 1.1 file source ID and reserved block
	ulong			Reserved;						// original version 1.0 data block
};

class CLASPublicHeaderBlock
{
public:
	char			FileSignature[5];				// must be "LASF"...magic number like signature
	FileSrcID		Reserved;						// extra variable
	ulong			GUIDData1;						// Globally unique identifier
	ushort			GUIDData2;						// Globally unique identifier
	ushort			GUIDData3;						// Globally unique identifier
	uchar			GUIDData4[9];					// Globally unique identifier
	uchar			VersionMajor;					// left hand side of version number
	uchar			VersionMinor;					// right hand side of version number
	char			SystemIdentifier[33];			// LIDAR hardware identifier
	char			GeneratingSoftware[33];			// generating software identifier
	ushort			FlightDateJulian;				// julian day of the year the data was collected
	ushort			Year;							// year the data was collected
	ushort			HeaderSize;						// actual size of the header block
	ulong			OffsetToData;					// number of bytes from beginning of file to the actual point data
	ulong			NumberOfVariableLengthRecords;	// number of variable length records
	uchar			PointDataFormatID;				// format identifier for point data records
	ushort			PointDataRecordLength;			// length of each point record
	ulong			NumberOfPointRecords;			// total number of point records in file
	ulong			NumberOfPointsByReturn[5];		// array containing number of point records per return..first value is number of first returns
	double			XScaleFactor;					// x scale factor
	double			YScaleFactor;					// y scale factor
	double			ZScaleFactor;					// z scale factor
	double			XOffset;						// x offset
	double			YOffset;						// y offset
	double			ZOffset;						// z offset
	double			MaxX;							// maximum x value (after expansion using scale and offset)
	double			MinX;							// minimum x value (after expansion using scale and offset)
	double			MaxY;							// maximum y value (after expansion using scale and offset)
	double			MinY;							// minimum y value (after expansion using scale and offset)
	double			MaxZ;							// maximum z value (after expansion using scale and offset)
	double			MinZ;							// minimum z value (after expansion using scale and offset)
	__int64			WaveformStart;					// Start of waveform data packet record...version 1.3+ only

	float			Version;						// combination of VersionMajor.VersionMinor
	ushort			FileSourceID;					// version 1.1 flight line number of other grouping code
public:
	BOOL Write(FILE *FileHandle);
	BOOL Read(FILE* FileHandle);
	CLASPublicHeaderBlock();
	virtual ~CLASPublicHeaderBlock();
};

class CLASVariableLengthRecord
{
public:
	ushort			RecordSignature;				// record signature for variable length record...0xAABB
	char			UserID[17];						// ASCII user identifier for creator of variable length record
	ushort			RecordID;						// record identifier
	ushort			RecordLengthAfterHeader;		// number of bytes in record after the standard portion of th header
	char			Description[33];				// record description

public:
	BOOL Read(FILE* FileHandle);					// poorly named...actually only reads the header fields
	BOOL ReadHeader(FILE* FileHandle);
	BOOL WriteHeader(FILE *FileHandle);
	CLASVariableLengthRecord();
	virtual ~CLASVariableLengthRecord();
};

class CLASPointDataRecord
{
public:
	long			RawX;							// return position
	long			RawY;							// return position
	long			RawZ;							// return position
	double			X;								// return position scaled using header values
	double			Y;								// return position scaled using header values
	double			Z;								// return position scaled using header values
	ushort			Intensity;						// pulse return magnitude...system specific with unknown range
	bitbyte			ReturnBitInfo;					// bit field containing return number, # returns, scan direction flag and edge flag
	uchar			Classification;					// classification code for return...corresponds to variable length record in CLASClassificationLookupTable
	char			ScanAngleRank;					// pulse angle including aircraft roll...0 is NADIR (-) left (+) right
	uchar			FileMarker;						// file marker code...corresponds to variable length record in CLASFlightLineHeaderLookupTable
	ushort			UserBitField;					// user bit field...version 1.1+ this is point source ID and it corresponds to the File Source ID in the public header
													// for files containing multiple flightlines, this would presumably indicate the flight line for the point

	uchar			ReturnNumber;					// pulse return number for a given pulse
	uchar			NumberOfReturns;				// number of returns for the pulse
	uchar			ScanDirectionFlag;				// scan direction...1 indicates positive scan direction, 0 negative
	uchar			EdgeOfFlightLineFlag;			// flag indicating pulse is on edge of filght line...last pulse in scan line before changing direction

	// version 1.1+ classification code variables
	uchar			V11Classification;				// bits 0-4: classification code for return...corresponds to variable length record in CLASClassificationLookupTable
	uchar			V11Synthetic;					// bit 5: 0 if point is from scan data, 1 if point was created by another method
	uchar			V11KeyPoint;					// bit 6: 0 if point is not a model key-point, 1 otherwise...key-points should not be removed by processing operations
	uchar			V11Withheld;					// bit 7: 0 if point is valid, 1 if point should not be included in processing (synonymous with deleted)

public:
	virtual BOOL Write(FILE* FileHandle);
	BOOL WriteBaseValues(FILE* FileHandle);
	void ScaleX(double Offset, double Scale);
	void ScaleY(double Offset, double Scale);
	void ScaleZ(double Offset, double Scale);
	BOOL ReadBaseValues(FILE* FileHandle);
	CLASPointDataRecord();
	virtual ~CLASPointDataRecord();

	void UnpackReturnBitInfo(BOOL FixFormatError = FALSE);
	void PackReturnBitInfo();
};

class CLASPointDataRecordFormat1 : public CLASPointDataRecord
{
public:
	double			GPSTime;						// GPS time at which the point was acquired

public:
	BOOL WriteAdditionalValues(FILE* FileHandle);
	BOOL Write(FILE* FileHandle);
	BOOL ReadAdditionalValues(FILE* FileHandle);
	CLASPointDataRecordFormat1();
	virtual ~CLASPointDataRecordFormat1();
};

class CLASPointDataRecordFormatAll : public CLASPointDataRecord
{
public:
	double			GPSTime;						// GPS time at which the point was acquired
	ushort			Red;							// Red image channel value...always scaled to 16-bit value...divide by 256 to get 8-bit value
	ushort			Green;							// Green image channel value...always scaled to 16-bit value...divide by 256 to get 8-bit value
	ushort			Blue;							// Blue image channel value...always scaled to 16-bit value...divide by 256 to get 8-bit value

public:
	CLASPointDataRecordFormatAll& CLASPointDataRecordFormatAll::operator=(CLASPointDataRecordFormatAll& src);
	BOOL WriteAdditionalValues(FILE* FileHandle, int Format = 0);
	BOOL Write(FILE* FileHandle, int Format = 0);
	BOOL ReadAdditionalValues(FILE* FileHandle, int Format = 0);
	CLASPointDataRecordFormatAll();
	virtual ~CLASPointDataRecordFormatAll();
};

class CLASClassificationLookupTable : public CLASVariableLengthRecord
{
public:
	CLASSIFICATION Class[256];						// classification code definitions

public:
	CLASClassificationLookupTable();
	virtual ~CLASClassificationLookupTable();
};

class CLASFlightLineHeaderLookupTable : public CLASVariableLengthRecord
{
public:
	FLIGHTLINE		FlightLine[256];				// flight line info
public:
	CLASFlightLineHeaderLookupTable();
	virtual ~CLASFlightLineHeaderLookupTable();
};

class CLASHistogram : public CLASVariableLengthRecord
{
public:
	CLASHistogram();
	virtual ~CLASHistogram();
};

class CLASTextAreaDescription : public CLASVariableLengthRecord
{
public:
	CLASTextAreaDescription();
	virtual ~CLASTextAreaDescription();
};

// class to abstract LAS file...contains instances of most of the above objects
class CLASFormatFile  
{
public:
	CLASPublicHeaderBlock Header;
	CLASVariableLengthRecord VariableHeader;
//	CLASPointDataRecord PointRecordFormat0;
//	CLASPointDataRecordFormat1 PointRecordFormat1;
// 8/25/2010	CLASPointDataRecordFormat1 PointRecord;
	CLASPointDataRecordFormatAll PointRecord;
	CLASClassificationLookupTable ClassificationCodes;
	CLASFlightLineHeaderLookupTable FlightLineInfo;
	CLASTextAreaDescription FileDescription;
	FILE* m_FileHandle;

public:
	BOOL TestVariableRecordType(unsigned short RecordID, int ValidTypes);
	BOOL JumpToVariableRecord(int index = 0);
	int CopyVariableRecord(int index, FILE *FileHandle, int RecordTypes = ALLRECORDS);
	void Close();
	BOOL ReadNextPoint();
	BOOL IsValid();
	void JumpToPointRecord(int index = 0);
	BOOL Open(LPCTSTR FileName, char *buffer = NULL, int buffersize = 0);
	BOOL VerifyFormat(LPCTSTR FileName);
	CString m_FileName;
	CLASFormatFile();
	virtual ~CLASFormatFile();

private:
	int m_CurrentPointIndex;
	BOOL m_ValidHeader;
};

#endif // !defined(AFX_LASFORMATFILE_H__8A7C6B08_13EB_46AE_A854_B37E90F3FB09__INCLUDED_)
