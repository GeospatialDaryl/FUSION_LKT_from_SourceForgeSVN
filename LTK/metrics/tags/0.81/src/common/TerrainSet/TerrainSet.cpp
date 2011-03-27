// TerrainSet.cpp: implementation of the CTerrainSet class.
//
//////////////////////////////////////////////////////////////////////
//
// 4/15/2010
//	Improved logic so that we can build an internal model using several individual DTM files and use the model to interpolate values
//	instead of loading several models into memory. This cuts down the amount of memory needed when working with small areas (compared
//	to the size of individual DTM files). This also lets us compute the amount of memory needed to provide models for a specific
//	extent to help when processing large data blocks.
//
// 8/18/2010
//	Further improved logic when creating internal models to only create a new model in memory when the requested area is more than 25%
//	smaller that the area loaded in memory or if a model's elevations could not be loaded into memory
//
// 11/5/2010
//	Added GetFirstModelColumnSpacing() and GetFirstModelPointSpacing() functions to return cell size information for first model. Changed
//	several variables to remove references to "ground" since this class is used for generic DTM handling. Improved the InterpolateElev()
//	function to remember the last model used for a point and try it first for a new location. This dramatically improves performance when
//	interpolating a new grid or other "ordered" set of points. May slow things down when working with random point locations.
//

#include "stdafx.h"
#include <math.h>
#include "TerrainSet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
//#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTerrainSet::CTerrainSet()
{
	Initialize();
}

CTerrainSet::CTerrainSet(LPCTSTR FileSpec)
{
	SetFileSpec(FileSpec);
	m_LastModelForInterpolation = -1;
}

CTerrainSet::~CTerrainSet()
{
	Destroy();
	m_LastModelForInterpolation = -1;
}

int CTerrainSet::GetError()
{
	return(m_ErrorCode);
}

void CTerrainSet::Initialize()
{
	m_FileCount = 0;
	m_FileSpec.Empty();
	m_ModelHeaderList = NULL;
	m_ModelElevLoaded = NULL;
	m_ModelUsedForInternalModel = NULL;

	m_OverviewValid = FALSE;
	m_HaveInternalModel = FALSE;
}

void CTerrainSet::SetFileSpec(LPCTSTR FileSpec)
{
	Initialize();
	m_FileSpec = FileSpec;
	CreateOverview();
}

int CTerrainSet::CountFilesMatchingSpec(LPCSTR FileSpec)
{
	if (FileSpec[0] == '\0')
		return(0);

	CFileFind finder;
	int FileCount = 0;
	BOOL bWorking = finder.FindFile(FileSpec);
	while (bWorking) {
		bWorking = finder.FindNextFile();
		FileCount ++;
	}

	return(FileCount);
}

int CTerrainSet::ExpandFileSpecToList(CString FileSpec)
{
	int Count = CountFilesMatchingSpec(FileSpec);
	CString csTemp;

	if (Count) {
		m_FileList.RemoveAll();
		if (Count > 1 || (Count == 1 && FileSpec.FindOneOf("*?") >= 0)) {
			// multiple files or a single match to a wild card specifier
			CFileFind finder;
			BOOL bWorking = finder.FindFile(FileSpec);
			while (bWorking) {
				bWorking = finder.FindNextFile();
				m_FileList.Add(finder.GetFilePath());
			}
		}
		else {		// only 1 file matched FileSpec and no wildcard...might be list file or single model
			// check for text file list...must have ".txt" extension
			csTemp = FileSpec;
			csTemp.MakeLower();
			if (csTemp.Find(".txt") >= 0) {
				// open text file and read file name
				CDataFile lst(FileSpec);
				if (lst.IsValid()) {
					char buf[1024];
					Count = 0;
					while (lst.NewReadASCIILine(buf)) {
						m_FileList.Add(buf);
						Count ++;
					}
				}
				else
					Count = 0;
			}
			else {
				// only 1 file directly specified (no wild card characters)
				m_FileList.Add(FileSpec);
			}
		}
	}

	return(Count);
}


void CTerrainSet::Destroy()
{
	if (m_OverviewValid)
		DestroyOverview();

	if (m_HaveInternalModel) {
		m_InternalModel.Destroy();
		m_HaveInternalModel = FALSE;
	}

	Initialize();
}

BOOL CTerrainSet::CreateOverview()
{
	int i;

	// get rid of existing info...if any
	DestroyOverview();

	// look at models
	if (!m_FileSpec.IsEmpty()) {
		m_FileCount = ExpandFileSpecToList(m_FileSpec);
		if (m_FileCount) {
			m_ModelHeaderList = new PlansDTM[m_FileCount];
			if (m_ModelHeaderList) {
				m_ModelElevLoaded = new int[m_FileCount];
				m_ModelUsedForInternalModel = new int[m_FileCount];

				// read all model headers...DTMs will be set up for patch access but file is not left open
				for (i = 0; i < m_FileCount; i ++) {
					m_ModelHeaderList[i].LoadElevations(m_FileList[i]);		// default is patch access

					// set flag to FALSE...no elevation loaded
					m_ModelElevLoaded[i] = FALSE;

					// clear flag indicating model was not used for internal model
					m_ModelUsedForInternalModel[i] = FALSE;
				}
				m_OverviewValid = TRUE;
			}
		}
		else {
			// no files
			m_OverviewValid = FALSE;
		}
	}
	else {
		// no files
		m_OverviewValid = FALSE;
	}

	m_LastModelForInterpolation = -1;

	return(m_OverviewValid);
}

void CTerrainSet::DestroyOverview()
{
	if (m_OverviewValid && m_ModelHeaderList) {
		// unload data
		for (int i = 0; i < m_FileCount; i ++) {
			m_ModelHeaderList[i].Destroy();
		}

		// delete list
		delete [] m_ModelHeaderList;
		m_ModelHeaderList = NULL;

		// delete flags for loaded elevations
		delete [] m_ModelElevLoaded;
		m_ModelElevLoaded = NULL;

		delete [] m_ModelUsedForInternalModel;
		m_ModelUsedForInternalModel = NULL;
	}

	if (m_HaveInternalModel) {
		m_InternalModel.Destroy();
		m_HaveInternalModel = FALSE;
	}

	m_OverviewValid = FALSE;
}

BOOL CTerrainSet::RectanglesIntersect(double MinX1, double MinY1, double MaxX1, double MaxY1, double MinX2, double MinY2, double MaxX2, double MaxY2)
{
	// test to see if the rectangles defined by the corners overlap
	if (MinX1 > MaxX2)
		return(FALSE);
	if (MaxX1 < MinX2)
		return(FALSE);
	if (MinY1 > MaxY2)
		return(FALSE);
	if (MaxY1 < MinY2)
		return(FALSE);

	return(TRUE);
}

void CTerrainSet::PreLoadData(double MinX, double MinY, double MaxX, double MaxY, BOOL CreateInternalModel)
{
	if (m_OverviewValid) {
		// if coords are all -1.0, load all models
		// no chance to use internal model because we don't have a valid extent
		if (MinX == -1.0 && MinY == -1.0 && MaxX == -1.0 && MaxY == -1.0) {
			for (int i = 0; i < m_FileCount; i ++) {
				// clear flag indicating elevations are loaded
				m_ModelElevLoaded[i] = FALSE;
				m_ModelUsedForInternalModel[i] = FALSE;

				// try to load elevation data
				m_ModelHeaderList[i].LoadElevations(m_FileList[i], TRUE, FALSE);
				if (!m_ModelHeaderList[i].IsValid()) {
					// set up for patch access
					m_ModelHeaderList[i].LoadElevations(m_FileList[i]);
				}
				else
					m_ModelElevLoaded[i] = TRUE;
			}
		}
		else {
			// see if we are creating an internal model
			if (CreateInternalModel && m_FileCount) {
				int i;

				// compute the area of the requested extent and the area of the loaded models
				double LoadedMinX, LoadedMaxX, LoadedMinY, LoadedMaxY;
				double LoadedArea, RequestedArea;
				GetOverallExtent(LoadedMinX, LoadedMaxX, LoadedMinY, LoadedMaxY);
				LoadedArea = (LoadedMaxX - LoadedMinX) * (LoadedMaxY - LoadedMinY);
				RequestedArea = (MaxX - MinX) * (MaxY - MinY);

				// count number of models in memory
				int LoadedModels = 0;
				for (i = 0; i < m_FileCount; i ++) {
					if (AreModelElevationsInMemory(i))
						LoadedModels ++;
				}

				// only create the internal model if the requested area is more than 25% smaller than the loaded area
				// or some models are not in memory
				if (RequestedArea < LoadedArea * 0.75 || LoadedModels < m_FileCount) {
					if (m_HaveInternalModel) {
						m_InternalModel.Destroy();
						m_HaveInternalModel = FALSE;
					}

					// computer modified extent and number of rows/cols for internal model
					double IM_OriginX, IM_OriginY;
					double IM_MaxX, IM_MaxY;
					double IM_CellWidth, IM_CellHeight;
					double X, Y, Elev;
					long IM_Columns, IM_Points;

					// compute model corners...add 1 cell to make sure there aren't any edge problems
					IM_CellWidth = m_ModelHeaderList[0].ColumnSpacing();
					IM_CellHeight = m_ModelHeaderList[0].PointSpacing();
					IM_OriginX = MinX - m_ModelHeaderList[0].ColumnSpacing();
					IM_OriginY = MinY - m_ModelHeaderList[0].PointSpacing();
					IM_MaxX = MaxX + m_ModelHeaderList[0].ColumnSpacing();
					IM_MaxY = MaxY + m_ModelHeaderList[0].PointSpacing();

					// adjust origina to be a multiple of the cell size
					if (fmod(IM_OriginX, IM_CellWidth) > 0.0) {
						IM_OriginX = IM_OriginX - fmod(IM_OriginX, IM_CellWidth);
					}
					if (fmod(IM_OriginY, IM_CellHeight) > 0.0) {
						IM_OriginY = IM_OriginY - fmod(IM_OriginY, IM_CellHeight);
					}
					if (fmod(IM_MaxX, IM_CellWidth) > 0.0) {
						IM_MaxX = IM_MaxX + (IM_CellWidth - fmod(IM_MaxX, IM_CellWidth));
					}
					if (fmod(IM_MaxY, IM_CellHeight) > 0.0) {
						IM_MaxY = IM_MaxY + (IM_CellHeight - fmod(IM_MaxY, IM_CellHeight));
					}

					// compute the number of rows/cols in the internal model
					IM_Columns = (int) ((IM_MaxX - IM_OriginX + IM_CellWidth / 2.0) / IM_CellWidth) + 1;
					IM_Points = (int) ((IM_MaxY - IM_OriginY + IM_CellHeight / 2.0) / IM_CellHeight) + 1;

					// create the model and allocate memory for elevation data
					m_InternalModel.CreateNewModel(IM_OriginX, IM_OriginY, IM_Columns, IM_Points, IM_CellWidth, IM_CellHeight, m_ModelHeaderList[0].GetXYUnits(), m_ModelHeaderList[0].GetElevationUnits(), m_ModelHeaderList[0].GetZBytes(), m_ModelHeaderList[0].CoordinateSystem(), m_ModelHeaderList[0].CoordinateZone(), m_ModelHeaderList[0].HorizontalDatum(), m_ModelHeaderList[0].VerticalDatum());

					// loop through models in list and interpolate values for internal model...do this 1 model at a time to minimize memory use
					int j, k;
					for (int i = 0; i < m_FileCount; i ++) {
						// clear flag indicating elevations are loaded
						m_ModelElevLoaded[i] = FALSE;
						m_ModelUsedForInternalModel[i] = FALSE;

						// see if the model overlaps area of interest
						if (RectanglesIntersect(MinX, MinY, MaxX, MaxY, m_ModelHeaderList[i].OriginX(), m_ModelHeaderList[i].OriginY(), m_ModelHeaderList[i].OriginX() + m_ModelHeaderList[i].Width(), m_ModelHeaderList[i].OriginY() + m_ModelHeaderList[i].Height())) {
							// try to load elevation data
							m_ModelHeaderList[i].LoadElevations(m_FileList[i], TRUE, FALSE);
							if (!m_ModelHeaderList[i].IsValid()) {
								// set up for patch access
								m_ModelHeaderList[i].LoadElevations(m_FileList[i]);
							}
							else
								m_ModelElevLoaded[i] = TRUE;

							m_ModelUsedForInternalModel[i] = TRUE;
						}

						// go through internal model and interpolate values for all cells using loaded model
						X = IM_OriginX;
						for (j = 0; j < IM_Columns; j ++) {
							Y = IM_OriginY;
							for (k = 0; k < IM_Points; k ++) {
								if (m_InternalModel.GetGridElevation(j, k) < 0.0) {
									Elev = m_ModelHeaderList[i].InterpolateElev(X, Y);
									m_InternalModel.SetInternalElevationValue(j, k, Elev);
								}
								Y += m_InternalModel.PointSpacing();
							}
							X += m_InternalModel.ColumnSpacing();
						}

						// unload model...set up for patch access
						m_ModelHeaderList[i].LoadElevations(m_FileList[i]);
						m_ModelElevLoaded[i] = FALSE;
					}

					m_HaveInternalModel = TRUE;
//					m_InternalModel.WriteModel("internalDTM.dtm");
				}
			}
			else {
				for (int i = 0; i < m_FileCount; i ++) {
					// clear flag indicating elevations are loaded
					m_ModelElevLoaded[i] = FALSE;
					m_ModelUsedForInternalModel[i] = FALSE;

					// see if the model overlaps area of interest
					if (RectanglesIntersect(MinX, MinY, MaxX, MaxY, m_ModelHeaderList[i].OriginX(), m_ModelHeaderList[i].OriginY(), m_ModelHeaderList[i].OriginX() + m_ModelHeaderList[i].Width(), m_ModelHeaderList[i].OriginY() + m_ModelHeaderList[i].Height())) {
						// try to load elevation data
						m_ModelHeaderList[i].LoadElevations(m_FileList[i], TRUE, FALSE);
						if (!m_ModelHeaderList[i].IsValid()) {
							// set up for patch access
							m_ModelHeaderList[i].LoadElevations(m_FileList[i]);
						}
						else
							m_ModelElevLoaded[i] = TRUE;
					}
				}
			}
		}
	}
}

void CTerrainSet::UnloadData()
{
	if (m_OverviewValid) {
		for (int i = 0; i < m_FileCount; i ++) {
			// set up for patch access
			m_ModelHeaderList[i].LoadElevations(m_FileList[i]);

			// clear flag indicating elevations are loaded
			m_ModelElevLoaded[i] = FALSE;
		}
	}
}

double CTerrainSet::InterpolateElev(double X, double Y)
{
	double elev = -1.0;

	if (m_OverviewValid) {
		if (m_HaveInternalModel) {
			elev = m_InternalModel.InterpolateElev(X, Y);
		}
		else {
			if (m_LastModelForInterpolation >= 0) {
				elev = m_ModelHeaderList[m_LastModelForInterpolation].InterpolateElev(X, Y);
				if (elev >= 0.0)
					return(elev);
			}

			// now try with all models
			for (int i = 0; i < m_FileCount; i ++) {
				elev = m_ModelHeaderList[i].InterpolateElev(X, Y);
				if (elev >= 0.0) {
					m_LastModelForInterpolation = i;
					break;
				}
			}
		}
	}
	return(elev);
}


BOOL CTerrainSet::IsValid()
{
	return(m_OverviewValid);
}

int CTerrainSet::GetModelCount()
{
	if (m_OverviewValid)
		return(m_FileCount);

	return(0);
}

CString CTerrainSet::GetModelFileName(int index)
{
	CString csTemp;
	csTemp.Empty();

	if (m_OverviewValid && index < m_FileCount && index >= 0)
		csTemp = m_FileList[index];

	return(csTemp);
}

BOOL CTerrainSet::AreModelElevationsInMemory(int index)
{
	if (m_OverviewValid && index < m_FileCount && index >= 0)
		return(m_ModelElevLoaded[index]);

	return(FALSE);
}

BOOL CTerrainSet::GetOverallExtent(double &MinX, double &MinY, double &MaxX, double &MaxY)
{
	if (m_OverviewValid && m_FileCount > 0) {
		// intialize to etent of first model
		MinX = m_ModelHeaderList[0].OriginX();
		MinY = m_ModelHeaderList[0].OriginY();
		MaxX = m_ModelHeaderList[0].OriginX() + m_ModelHeaderList[0].Width();
		MaxY = m_ModelHeaderList[0].OriginY() + m_ModelHeaderList[0].Height();

		// go through models and get overall extent
		for (int i = 1; i < m_FileCount; i ++) {
			MinX = min(MinX, m_ModelHeaderList[i].OriginX());
			MinY = min(MinY, m_ModelHeaderList[i].OriginY());
			MaxX = max(MaxX, m_ModelHeaderList[i].OriginX() + m_ModelHeaderList[i].Width());
			MaxY = max(MaxY, m_ModelHeaderList[i].OriginY() + m_ModelHeaderList[i].Height());
		}

		return(TRUE);
	}
	return(FALSE);
}

BOOL CTerrainSet::AreModelsInteger()
{
	BOOL retcode = TRUE;

	if (m_OverviewValid && m_FileCount > 0) {
		for (int i = 0; i < m_FileCount; i ++) {
			if (m_ModelHeaderList[i].GetZBytes() >= 2) {
				retcode = FALSE;
				break;
			}
		}
	}
	return(retcode);
}

BOOL CTerrainSet::InternalModelIsValid()
{
	return (m_InternalModel.IsValid());
}

CString CTerrainSet::InternalModelDescription()
{
	CString csTemp;
	csTemp = _T("Internal model is invalid");

	if (InternalModelIsValid()) {
		csTemp.Format("Origin (%.2lf,%.2lf) Cols: %li Rows: %li Cellwidth: %.2lf CellHeight: %.2lf", m_InternalModel.OriginX(), m_InternalModel.OriginY(), m_InternalModel.Columns(), m_InternalModel.Rows(), m_InternalModel.ColumnSpacing(), m_InternalModel.PointSpacing());
	}

	return(csTemp);
}

BOOL CTerrainSet::WasModelUsedForInternalModel(int index)
{
	if (InternalModelIsValid() && m_OverviewValid && index < m_FileCount && index >= 0)
		return(m_ModelUsedForInternalModel[index]);

	return(FALSE);
}

double CTerrainSet::GetFirstModelColumnSpacing()
{
	if (m_OverviewValid && m_FileCount > 0) {
		return(m_ModelHeaderList[0].ColumnSpacing());
	}
	return(-1.0);
}

double CTerrainSet::GetFirstModelPointSpacing()
{
	if (m_OverviewValid && m_FileCount > 0) {
		return(m_ModelHeaderList[0].PointSpacing());
	}
	return(-1.0);
}
