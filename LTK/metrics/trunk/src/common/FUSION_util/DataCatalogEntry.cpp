// DataCatalogEntry.cpp: implementation of the CDataCatalogEntry class.
//
//////////////////////////////////////////////////////////////////////

//#include "clipdata.h"
#include "DataCatalogEntry.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDataCatalogEntry::CDataCatalogEntry()
{

}

CDataCatalogEntry::~CDataCatalogEntry()
{

}

CDataCatalogEntry& CDataCatalogEntry::operator=(CDataCatalogEntry& src)
{
	m_FileName = src.m_FileName;
	m_CheckSum = src.m_CheckSum;
	m_MinX = src.m_MinX;
	m_MaxX = src.m_MaxX;
	m_MinY = src.m_MinY;
	m_MaxY = src.m_MaxY;
	m_MinZ = src.m_MinZ;
	m_MaxZ = src.m_MaxZ;
	m_PointDensity = src.m_PointDensity;
	m_Points = src.m_Points;

	return *this;
}
