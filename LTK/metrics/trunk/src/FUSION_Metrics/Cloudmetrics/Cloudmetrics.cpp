// cloudmetrics.cpp : Defines the entry point for the console application.
//
// Version info
//
// V1.3 Pre-January 2007 clean-up
//
// V1.4 Added logic to check for existing output file and automatically set the NewOutputFile flag
//      if the file does not exist
//
//      Added output of 5th and 95 percentile values for elevation and intensity
//
//      Fixed problem with /id switch...parsing logic failed if there were no numbers in the file title
//      now the ID is set to 0 if there are no numbers
//
// V1.5 2/8/2007
//      Changed reporting logic so the min. max, and mean values are reported when there are points in the
//      data file...previous version required 4 or more points
//
//      Added /highpoint option to produce output with only #pts and highest point XYZ...used for individual
//      tree samples
//
//      Removed trailing comma from CSV output
//
// V1.6 2/8/2007
//      Added htmin:# switch to restrict statistics to points at or above the specified height...
//      used only when data sample has been normalized using a ground surface model
//
// V1.7 11/4/2008
//		Added /firstreturn switch to use only first returns to compute metrics. Previous versions used
//		the /firstinpulse switch to identify the first return in a pulse which was not necessarily the first
//		return due to tiled data deliveries and clipped data samples. The logic associated with the 
//		/firstreturn switch is much simplier and should result in consistent results no matter the processing
//		steps used to produce a point cloud file.
//
// V1.71 7/14/2009
//		Fixed problem reading list files with a blank line at the end of the file. Previous version would hang.
//
// V1.80 7/28/2009
//		Added /relcover switch for computing cover above various height metrics
//
// V1.90 8/8/2009
//		Added /alldensity switch to compute cover estimates using all returns instead of only first returns
//
// V2.0 9/16/2009 & 10/19/2009
//		Modified the logic used to compute metrics related to cover. Now the /above:# switch also triggers calculation 
//		of the proportion of returns above the mean and mode elevations (or heights) and the cover value
//		using all returns (instead of onlyfirst returns).
//
//		Also added columns for the number of returns by return number and the point counts used for the cover
//		calculations. These columns come just after the total number of points used for the metrics (pts above htmin)
//
//		Cleaned up variables computed and output
//			removed median and duplicate 50th percentile columns (median is the same as 50th percentile and this value
//				was output 3 times in the file)
//			added coefficient of variation
//			re-ordered percentile values...moved 25th and 75th percentile values into sequence of other percentile values
//
//		Also added /first switch (deos the same thing as /firstreturn switch) to maintain consistency with GridMetrics
//
// 2/3/2010 CloudMetrics V2.10
//		Added code to compute L moments and moment ratios. Added 14 columns to output (7 each for elevation and intensity values)
//
// 2/19/2010 CloudMetrics V2.20
//		Changed /htmin to /minht to match GridMetrics. /htmin is still recognized but not documented. Added /maxht
//		option to better match GridMetrics. In practice, it is best to use this option when clipping data samples with
//		ClipData and then simply use CloudMetrics to compute the metrics. However, some analysis processes are better served
//		when you can use this option with CloudMetrics (e.g. computing metrics using height slices via /outlier)
//
// 3/11/2010 CloudMetrics V2.30
//		Added file title to output as a separate column from the filename. This helps when the point data files are named
//		using a meaningful name (such as plot identifier). The /id option also provides an identifier for each line of data
//		but only works with numbers in the file name. This new column contains the full file name.
//
// 9/7/2010 CloudMetrics V2.31
//		Corrected the use of the height minimum and maximum to include points above (but not equal to) the minimum height
//		and below (but not equal to) the maximum height. Previous versions included points equal to the min and max heights.
//
// 11/2/2010 CloudMetrics V2.32
//		Added the /outlier switch to make the command line sytnax more consistent with GridMetrics. The /outlier:low,hig
//		switch has the same effect as using both the /minht:low and /maxht:high switches. The values specified via the /outlier
//		switch will override the values specified with the /minht and /maxht switches.
//
// 11/10/2010 CloudMetrics V2.33
//	Added command line parameter verification logic.
//
// 12/6/2010 CloudMetrics V2.34
//	Corrected some variable names to match those output in GridMetrics: 
//		Elev InterquartileDistance changed to Elev IQ
//		Int InterquartileDistance changed to Int IQ
//	Changed the capitalization of some of the variable names. For the most part the rule is that the first letter of the column
//	heading is capitalized and no other letter. If a word in the heading is an abbreviation, it is capitalized. For example, CV for
//	coefficient of variation.
//
//	1/14/2011 CloudMetrics V2.35
//	Restructured code to provide better organization and make it easier to maintain and distribute FUSION source code.
//
#include "stdafx.h"
#include <time.h>
#include <float.h>
#include <math.h>
#include "..\..\fusion\versionID.h"
#include "lidardata.h"
#include "dataindex.h"
#include "filespec.h"
#include "plansdtm.h"
#include "argslib.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "DataCatalogEntry.h"
#include "command_line_core_functions.h"

#define		PROGRAM_NAME		"CloudMetrics"
#define		PROGRAM_VERSION		2.35
#define		MINIMUM_CL_PARAMS	2
//			                              1         2         3         4         5         6         7         8         9         10
//			                     1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
#define		PROGRAM_DESCRIPTION	"Computes metrics describing an entire point cloud"

#define		NUMBEROFBINS		64
#define		NUMBER_OF_CLASS_CODES		32

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;

using namespace std;

char* ValidCommandLineSwitches = "above new firstinpulse firstreturn first highpoint subset id minht maxht outlier";

// global variables...not the best programming practice but helps with a "standard" template for command line utilities
CList<CDataCatalogEntry, CDataCatalogEntry&> DataFile;
int DataFileCount;
CDataCatalogEntry ce;
BOOL NewOutputFile;
BOOL ParseID;
BOOL ComputeCover;
BOOL m_ProduceHighpointOutput;
BOOL m_ProduceYZLiOutput;
BOOL m_UseFirstReturnInPulse;
BOOL m_UseFirstReturns;
BOOL m_UseHeightMin;
BOOL m_UseHeightMax;

BOOL m_ComputeRelCover;
BOOL m_UseAllReturnsForCover;
BOOL m_CountReturns;

double m_MinHeight;
double m_MaxHeight;
double CoverCutoff;
CString InputFileCL;
CString OutputFileCL;
CFileSpec DataFS;
CFileSpec OutputFS;

// global functions that are modified to create a new program

void usage()
{
	LTKCL_PrintProgramHeader();
	LTKCL_PrintProgramDescription();
//			         1         2         3         4         5         6         7         8         9         10
//			1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
	printf("\nSyntax: %s [switches] InputSpecifier OutputFile\n", PROGRAM_NAME);
	printf(" InputSpecifier LIDAR data file template, name of text file containing a\n");
	printf("              list of file names (must have .txt extension), or a\n");
	printf("              catalog file\n");
	printf(" OutputFile   Name for output file to contain cloud metrics (usually\n");
	printf("              .csv extension)\n");
	printf(" Switches:\n");

	LTKCL_PrintStandardSwitchInfo();
	
	printf("  above:#     Compute proportion of first returns above # (canopy cover).\n");
	printf("              Also compute the proportion of all returns above # and the\n");
	printf("              (number of returns above #) / (total number of 1st returns).\n");
	printf("              The same metrics are also computed using the mean and mode\n");
	printf("              point elevation (or height) values. Starting with version\n");
	printf("              2.0 of %s, the /relcover and /alldensity options were\n", PROGRAM_NAME);
	printf("              removed. All cover metrics are computed when the /above:#\n");
	printf("              switch is used.\n");
	printf("  new         Start new output file...delete existing output file\n");
	printf("  firstinpulse Use only the first return for the pulse to compute metrics\n");
	printf("  firstreturn Use only first returns to compute metrics\n");
	printf("  first       Use only first returns to compute metrics (same as /firstreturn)\n");
	printf("  highpoint   Produce a limited set of metrics ([ID],#pts,highX,highY,highZ)\n");
	printf("  subset      Produce a limited set of metrics([ID],#pts,Mean ht,Std dev ht,\n");
	printf("              75th percentile,cover)...must be used with /above:#\n");
	printf("  id          Parse the data file name to create an identifier...output\n");
	printf("              as the first column of data\n");
	printf("  minht:#     Use only returns above # (use when data is normalized to ground)\n");
	printf("              Prior to version 2.20 this switch was /htmin. /htmin can still\n");
	printf("              be used but /minht is preferred.\n");
	printf("  maxht:#     Use only returns below # (use when data is normalized to ground)\n");
	printf("  outlier:low,high Omit points with elevations below low and above high.\n");
	printf("              When used with data that has been normalized using a ground\n");
	printf("              surface, low and high are interpreted as heights above ground.\n");
	printf("              Using /outlier:low,high has the same effect as using both\n");
	printf("              /minht:low and /maxht:high. The values specified using the\n");
	printf("              /outlier switch will override the values specified using the\n");
	printf("              /minht and /maxht switches.\n");
//	printf("  alldensity  Use all returns to compute canopy and relative cover values.\n");
//	printf("  relcover    Compute proportion of first returns above the mean and mode\n");
//	printf("              of return elevations (or heights)\n");
	printf("\nColumn labels will only be correct if the output file does not exist or if\n");
	printf("the /new switch is specified.\n");

//			         1         2         3         4         5         6         7         8         9         10
//			1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890

//	LTKCL_LaunchHelpPage();
}

void InitializeGlobalVariables()
{
	m_nRetCode = 0;
	CoverCutoff = 0.0;

	m_ProduceHighpointOutput = FALSE;
	m_ProduceYZLiOutput = FALSE;
	m_UseFirstReturnInPulse = FALSE;
	m_UseFirstReturns = FALSE;
	m_UseHeightMin = FALSE;
	m_UseHeightMax = FALSE;

	m_UseAllReturnsForCover = FALSE;

	m_ComputeRelCover = FALSE;

	m_CountReturns = TRUE;		// always put out counts (new in V2.0)
}

int PresentInteractiveDialog()
{
	if (m_RunInteractive) {
		usage();
		::MessageBox(NULL, "Interactive mode is not currently implemented", PROGRAM_NAME, MB_OK);
		return(1);		// temp...needs to reflect success/failure in interactive mode or user pressing cancel
	}
	return(0);
}

int ParseCommandLine()
{
	if (m_clp.CheckHelp() || m_clp.ParamCount() - m_clp.SwitchCount() < MINIMUM_CL_PARAMS) {
		usage();
		return(1);
	}

	// process command line switches
	NewOutputFile = m_clp.Switch("new");
	m_UseFirstReturnInPulse = m_clp.Switch("firstinpulse");
	m_UseFirstReturns = m_clp.Switch("firstreturn");
	if (!m_UseFirstReturns)
		m_UseFirstReturns = m_clp.Switch("first");
	ParseID = m_clp.Switch("id");
	ComputeCover = m_clp.Switch("above");
//	m_UseAllReturnsForCover = m_clp.Switch("alldensity");
	CoverCutoff = atof(m_clp.GetSwitchStr("above", "0.0"));
//	m_ComputeRelCover = m_clp.Switch("relcover");
	m_ProduceHighpointOutput = m_clp.Switch("highpoint");
	m_ProduceYZLiOutput = m_clp.Switch("subset");
	m_UseHeightMin = m_clp.Switch("htmin");
	m_MinHeight = atof(m_clp.GetSwitchStr("htmin", "0.0"));
	if (!m_UseHeightMin) {
		m_UseHeightMin = m_clp.Switch("minht");
		m_MinHeight = atof(m_clp.GetSwitchStr("minht", "0.0"));
	}
	m_UseHeightMax = m_clp.Switch("maxht");
	m_MaxHeight = atof(m_clp.GetSwitchStr("maxht", "0.0"));

	// check for /outlier switch...same effect as including /htmin and htmax switches
	if (m_clp.Switch("outlier")) {
		CString temp = m_clp.GetSwitchStr("outlier", "");
		if (!temp.IsEmpty()) {
			sscanf(temp, "%lf,%lf", &m_MinHeight, &m_MaxHeight);

			m_UseHeightMax = TRUE;
			m_UseHeightMin = TRUE;
		}
	}

	// force all return cover metrics if doing any cover metrics
	m_UseAllReturnsForCover = ComputeCover;
	m_ComputeRelCover = ComputeCover;

	// get input file specifier...may be single file, wildcard, or list file with .txt extension
	InputFileCL = m_clp.ParamStr(m_clp.FirstNonSwitchIndex());
	DataFS.SetFullSpec(InputFileCL);

	// get output file specifier
	OutputFileCL = m_clp.ParamStr(m_clp.FirstNonSwitchIndex() + 1);

	// check to see if the output file exists...if not, set the NewOutputFile flag to include headers
	// in the output file
	OutputFS.SetFullSpec(OutputFileCL);
	if (!OutputFS.Exists())
		NewOutputFile = TRUE;

	// build list of input data files...may contain 1 or more files
	if (DataFS.Extension().CompareNoCase(".txt") == 0) {
		// read data files from list file
		CDataFile lst(DataFS.GetFullSpec());
		if (lst.IsValid()) {
			char buf[1024];
			CFileSpec TempFS;
			while (lst.NewReadASCIILine(buf)) {
				TempFS.SetFullSpec(buf);
				if (TempFS.Exists()) {
					ce.m_FileName = TempFS.GetFullSpec();
					ce.m_CheckSum = 0;
					ce.m_MinX = ce.m_MinY = ce.m_MinZ = ce.m_MaxX = ce.m_MaxY = ce.m_MaxZ = 0.0;
					ce.m_PointDensity= -1.0;
					ce.m_Points = -1;
					DataFile.AddTail(ce);

					DataFileCount ++;
				}
			}
		}
	}
	else if (DataFS.Extension().CompareNoCase(".csv") == 0) {
		// read data files from catalog file
		CDataFile lst(DataFS.GetFullSpec());
		if (lst.IsValid()) {
			char buf[1024];
			CFileSpec TempFS;
			WIN32_FILE_ATTRIBUTE_DATA attribdata;
			long chksum;
			while (lst.ReadASCIILine(buf)) {
				// parse file name and check to see if it exists
				TempFS.SetFullSpec(strtok(buf, " ,\t"));
				if (TempFS.Exists()) {
					ce.m_FileName = TempFS.GetFullSpec();

					// parse remaining fields
					ce.m_CheckSum = atol(strtok(NULL, " ,\t"));
					ce.m_MinX = atof(strtok(NULL, " ,\t"));
					ce.m_MinY = atof(strtok(NULL, " ,\t"));
					ce.m_MinZ = atof(strtok(NULL, " ,\t"));
					ce.m_MaxX = atof(strtok(NULL, " ,\t"));
					ce.m_MaxY = atof(strtok(NULL, " ,\t"));
					ce.m_MaxZ = atof(strtok(NULL, " ,\t"));
					ce.m_PointDensity = atof(strtok(NULL, " ,\t"));
					ce.m_Points = atoi(strtok(NULL, " ,\t"));

					// validate the checksum
					// get file modify time and compute checksum value
					// using low DWORD components should catch all the details.  Not likely the modification time will keep the 
					// same nanosecond count or that the file size will change by 2Gb chunks
					chksum = 0;
					if (GetFileAttributesEx(ce.m_FileName, GetFileExInfoStandard, &attribdata)) {
						chksum = attribdata.nFileSizeLow + attribdata.ftLastWriteTime.dwLowDateTime;
					}

					// if checksum doesn't match clear the information from the catalog entry
					if (ce.m_CheckSum != chksum) {
						ce.m_CheckSum = 0;
						ce.m_MinX = ce.m_MinY = ce.m_MinZ = ce.m_MaxX = ce.m_MaxY = ce.m_MaxZ = 0.0;
						ce.m_PointDensity= -1.0;
						ce.m_Points = -1;
					}

					DataFile.AddTail(ce);
					DataFileCount ++;
				}
			}
		}
	}
	else {
		// look for data files using file specifier
		CFileFind finder;
		int FileCount = 0;
		BOOL bWorking = finder.FindFile(InputFileCL);
		while (bWorking) {
			bWorking = finder.FindNextFile();
			FileCount ++;
		}

		if (FileCount > 1 || (FileCount == 1 && InputFileCL.FindOneOf("*?") >= 0)) {
			bWorking = finder.FindFile(InputFileCL);
			DataFileCount = 0;
			while (bWorking) {
				bWorking = finder.FindNextFile();
				ce.m_FileName = finder.GetFilePath();
				ce.m_CheckSum = 0;
				ce.m_MinX = ce.m_MinY = ce.m_MinZ = ce.m_MaxX = ce.m_MaxY = ce.m_MaxZ = 0.0;
				ce.m_PointDensity= -1.0;
				ce.m_Points = -1;
				DataFile.AddTail(ce);

				DataFileCount ++;
			}
		}
		else {
			ce.m_FileName = InputFileCL;
			ce.m_CheckSum = 0;
			ce.m_MinX = ce.m_MinY = ce.m_MinZ = ce.m_MaxX = ce.m_MaxY = ce.m_MaxZ = 0.0;
			ce.m_PointDensity= -1.0;
			ce.m_Points = -1;
			DataFile.AddTail(ce);

			DataFileCount = 1;
		}
	}

	if (m_ProduceYZLiOutput && !ComputeCover) {
		LTKCL_PrintStatus("You must use the /above:# switch with the /subset switch");
		return(1);
	}
	return(0);
}

// comparison function for sorting...must be global to use qsort()
int compareflt(const void *arg1, const void *arg2)
{
	if (*((float*) arg1) < (*(float*) arg2))
		return(-1);
	else if (*((float*) arg1) > (*(float*) arg2))
		return(1);
	else
		return(0);
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	CString csTemp;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		m_nRetCode = 1;
	}
	else
	{
		// initialize toolkit variables...if return value is FALSE, /version was on the command line
		// with /version, only the program name and version information are output
		if (!LTKCL_Initialize())
			return(0);

		InitializeGlobalVariables();

		m_nRetCode = ParseCommandLine();

		m_nRetCode |= PresentInteractiveDialog();

		// do the processing to create data subsample
		// ********************************************************************************************************
		if (DataFileCount && !m_nRetCode) {
			// print status info
			LTKCL_PrintProgramHeader();
			LTKCL_PrintCommandLine();
			LTKCL_PrintRunTime();

			// look for bad switches on command line
			LTKCL_VerifyCommandLine(ValidCommandLineSwitches);
			
			if (!m_nRetCode) {
				CFileSpec Tempfs;
				CFileSpec TempfsForFileTitle;
				LIDARRETURN pt;

				// stat variables
				int PointCount;
				double ElevMin, ElevMax, ElevMean, ElevMedian, ElevMode, ElevStdDev, ElevVariance, ElevIQDist, ElevSkewness, ElevKurtosis, ElevAAD, ElevP25, ElevP75;
				double ElevP01, ElevP05, ElevP10, ElevP20, ElevP30, ElevP40, ElevP50, ElevP60, ElevP70, ElevP80, ElevP90, ElevP95, ElevP99;
				double ElevL1, ElevL2, ElevL3, ElevL4;
				double IntMin, IntMax, IntMean, IntMedian, IntMode, IntStdDev, IntVariance, IntIQDist, IntSkewness, IntKurtosis, IntAAD, IntP25, IntP75;
				double IntP01, IntP05, IntP10, IntP20, IntP30, IntP40, IntP50, IntP60, IntP70, IntP80, IntP90, IntP95, IntP99;
				double IntL1, IntL2, IntL3, IntL4;
				double Cover;
				double AllCover;
				double AllFirstCover;
				double HighX, HighY, HighElevation;
				int FirstReturnsAbove;
				int FirstReturnsTotal;
				int AllReturnsAbove;
				int AllReturnsTotal;
				float* ElevValueList;
				float* IntValueList;
				float ElevPercentile[21];
				float IntPercentile[21];
				int Bins[NUMBEROFBINS];
				int ReturnCounts[10];
				int TheBin;
				int ID;
				int LastReturn;
				float Fraction;
				int WholePart;
				double CL1, CL2, CL3, CR1, CR2, CR3, C1, C2, C3, C4;

				// open output file...if new switch was used, start a new file, otherwise append and don't write a header
				FILE* f;
				if (NewOutputFile) {
					f = fopen(OutputFileCL, "wt");

					if (ParseID)
						fprintf(f, "Identifier,");

					if (m_ProduceHighpointOutput) {
						fprintf(f, "DataFile,Points,High point X,High point Y,High point elevation\n");
					}
					else if (m_ProduceYZLiOutput) {
						fprintf(f, "DataFile,Points,Elev mean,Elev stddev,Elev P75,Percentage of first returns above %.2lf\n", CoverCutoff);
					}
					else {
						// write base column label header
						if (m_UseHeightMin) {
							if (m_UseHeightMax)
								fprintf(f, "DataFile,FileTitle,Total return count above %.2lf and below %.2lf", m_MinHeight, m_MaxHeight);
							else
								fprintf(f, "DataFile,FileTitle,Total return count above %.2lf", m_MinHeight);
						}
						else if (m_UseHeightMax)
							fprintf(f, "DataFile,FileTitle,Total return count below %.2lf", m_MaxHeight);
						else
							fprintf(f, "DataFile,FileTitle,Total return count");

						// add headings for return counts
						if (m_CountReturns) {
							if (m_UseHeightMin) {
								if (m_UseHeightMax)
									fprintf(f, ",Return 1 count above %.2lf and below %.2lf,Return 2 count above %.2lf and below %.2lf,Return 3 count above %.2lf and below %.2lf,Return 4 count above %.2lf and below %.2lf,Return 5 count above %.2lf and below %.2lf,Return 6 count above %.2lf and below %.2lf,Return 7 count above %.2lf and below %.2lf,Return 8 count above %.2lf and below %.2lf,Return 9 count above %.2lf and below %.2lf,Other return count above %.2lf and below %.2lf", m_MinHeight, m_MaxHeight, m_MinHeight, m_MaxHeight, m_MinHeight, m_MaxHeight, m_MinHeight, m_MaxHeight, m_MinHeight, m_MaxHeight, m_MinHeight, m_MaxHeight, m_MinHeight, m_MaxHeight, m_MinHeight, m_MaxHeight, m_MinHeight, m_MaxHeight, m_MinHeight);
								else
									fprintf(f, ",Return 1 count above %.2lf,Return 2 count above %.2lf,Return 3 count above %.2lf,Return 4 count above %.2lf,Return 5 count above %.2lf,Return 6 count above %.2lf,Return 7 count above %.2lf,Return 8 count above %.2lf,Return 9 count above %.2lf,Other return count above %.2lf", m_MinHeight, m_MinHeight, m_MinHeight, m_MinHeight, m_MinHeight, m_MinHeight, m_MinHeight, m_MinHeight, m_MinHeight, m_MinHeight);
							}
							else if (m_UseHeightMax)
									fprintf(f, ",Return 1 count below %.2lf,Return 2 count below %.2lf,Return 3 count below %.2lf,Return 4 count below %.2lf,Return 5 count below %.2lf,Return 6 count below %.2lf,Return 7 count below %.2lf,Return 8 count below %.2lf,Return 9 count below %.2lf,Other return count below %.2lf", m_MaxHeight, m_MaxHeight, m_MaxHeight, m_MaxHeight, m_MaxHeight, m_MaxHeight, m_MaxHeight, m_MaxHeight, m_MaxHeight, m_MaxHeight);
							else
								fprintf(f, ",Return 1 count,Return 2 count,Return 3 count,Return 4 count,Return 5 count,Return 6 count,Return 7 count,Return 8 count,Return 9 count,Other return count");
						}

						fprintf(f, ",Elev minimum,Elev maximum,Elev mean,Elev mode,Elev stddev,Elev variance,Elev CV,Elev IQ,Elev skewness,Elev kurtosis,Elev AAD,Elev L1,Elev L2,Elev L3,Elev L4,Elev L CV,Elev L skewness,Elev L kurtosis,Elev P01,Elev P05,Elev P10,Elev P20,Elev P25,Elev P30,Elev P40,Elev P50,Elev P60,Elev P70,Elev P75,Elev P80,Elev P90,Elev P95,Elev P99");
						fprintf(f, ",Int minimum,Int maximum,Int mean,Int mode,Int stddev,Int variance,Int CV,Int IQ,Int skewness,Int kurtosis,Int AAD,Int L1,Int L2,Int L3,Int L4,Int L CV,Int L skewness,Int L kurtosis,Int P01,Int P05,Int P10,Int P20,Int P25,Int P30,Int P40,Int P50,Int P60,Int P70,Int P75,Int P80,Int P90,Int P95,Int P99");

						if (ComputeCover) {
							if (m_UseAllReturnsForCover)
								fprintf(f, ",Percentage first returns above %.2lf,Percentage all returns above %.2lf,(All returns above %.2lf) / (Total first returns) * 100,First returns above %.2lf,All returns above %.2lf", CoverCutoff, CoverCutoff, CoverCutoff, CoverCutoff, CoverCutoff);
							else
								fprintf(f, ",Percentage first returns above %.2lf", CoverCutoff);
						}

						if (m_ComputeRelCover) {
							fprintf(f, ",Percentage first returns above mean,Percentage first returns above mode,Percentage all returns above mean,Percentage all returns above mode,(All returns above mean) / (Total first returns) * 100,(All returns above mode) / (Total first returns) * 100");
							fprintf(f, ",First returns above mean,First returns above mode,All returns above mean,All returns above mode,Total first returns,Total all returns");
						}


						// print end of line for header
						fprintf(f, "\n");
					}
				}
				else
					f = fopen(OutputFileCL, "at");

				if (f) {
					// iterate through the list of data files and compute metrics
					POSITION pos = DataFile.GetHeadPosition();
					for (int i = 0; i < DataFile.GetCount(); i ++) {
						ce = DataFile.GetNext(pos);

						PointCount = 0;

						// intialize return counts
						for (int k = 0; k < 10; k ++)
							ReturnCounts[k] = 0;

						CLidarData dat(ce.m_FileName);
						if (dat.IsValid()) {
							// get the file title
							TempfsForFileTitle.SetFullSpec(ce.m_FileName);

							// parse ID from file name
							if (ParseID) {
								CFileSpec tfs(ce.m_FileName);
								if (strpbrk(tfs.FileTitle(), "0123456789")) {
									ID = atoi(strpbrk(tfs.FileTitle(), "0123456789"));
								}
								else {
									ID = 0;
								}
							}

							// initialize variables
							ElevMin = 9999999999.0;
							ElevMax = -9999999999.0;
							ElevMean = 0.0;
							IntMin = 9999999999.0;
							IntMax = -9999999999.0;
							IntMean = 0.0;
							LastReturn = 9999;

							// read all returns and do simple statistics
							while (dat.ReadNextRecord(&pt)) {
								if (m_UseFirstReturns && pt.ReturnNumber > 1) {
									LastReturn = pt.ReturnNumber;
									continue;
								}

								if (m_UseFirstReturnInPulse && (pt.ReturnNumber > 1 && pt.ReturnNumber > LastReturn)) {
									LastReturn = pt.ReturnNumber;
									continue;
								}

								if (m_UseHeightMin && pt.Elevation <= m_MinHeight) {
									LastReturn = pt.ReturnNumber;
									continue;
								}

								if (m_UseHeightMax && pt.Elevation >= m_MaxHeight) {
									LastReturn = pt.ReturnNumber;
									continue;
								}

								if (PointCount == 0) {
									HighX = pt.X;
									HighY = pt.Y;
									HighElevation = pt.Elevation;
								}

								if (pt.Elevation < ElevMin)
									ElevMin = (double) pt.Elevation;
								if (pt.Elevation > ElevMax) {
									ElevMax = (double) pt.Elevation;
									HighX = pt.X;
									HighY = pt.Y;
									HighElevation = pt.Elevation;
								}

								ElevMean += (double) pt.Elevation;

								if (pt.Intensity < IntMin)
									IntMin = (double) pt.Intensity;
								if (pt.Intensity > IntMax)
									IntMax = (double) pt.Intensity;

								IntMean += (double) pt.Intensity;

								PointCount ++;

								// count returns
								if (pt.ReturnNumber < 10 && pt.ReturnNumber >= 1)
									ReturnCounts[pt.ReturnNumber - 1] ++;
								else
									ReturnCounts[9] ++;

								LastReturn = pt.ReturnNumber;
							}

							if (PointCount >= 1) {
								// compute means
								ElevMean = ElevMean / (double) PointCount;
								IntMean = IntMean / (double) PointCount;
							}

							// must have at least 4 points
							if (PointCount >= 4) {
								// allocate memory for list to get median and Quartile values
								ElevValueList = new float[PointCount];
								IntValueList = new float[PointCount];

								// initialize
								ElevVariance = 0.0;
								ElevKurtosis = 0.0;
								ElevSkewness = 0.0;
								ElevAAD = 0.0;

								IntVariance = 0.0;
								IntKurtosis = 0.0;
								IntSkewness = 0.0;
								IntAAD = 0.0;

								// reset point count so we can use it as an array index
								PointCount = 0;

								// read all returns and do statistics
								dat.Rewind();
								LastReturn = 9999;
								while (dat.ReadNextRecord(&pt)) {
									if (m_UseFirstReturns && pt.ReturnNumber > 1) {
										LastReturn = pt.ReturnNumber;
										continue;
									}

									if (m_UseFirstReturnInPulse && (pt.ReturnNumber > 1 && pt.ReturnNumber > LastReturn)) {
										LastReturn = pt.ReturnNumber;
										continue;
									}

									if (m_UseHeightMin && pt.Elevation <= m_MinHeight) {
										LastReturn = pt.ReturnNumber;
										continue;
									}

									if (m_UseHeightMax && pt.Elevation >= m_MaxHeight) {
										LastReturn = pt.ReturnNumber;
										continue;
									}

									ElevVariance += ((double) pt.Elevation - ElevMean) * ((double) pt.Elevation - ElevMean);
									ElevSkewness += ((double) pt.Elevation - ElevMean) * ((double) pt.Elevation - ElevMean) * ((double) pt.Elevation - ElevMean);
									ElevKurtosis += ((double) pt.Elevation - ElevMean) * ((double) pt.Elevation - ElevMean) * ((double) pt.Elevation - ElevMean) * ((double) pt.Elevation - ElevMean);
									ElevAAD += fabs(((double) pt.Elevation - ElevMean));

									IntVariance += ((double) pt.Intensity - IntMean) * ((double) pt.Intensity - IntMean);
									IntSkewness += ((double) pt.Intensity - IntMean) * ((double) pt.Intensity - IntMean) * ((double) pt.Intensity - IntMean);
									IntKurtosis += ((double) pt.Intensity - IntMean) * ((double) pt.Intensity - IntMean) * ((double) pt.Intensity - IntMean) * ((double) pt.Intensity - IntMean);
									IntAAD += fabs(((double) pt.Intensity - IntMean));

									// add value to list for median
									ElevValueList[PointCount] = pt.Elevation;
									IntValueList[PointCount] = pt.Intensity;

									PointCount ++;

									LastReturn = pt.ReturnNumber;
								}

								// compute statistics
								ElevVariance = ElevVariance / ((double) (PointCount - 1));
								ElevStdDev = sqrt(ElevVariance);
								ElevSkewness = ElevSkewness / ((double) (PointCount - 1) * ElevStdDev * ElevStdDev * ElevStdDev);
								ElevKurtosis = ElevKurtosis / ((double) (PointCount - 1) * ElevStdDev * ElevStdDev * ElevStdDev * ElevStdDev);
								ElevAAD = ElevAAD / (double) PointCount;

								// cover was claculated here 7/28/2009

								IntVariance = IntVariance / ((double) (PointCount - 1));
								IntStdDev = sqrt(IntVariance);
								IntSkewness = IntSkewness / ((double) (PointCount - 1) * IntStdDev * IntStdDev * IntStdDev);
								IntKurtosis = IntKurtosis / ((double) (PointCount - 1) * IntStdDev * IntStdDev * IntStdDev * IntStdDev);
								IntAAD = IntAAD / (double) PointCount;

								// sort value list
								qsort(ElevValueList, (size_t) PointCount, sizeof(float), compareflt);
								qsort(IntValueList, (size_t) PointCount, sizeof(float), compareflt);

								// compute percentile related metrics
								// source http://www.resacorp.com/quartiles.htm...method 5
								ElevPercentile[0] = ElevMin;
								IntPercentile[0] = IntMin;
								for (k = 1; k < 20; k ++) {
									WholePart = (int) ((float) (PointCount - 1) * ((float) k * 5.0) / 100.0);
									Fraction = ((float) (PointCount - 1) * ((float) k * 5.0) / 100.0) - WholePart;
								
									if (Fraction == 0.0) {
										ElevPercentile[k] = ElevValueList[WholePart];
										IntPercentile[k] = IntValueList[WholePart];
									}
									else {
										ElevPercentile[k] = ElevValueList[WholePart] + Fraction * (ElevValueList[WholePart + 1] - ElevValueList[WholePart]);
										IntPercentile[k] = IntValueList[WholePart] + Fraction * (IntValueList[WholePart + 1] - IntValueList[WholePart]);
									}
								}
								ElevPercentile[20] = ElevMax;
								IntPercentile[20] = IntMax;

								// compute P01 and P99 values
								WholePart = (int) ((float) (PointCount - 1) * 1.0 / 100.0);
								Fraction = ((float) (PointCount - 1) * 1.0 / 100.0) - WholePart;
							
								if (Fraction == 0.0) {
									ElevP01 = ElevValueList[WholePart];
									IntP01 = IntValueList[WholePart];
								}
								else {
									ElevP01 = ElevValueList[WholePart] + Fraction * (ElevValueList[WholePart + 1] - ElevValueList[WholePart]);
									IntP01 = IntValueList[WholePart] + Fraction * (IntValueList[WholePart + 1] - IntValueList[WholePart]);
								}

								WholePart = (int) ((float) (PointCount - 1) * 99.0 / 100.0);
								Fraction = ((float) (PointCount - 1) * 99.0 / 100.0) - WholePart;
							
								if (Fraction == 0.0) {
									ElevP99 = ElevValueList[WholePart];
									IntP99 = IntValueList[WholePart];
								}
								else {
									ElevP99 = ElevValueList[WholePart] + Fraction * (ElevValueList[WholePart + 1] - ElevValueList[WholePart]);
									IntP99 = IntValueList[WholePart] + Fraction * (IntValueList[WholePart + 1] - IntValueList[WholePart]);
								}

								// compute metrics
								ElevMedian = ElevPercentile[10];

								ElevP05 = ElevPercentile[1];
								ElevP10 = ElevPercentile[2];
								ElevP20 = ElevPercentile[4];
								ElevP25 = ElevPercentile[5];
								ElevP30 = ElevPercentile[6];
								ElevP40 = ElevPercentile[8];
								ElevP50 = ElevPercentile[10];
								ElevP60 = ElevPercentile[12];
								ElevP70 = ElevPercentile[14];
								ElevP75 = ElevPercentile[15];
								ElevP80 = ElevPercentile[16];
								ElevP90 = ElevPercentile[18];
								ElevP95 = ElevPercentile[19];

								// compute interquartile distance
								ElevIQDist = ElevP75 - ElevP25;

								IntMedian = IntPercentile[10];

								IntP05 = IntPercentile[1];
								IntP10 = IntPercentile[2];
								IntP20 = IntPercentile[4];
								IntP25 = IntPercentile[5];
								IntP30 = IntPercentile[6];
								IntP40 = IntPercentile[8];
								IntP50 = IntPercentile[10];
								IntP60 = IntPercentile[12];
								IntP70 = IntPercentile[14];
								IntP75 = IntPercentile[15];
								IntP80 = IntPercentile[16];
								IntP90 = IntPercentile[18];
								IntP95 = IntPercentile[19];

								// compute interquartile distance
								IntIQDist = IntP75 - IntP25;

								// figure out the mode using 64 bins for data
								for (k = 0; k < NUMBEROFBINS; k ++)
									Bins[k] = 0;

								// count values using bins
								for (k = 0; k < PointCount; k ++) {
									TheBin = (int) ((((double) ElevValueList[k] - ElevMin) / (ElevMax - ElevMin)) * (double) (NUMBEROFBINS - 1));
									Bins[TheBin] ++;
								}

								// find most frequent value
								int MaxCount = -1;
								for (k = 0; k < NUMBEROFBINS; k ++) {
									if (Bins[k] > MaxCount) {
										MaxCount = Bins[k];
										TheBin = k;
									}
								}

								// compute mode by un-scaling the bin number
								ElevMode = ElevMin + ((double) TheBin / (double) (NUMBEROFBINS - 1)) * (ElevMax - ElevMin);

								// figure out the mode using 64 bins for data
								for (k = 0; k < NUMBEROFBINS; k ++)
									Bins[k] = 0;

								// count values using bins
								for (k = 0; k < PointCount; k ++) {
									TheBin = (int) ((((double) IntValueList[k] - IntMin) / (IntMax - IntMin)) * (double) (NUMBEROFBINS - 1));
									Bins[TheBin] ++;
								}

								// find most frequent value
								MaxCount = -1;
								for (k = 0; k < NUMBEROFBINS; k ++) {
									if (Bins[k] > MaxCount) {
										MaxCount = Bins[k];
										TheBin = k;
									}
								}

								// compute mode by un-scaling the bin number
								IntMode = IntMin + ((double) TheBin / (double) (NUMBEROFBINS - 1)) * (IntMax - IntMin);

								// compute L moments & ratios
								// elevation related
								ElevL1 = ElevL2 = ElevL3 = ElevL4 = 0.0;
								for (k = 0; k < PointCount; k ++) {
									CL1 = (k + 1) - 1;
									CL2 = CL1 * (double) ((k + 1) - 1 - 1) / 2.0;
									CL3 = CL2 * (double) ((k + 1) - 1 - 2) / 3.0;

									CR1 = PointCount - (k + 1);
									CR2 = CR1 * (double) (PointCount - (k + 1) - 1) / 2.0;
									CR3 = CR2 * (double) (PointCount - (k + 1) - 2) / 3.0;

									ElevL1 = ElevL1 + ElevValueList[k];
									ElevL2 = ElevL2 + (CL1 - CR1) * ElevValueList[k];
									ElevL3 = ElevL3 + (CL2 - 2.0 * CL1 * CR1 + CR2) * ElevValueList[k];
									ElevL4 = ElevL4 + (CL3 - 3.0 * CL2 * CR1 + 3.0 * CL1 * CR2 - CR3) * ElevValueList[k];
								}
								C1 = PointCount;
								C2 = C1 * (double) (PointCount - 1) / 2.0;
								C3 = C2 * (double) (PointCount - 2) / 3.0;
								C4 = C3 * (double) (PointCount - 3) / 4.0;
								ElevL1 = ElevL1 / C1;
								ElevL2 = ElevL2 / C2 / 2.0;
								ElevL3 = ElevL3 / C3 / 3.0;
								ElevL4 = ElevL4 / C4 / 4.0;

								// intensity related
								IntL1 = IntL2 = IntL3 = IntL4 = 0.0;
								for (k = 0; k < PointCount; k ++) {
									CL1 = (k + 1) - 1;
									CL2 = CL1 * (double) ((k + 1) - 1 - 1) / 2.0;
									CL3 = CL2 * (double) ((k + 1) - 1 - 2) / 3.0;

									CR1 = PointCount - (k + 1);
									CR2 = CR1 * (double) (PointCount - (k + 1) - 1) / 2.0;
									CR3 = CR2 * (double) (PointCount - (k + 1) - 2) / 3.0;

									IntL1 = IntL1 + IntValueList[k];
									IntL2 = IntL2 + (CL1 - CR1) * IntValueList[k];
									IntL3 = IntL3 + (CL2 - 2.0 * CL1 * CR1 + CR2) * IntValueList[k];
									IntL4 = IntL4 + (CL3 - 3.0 * CL2 * CR1 + 3.0 * CL1 * CR2 - CR3) * IntValueList[k];
								}
								C1 = PointCount;
								C2 = C1 * (double) (PointCount - 1) / 2.0;
								C3 = C2 * (double) (PointCount - 2) / 3.0;
								C4 = C3 * (double) (PointCount - 3) / 4.0;
								IntL1 = IntL1 / C1;
								IntL2 = IntL2 / C2 / 2.0;
								IntL3 = IntL3 / C3 / 3.0;
								IntL4 = IntL4 / C4 / 4.0;

								// if computing cover, do processing
								if (ComputeCover) {
									FirstReturnsAbove = 0;
									FirstReturnsTotal = 0;
									AllReturnsAbove = 0;
									AllReturnsTotal = 0;

									// read all returns and do statistics
									dat.Rewind();
									while (dat.ReadNextRecord(&pt)) {
										if (m_UseAllReturnsForCover) {
											if (pt.Elevation > CoverCutoff)
												AllReturnsAbove ++;

											AllReturnsTotal ++;
										}
										
										if (pt.ReturnNumber == 1) {
											if (pt.Elevation > CoverCutoff)
												FirstReturnsAbove ++;

											FirstReturnsTotal ++;
										}
									}
									
									if (m_UseAllReturnsForCover) {
										if (AllReturnsTotal)
											AllCover = (double) AllReturnsAbove / (double) AllReturnsTotal * 100.0;
										else
											AllCover = -1.0;

										if (FirstReturnsTotal)
											AllFirstCover = (double) AllReturnsAbove / (double) FirstReturnsTotal * 100.0;
										else
											AllFirstCover = -1.0;
									}

									if (FirstReturnsTotal)
										Cover = (double) FirstReturnsAbove / (double) FirstReturnsTotal * 100.0;
									else
										Cover = -1.0;
								}
								else
									Cover = -1.0;

								double FirstCoverMean = 0.0;
								double FirstCoverMode = 0.0;
								double AllCoverMean = 0.0;
								double AllCoverMode = 0.0;
								double AllFirstCoverMean = 0.0;
								double AllFirstCoverMode = 0.0;
								int AllAboveMean = 0;
								int AllAboveMode = 0;
								int FirstAboveMean = 0;
								int FirstAboveMode = 0;
								if (m_ComputeRelCover) {
									// compute cover relative to the mean, mode, and percentile heights
									FirstReturnsTotal = 0;
									AllReturnsTotal = 0;

									// read all returns and do statistics
									dat.Rewind();
									while (dat.ReadNextRecord(&pt)) {
										if (m_UseAllReturnsForCover) {
											if (pt.Elevation > ElevMean)
												AllAboveMean ++;
											if (pt.Elevation > ElevMode)
												AllAboveMode ++;

											AllReturnsTotal ++;
										}
										
										if (pt.ReturnNumber == 1) {
											if (pt.Elevation > ElevMean)
												FirstAboveMean ++;
											if (pt.Elevation > ElevMode)
												FirstAboveMode ++;

											FirstReturnsTotal ++;
										}
									}
									
									if (AllReturnsTotal) {
										AllCoverMean = (double) AllAboveMean / (double) AllReturnsTotal * 100.0;
										AllCoverMode = (double) AllAboveMode / (double) AllReturnsTotal * 100.0;
									}
									else {
										AllCoverMean = -1.0;
										AllCoverMode = -1.0;
									}

									if (FirstReturnsTotal) {
										AllFirstCoverMean = (double) AllAboveMean / (double) FirstReturnsTotal * 100.0;
										AllFirstCoverMode = (double) AllAboveMode / (double) FirstReturnsTotal * 100.0;
										FirstCoverMean = (double) FirstAboveMean / (double) FirstReturnsTotal * 100.0;
										FirstCoverMode = (double) FirstAboveMode / (double) FirstReturnsTotal * 100.0;
									}
									else {
										AllFirstCover = -1.0;
										AllFirstCover = -1.0;
										FirstCoverMean = -1.0;
										FirstCoverMode = -1.0;
									}
								}

								// report stuff to csv file
								if (ParseID) 
									fprintf(f, "%i,", ID);

								if (m_ProduceHighpointOutput) {
									fprintf(f, "\"%s\",%i,%lf,%lf,%lf\n", ce.m_FileName, PointCount, HighX, HighY, HighElevation);
								}
								else if (m_ProduceYZLiOutput) {
									fprintf(f, "\"%s\",%i,%lf,%lf,%lf,%lf\n", ce.m_FileName, PointCount, ElevMean, ElevStdDev, ElevP75, Cover);
								}
								else {
									// print base values
									fprintf(f, "\"%s\",%s,%i", ce.m_FileName, TempfsForFileTitle.FileTitle(), PointCount);

									if (m_CountReturns) {
										for (k = 0; k < 10; k ++)
											fprintf(f, ",%i", ReturnCounts[k]);
									}

									fprintf(f, ",%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", 
											ElevMin, ElevMax, ElevMean, ElevMode, ElevStdDev, ElevVariance, ElevStdDev / ElevMean, ElevIQDist, ElevSkewness, ElevKurtosis, ElevAAD, 
											ElevL1, ElevL2, ElevL3, ElevL4, ElevL2 / ElevL1, ElevL3 / ElevL2, ElevL4 / ElevL2,
											ElevP01, ElevP05, ElevP10, ElevP20, ElevP25, ElevP30, ElevP40, ElevP50, ElevP60, ElevP70, ElevP75, ElevP80, ElevP90, ElevP95, ElevP99,
											IntMin, IntMax, IntMean, IntMode, IntStdDev, IntVariance, IntStdDev / IntMean, IntIQDist, IntSkewness, IntKurtosis, IntAAD, 
											IntL1, IntL2, IntL3, IntL4, IntL2 / IntL1, IntL3 / IntL2, IntL4 / IntL2,
											IntP01, IntP05, IntP10, IntP20, IntP25, IntP30, IntP40, IntP50, IntP60, IntP70, IntP75, IntP80, IntP90, IntP95, IntP99);

									if (ComputeCover) {
										fprintf(f, ",%lf", Cover);
										
										if (m_UseAllReturnsForCover) {
											fprintf(f, ",%lf,%lf", AllCover, AllFirstCover);
											fprintf(f, ",%i,%i", FirstReturnsAbove, AllReturnsAbove);
										}
									}
									
									if (m_ComputeRelCover) {
										fprintf(f, ",%lf,%lf,%lf,%lf,%lf,%lf", FirstCoverMean, FirstCoverMode, AllCoverMean, AllCoverMode, AllFirstCoverMean, AllFirstCoverMode);
										fprintf(f, ",%i,%i,%i,%i,%i,%i", FirstAboveMean, FirstAboveMode, AllAboveMean, AllAboveMode, FirstReturnsTotal, AllReturnsTotal);
									}

									// print end of line
									fprintf(f, "\n");
								}

								// clean up
								delete [] ElevValueList;
								delete [] IntValueList;

								// status info
								csTemp.Format("   %s: %i points", ce.m_FileName, PointCount);
								LTKCL_PrintStatus(csTemp);
							}
							else if (PointCount >= 1) {
								// only report min, max, mean
								
								// report stuff to csv file
								if (ParseID) 
									fprintf(f, "%i,", ID);

								if (m_ProduceHighpointOutput) {
									fprintf(f, "\"%s\",%i,%lf,%lf,%lf\n", ce.m_FileName, PointCount, HighX, HighY, HighElevation);
								}
								else if (m_ProduceYZLiOutput) {
									fprintf(f, "\"%s\",%i,%lf,%lf,%lf,%lf\n", ce.m_FileName, PointCount, ElevMean, ElevStdDev, ElevP75, Cover);
								}
								else {
									// print base values
									fprintf(f, "\"%s\",%s,%i", ce.m_FileName, TempfsForFileTitle.FileTitle(), PointCount);
	//								fprintf(f, "\"%s\",%i", ce.m_FileName, PointCount);

									if (m_CountReturns) {
										for (k = 0; k < 10; k ++)
											fprintf(f, ",%i", ReturnCounts[k]);
									}

									fprintf(f, ",%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", 
											ElevMin, ElevMax, ElevMean, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
											0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
											0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
											IntMin, IntMax, IntMean, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
											0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
											0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

									if (ComputeCover) {
										fprintf(f, ",%lf", 0.0);

										if (m_UseAllReturnsForCover) {
											fprintf(f, ",0.0,0.0");
											fprintf(f, ",0,0");
										}
									}

									if (m_ComputeRelCover) {
										fprintf(f, ",0.0,0.0,0.0,0.0,0.0,0.0");
										fprintf(f, ",0,0,0,0,0,0");
									}

									fprintf(f, "\n");
								}
							}
							else {
								// not enough points...print zeros to output file
								if (ParseID) 
									fprintf(f, "0,");

								if (m_ProduceHighpointOutput) {
									fprintf(f, "\"%s\",%i,0.0,0.0,0.0\n", ce.m_FileName, PointCount);
								}
								else if (m_ProduceYZLiOutput) {
									fprintf(f, "\"%s\",%i,%lf,%lf,%lf,%lf\n", ce.m_FileName, PointCount, 0.0, 0.0, 0.0, 0.0);
								}
								else {
									fprintf(f, "\"%s\",%s,%i", ce.m_FileName, TempfsForFileTitle.FileTitle(), PointCount);
	//								fprintf(f, "\"%s\",%i", ce.m_FileName, PointCount);

									if (m_CountReturns) {
										for (k = 0; k < 10; k ++)
											fprintf(f, ",%i", ReturnCounts[k]);
									}

									fprintf(f, ",0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0");

									if (ComputeCover) {
										fprintf(f, ",0.0");

										if (m_UseAllReturnsForCover) {
											fprintf(f, ",0.0,0.0");
											fprintf(f, ",0,0");
										}
									}

									if (m_ComputeRelCover) {
										fprintf(f, ",0.0,0.0,0.0,0.0,0.0,0.0");
										fprintf(f, ",0,0,0,0,0,0");
									}

									fprintf(f, "\n");
								}

								// status info
								csTemp.Format("***ERROR: Metrics NOT computed for %s...not a valid data file", ce.m_FileName);
								LTKCL_PrintStatus(csTemp);
							}
						}
						else {
							// bad file...print zeros to output file
							if (ParseID) 
								fprintf(f, "0,");

							if (m_ProduceHighpointOutput) {
								fprintf(f, "\"%s\",%i,0.0,0.0,0.0\n", ce.m_FileName, PointCount);
							}
							else if (m_ProduceYZLiOutput) {
								fprintf(f, "\"%s\",%i,%lf,%lf,%lf,%lf\n", ce.m_FileName, PointCount, 0.0, 0.0, 0.0, 0.0);
							}
							else {
								fprintf(f, "\"%s\",%s,%i", ce.m_FileName, TempfsForFileTitle.FileTitle(), PointCount);
	//							fprintf(f, "\"%s\",%i", ce.m_FileName, PointCount);

								if (m_CountReturns) {
									for (k = 0; k < 10; k ++)
										fprintf(f, ",%i", ReturnCounts[k]);
								}

								//                                                1                                       2                                       3                                       4                                       5
								//            1   2   3   4   5   6   7   8   9   0   1   2   3   4   5   6   7   8   9   0   1   2   3   4   5   6   7   8   9   0   1   2   3   4   5   6   7   8   9   0   1   2   3   4   5   6   7   8   9   0   1   2
								fprintf(f, ",0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0");

								if (ComputeCover) {
									fprintf(f, ",0.0");

									if (m_UseAllReturnsForCover) {
										fprintf(f, ",%lf,%lf", 0.0, 0.0);
										fprintf(f, ",%i,%i", 0, 0);
									}
								}

								if (m_ComputeRelCover) {
									fprintf(f, ",%lf,%lf,%lf,%lf,%lf,%lf", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
									fprintf(f, ",%i,%i,%i,%i,%i,%i", 0, 0, 0, 0, 0, 0);
								}

								fprintf(f, "\n");
							}

							// status info
							csTemp.Format("***ERROR: Metrics NOT computed for %s...not a valid data file", ce.m_FileName);
							LTKCL_PrintStatus(csTemp);
						}
					}
					fclose(f);

					LTKCL_ReportProductFile(OutputFileCL, "CloudMetrics output");
				}
			}
			LTKCL_PrintEndReport(m_nRetCode);
		}
	}

	return m_nRetCode;
}

#include "command_line_core_functions.cpp"
