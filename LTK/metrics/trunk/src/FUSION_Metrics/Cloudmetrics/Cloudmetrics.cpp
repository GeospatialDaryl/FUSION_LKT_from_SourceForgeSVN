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
//	Added new options: /strata:[#,#,#,...] /intstrata:[#,#,#,...] /kde:[window, multiplier]
//
//	Added new metrics: Canopy relief ratio, height strata (elevation and intensity metrics), MAD_MED, MAD_MODE, #modes
//	from a kernal density function using the return heights, min/max mode values, min/max range
//
//	5/4/2011 CloudMetrics V2.36
//	Added proportion of returns in strata when using the /strata option. Also corrected a minor error in the output of 
//	intensity metrics for the strate
//
//#include "stdafx.h"

#include <algorithm>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include <time.h>
#include <float.h>
#include <math.h>
#include "../../fusion/versionID.h"
#include "lidardata.h"
#include "dataindex.h"
#include "filespec.h"
#include "plansdtm.h"
#include "argslib.h"
//#include <sys/types.h>
//#include <sys/stat.h>
#include "DataCatalogEntry.h"
#include "Command_line_core_functions.h"

#define		PROGRAM_NAME		"CloudMetrics"
#define		PROGRAM_VERSION		2.36
#define		MINIMUM_CL_PARAMS	2
//			                              1         2         3         4         5         6         7         8         9         10
//			                     1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
#define		PROGRAM_DESCRIPTION	"Computes metrics describing an entire point cloud"

#define		NUMBEROFBINS		64
#define		NUMBER_OF_CLASS_CODES		32
#define		MAX_NUMBER_OF_STRATA		32

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

//CWinApp theApp;

using namespace std;

typedef struct {
	float Elevation;
	float Intensity;
	int ReturnNumber;
} STRATAPOINT;

char* ValidCommandLineSwitches = "above new firstinpulse firstreturn first highpoint subset id minht maxht outlier strata intstrata kde";

// global variables...not the best programming practice but helps with a "standard" template for command line utilities
std::vector<CDataCatalogEntry> DataFiles;
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

BOOL m_DoKDEStats;
double m_KDEBandwidthMultiplier;
double m_KDEWindowSize;

BOOL m_DoHeightStrata;
BOOL m_DoHeightStrataIntensity;
int m_HeightStrataCount;
int m_HeightStrataIntensityCount;
double m_HeightStrata[MAX_NUMBER_OF_STRATA];
double m_HeightStrataIntensity[MAX_NUMBER_OF_STRATA];

double m_MinHeight;
double m_MaxHeight;
double CoverCutoff;
CString InputFileCL;
CString OutputFileCL;
CFileSpec DataFS;
CFileSpec OutputFS;

// global functions that are modified to create a new program
void GaussianKDE(float* PointData, int Pts, double BW, double SmoothWindow, int& ModeCount, double& MinMode, double& MaxMode);

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
	printf("              be used but /minht is preferred. The minht is not used when\n");
	printf("              computing metrics related to the /strata and /intstrata options.\n");
	printf("  maxht:#     Use only returns below # (use when data is normalized to ground)\n");
	printf("              to compute metrics. The maxht is not used when computing metrics\n");
	printf("              related to the /strata or /intstrata options.\n");
	printf("  outlier:low,high Omit points with elevations below low and above high.\n");
	printf("              When used with data that has been normalized using a ground\n");
	printf("              surface, low and high are interpreted as heights above ground.\n");
	printf("              Using /outlier:low,high has the same effect as using both\n");
	printf("              /minht:low and /maxht:high. The values specified using the\n");
	printf("              /outlier switch will override the values specified using the\n");
	printf("              /minht and /maxht switches.\n");
	printf("  strata:[#,#,...] Count returns in various height strata. # gives the upper\n");
	printf("              limit for each strata. Returns are counted if their height\n");
	printf("              is >= the lower limit and < the upper limit. The first strata\n");
	printf("              contains points < the first limit. The last strata contains\n");
	printf("              points >= the last limit. Default strata: 0.15, 1.37, 5, 10,\n");
	printf("              20, 30.\n");
	printf("  intstrata:[#,#,...] Compute metrics using the intensity values in various\n");
	printf("              height strata. Strata for intensity metrics are defined in\n");
	printf("              the same way as the /strata option. Default strata: 0.25, 1.37.\n");
	printf("  kde:[window,mult] Compute the number of modes and minimum and maximum node\n");
	printf("              using a kernal density estimator. Window is the width of a\n");
	printf("              moving average smoothing window in data units and mult is a\n");
	printf("              multiplier for the bandwidth parameter of the KDE. Default\n");
	printf("              window is 2.5 and the multiplier is 1.0\n");
//	printf("  alldensity  Use all returns to compute canopy and relative cover values.\n");
//	printf("  relcover    Compute proportion of first returns above the mean and mode\n");
//	printf("              of return elevations (or heights)\n");
	printf("\nColumn labels will only be correct if the output file does not exist or if\n");
	printf("the /new switch is specified. Appending output from runs with different\n");
	printf("options will result in misaligned columns.\n");
	printf("When using /first, /firstreturn, or /firstinpulse, column headings do not\n");
	printf("indicate that only first returns were used so it is up to the user to\n");
	printf("keep track of the returns used to compute the metrics.\n");

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

	m_DoKDEStats = FALSE;

	m_DoHeightStrata = FALSE;
	m_DoHeightStrataIntensity = FALSE;
	m_HeightStrataCount = 0;
	m_HeightStrataIntensityCount = 0;

	m_CountReturns = TRUE;		// always put out counts (new in V2.0)
}

int PresentInteractiveDialog()
{
	if (m_RunInteractive) {
		usage();
		printf("\n%s - Interactive mode is not currently implemented\n", PROGRAM_NAME);
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
	m_DoKDEStats = m_clp.Switch("kde");

	if (m_DoKDEStats) {
		CString temp = m_clp.GetSwitchStr("kde", "");
		if (!temp.IsEmpty()) {
			sscanf(temp, "%lf,%lf", &m_KDEWindowSize, &m_KDEBandwidthMultiplier);
		}
		else {
			// default
			m_KDEBandwidthMultiplier = 1.0;
			m_KDEWindowSize = 2.5;
		}
	}

	// check for height strata options
	m_DoHeightStrata = m_clp.Switch("strata");
	if (m_DoHeightStrata) {
		CString temp = m_clp.GetSwitchStr("strata", "");
		if (!temp.IsEmpty()) {
			m_HeightStrataCount = 0;
			std::string switchValue(temp);
			std::vector<std::string> heights;
			boost::algorithm::split(heights, switchValue, boost::algorithm::is_any_of(","));
			BOOST_FOREACH(const std::string & height, heights) {
				m_HeightStrata[m_HeightStrataCount] = atof(height.c_str());
				m_HeightStrataCount ++;

				// check number of strata
				if (m_HeightStrataCount >= MAX_NUMBER_OF_STRATA) {
					LTKCL_PrintStatus("Too many strata for /strata...maximum is 31");
					return(1);

					// need break if we add a return variable
//					break;
				}
			}

			// add a final strata to define upper bound
			m_HeightStrata[m_HeightStrataCount] = DBL_MAX;
			m_HeightStrataCount ++;
		}
		else {
			// default
			m_HeightStrataCount = 7;
			m_HeightStrata[0] = 0.15;
			m_HeightStrata[1] = 1.37;
			m_HeightStrata[2] = 5.0;
			m_HeightStrata[3] = 10.0;
			m_HeightStrata[4] = 20.0;
			m_HeightStrata[5] = 30.0;
			m_HeightStrata[6] = DBL_MAX;

		}
	}

	m_DoHeightStrataIntensity = m_clp.Switch("intstrata");
	if (m_DoHeightStrataIntensity) {
		CString temp = m_clp.GetSwitchStr("intstrata", "");
		if (!temp.IsEmpty()) {
			m_HeightStrataIntensityCount = 0;
			std::string switchValue(temp);
			std::vector<std::string> heights;
			boost::algorithm::split(heights, switchValue, boost::algorithm::is_any_of(","));
			BOOST_FOREACH(const std::string & height, heights) {
				m_HeightStrataIntensity[m_HeightStrataIntensityCount] = atof(height.c_str());
				m_HeightStrataIntensityCount ++;
				
				if (m_HeightStrataIntensityCount >= MAX_NUMBER_OF_STRATA) {
					LTKCL_PrintStatus("Too many strata for /intstrata...maximum is 31");
					return(1);

					// need break if we add a return variable
//					break;
				}
			}
		
			// add a final strata to define upper bound
			m_HeightStrataIntensity[m_HeightStrataIntensityCount] = DBL_MAX;
			m_HeightStrataIntensityCount ++;
		}
		else {
			// default
			m_HeightStrataIntensityCount = 3;
			m_HeightStrataIntensity[0] = 0.15;
			m_HeightStrataIntensity[1] = 1.37;
			m_HeightStrataIntensity[2] = DBL_MAX;
		}
	}

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

	int LastInputFileIndex = m_clp.ParamCount() - 2; // 2nd to last (which is output file)
	int InputFileCount = LastInputFileIndex - m_clp.FirstNonSwitchIndex() + 1;

	// get output file specifier (last parameter)
	OutputFileCL = m_clp.ParamStr(LastInputFileIndex + 1);

	// check to see if the output file exists...if not, set the NewOutputFile flag to include headers
	// in the output file
	OutputFS.SetFullSpec(OutputFileCL);
	if (!OutputFS.Exists())
		NewOutputFile = TRUE;

	// build list of input data files...may contain 1 or more files
	if (DataFS.Extension().CompareNoCase(".txt") == 0 && InputFileCount == 1) {
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
					DataFiles.push_back(ce);

					DataFileCount ++;
				}
			}
		}
	}
	else if (DataFS.Extension().CompareNoCase(".csv") == 0 && InputFileCount == 1) {
		// read data files from catalog file
		CDataFile lst(DataFS.GetFullSpec());
		if (lst.IsValid()) {
			char buf[1024];
			CFileSpec TempFS;
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
					// (Calculation appears to be the same as method used by CDataIndex)
					chksum = CDataIndex::ComputeChecksum(ce.m_FileName);

					// if checksum doesn't match clear the information from the catalog entry
					if (ce.m_CheckSum != chksum) {
						ce.m_CheckSum = 0;
						ce.m_MinX = ce.m_MinY = ce.m_MinZ = ce.m_MaxX = ce.m_MaxY = ce.m_MaxZ = 0.0;
						ce.m_PointDensity= -1.0;
						ce.m_Points = -1;
					}

					DataFiles.push_back(ce);
					DataFileCount ++;
				}
			}
		}
	}
	else {
		// collect all the input files listed on the command line
/*
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
*/
			DataFileCount = 0;
/*
			while (bWorking) {
				bWorking = finder.FindNextFile();
*/
			for (int j = m_clp.FirstNonSwitchIndex(); j <= LastInputFileIndex; ++j) {
				ce.m_FileName = m_clp.ParamStr(j);
				ce.m_CheckSum = 0;
				ce.m_MinX = ce.m_MinY = ce.m_MinZ = ce.m_MaxX = ce.m_MaxY = ce.m_MaxZ = 0.0;
				ce.m_PointDensity= -1.0;
				ce.m_Points = -1;
				DataFiles.push_back(ce);

				DataFileCount ++;
			}
/*
		}
		else {
			ce.m_FileName = InputFileCL;
			ce.m_CheckSum = 0;
			ce.m_MinX = ce.m_MinY = ce.m_MinZ = ce.m_MaxX = ce.m_MaxY = ce.m_MaxZ = 0.0;
			ce.m_PointDensity= -1.0;
			ce.m_Points = -1;
			DataFiles.push_back(ce);

			DataFileCount = 1;
		}
*/
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

// comparison function for sorting...must be global to use qsort()
int compareSP(const void *arg1, const void *arg2)
{
	if (((STRATAPOINT*) arg1)->Elevation < (((STRATAPOINT*) arg2)->Elevation))
		return(-1);
	else if (((STRATAPOINT*) arg1)->Elevation > (((STRATAPOINT*) arg2)->Elevation))
		return(1);
	else
		return(0);
}

// comparison function for sorting...must be global to use qsort()
int compareSPint(const void *arg1, const void *arg2)
{
	if (((STRATAPOINT*) arg1)->Intensity < (((STRATAPOINT*) arg2)->Intensity))
		return(-1);
	else if (((STRATAPOINT*) arg1)->Intensity > (((STRATAPOINT*) arg2)->Intensity))
		return(1);
	else
		return(0);
}

int main(int argc, char* argv[])
{
	CString csTemp;

/*
	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		m_nRetCode = 1;
	}
	else
*/
	{
		// initialize toolkit variables...if return value is FALSE, /version was on the command line
		// with /version, only the program name and version information are output
		m_clp.SetCommandLine(argc, argv);
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
				int i, j, k, l;

				// stat variables
				int PointCount;
				int TotalPointCount;
				int TempPointCount;
				int StrataPointCount;
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

				double CanopyReliefRatio, ElevMadMedian, ElevMadMode;
				int KDE_ModeCount;
				double KDE_MaxMode, KDE_MinMode, KDE_ModeRange;
				int ElevStrataCount[MAX_NUMBER_OF_STRATA];
				int ElevStrataCountReturn[MAX_NUMBER_OF_STRATA][11];
				double ElevStrataMean[MAX_NUMBER_OF_STRATA];
				double ElevStrataMin[MAX_NUMBER_OF_STRATA];
				double ElevStrataMax[MAX_NUMBER_OF_STRATA];
				double ElevStrataMedian[MAX_NUMBER_OF_STRATA];
				double ElevStrataMode[MAX_NUMBER_OF_STRATA];
				double ElevStrataSkewness[MAX_NUMBER_OF_STRATA];
				double ElevStrataKurtosis[MAX_NUMBER_OF_STRATA];
				double ElevStrataVariance[MAX_NUMBER_OF_STRATA];
				double ElevStrataM2[MAX_NUMBER_OF_STRATA];		// used to compute variance
				double ElevStrataM3[MAX_NUMBER_OF_STRATA];		// used to compute skewness & kurtosis
				double ElevStrataM4[MAX_NUMBER_OF_STRATA];		// used to compute skewness & kurtosis
				
				int IntStrataCount[MAX_NUMBER_OF_STRATA];
				int IntStrataCountReturn[MAX_NUMBER_OF_STRATA][11];
				double IntStrataMean[MAX_NUMBER_OF_STRATA];
				double IntStrataMin[MAX_NUMBER_OF_STRATA];
				double IntStrataMax[MAX_NUMBER_OF_STRATA];
				double IntStrataMedian[MAX_NUMBER_OF_STRATA];
				double IntStrataMode[MAX_NUMBER_OF_STRATA];
				double IntStrataSkewness[MAX_NUMBER_OF_STRATA];
				double IntStrataKurtosis[MAX_NUMBER_OF_STRATA];
				double IntStrataVariance[MAX_NUMBER_OF_STRATA];
				double IntStrataM2[MAX_NUMBER_OF_STRATA];		// used to compute variance
				double IntStrataM3[MAX_NUMBER_OF_STRATA];		// used to compute skewness & kurtosis
				double IntStrataM4[MAX_NUMBER_OF_STRATA];		// used to compute skewness & kurtosis
				double delta;
				double delta_n;
				double delta_n2;
				double term1;
				int n1;

				int FirstReturnsAbove;
				int FirstReturnsTotal;
				int AllReturnsAbove;
				int AllReturnsTotal;
				float* ElevValueList;
				float* IntValueList;
				STRATAPOINT* StrataPointList;
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

				// echo strata info
				// verbose status with strata...don't print the last one
				char ts[1024];
				if (m_DoHeightStrata) {
					sprintf(ts, "Elevation strata:");

					for (int i = 0; i < m_HeightStrataCount - 1; i ++) {
						sprintf(ts, "%s %.2lf", ts, m_HeightStrata[i]);
					}
					LTKCL_PrintVerboseStatus(ts);
				}


				// verbose status with strata...don't print the last one
				if (m_DoHeightStrataIntensity) {
					sprintf(ts, "Intensity strata:");

					for (int i = 0; i < m_HeightStrataIntensityCount - 1; i ++) {
						sprintf(ts, "%s %.2lf", ts, m_HeightStrataIntensity[i]);
					}
					LTKCL_PrintVerboseStatus(ts);
				}

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
								fprintf(f, "DataFile,FileTitle,Total return count,Total return count above %.2lf and below %.2lf", m_MinHeight, m_MaxHeight);
							else
								fprintf(f, "DataFile,FileTitle,Total return count,Total return count above %.2lf", m_MinHeight);
						}
						else if (m_UseHeightMax)
							fprintf(f, "DataFile,FileTitle,Total return count,Total return count below %.2lf", m_MaxHeight);
						else
							fprintf(f, "DataFile,FileTitle,Total return count");

						// add headings for return counts
						if (m_CountReturns) {
							if (m_UseHeightMin) {
								if (m_UseHeightMax)
									fprintf(f, ",Return 1 count above %.2lf and below %.2lf,Return 2 count above %.2lf and below %.2lf,Return 3 count above %.2lf and below %.2lf,Return 4 count above %.2lf and below %.2lf,Return 5 count above %.2lf and below %.2lf,Return 6 count above %.2lf and below %.2lf,Return 7 count above %.2lf and below %.2lf,Return 8 count above %.2lf and below %.2lf,Return 9 count above %.2lf and below %.2lf,Other return count above %.2lf and below %.2lf", m_MinHeight, m_MaxHeight, m_MinHeight, m_MaxHeight, m_MinHeight, m_MaxHeight, m_MinHeight, m_MaxHeight, m_MinHeight, m_MaxHeight, m_MinHeight, m_MaxHeight, m_MinHeight, m_MaxHeight, m_MinHeight, m_MaxHeight, m_MinHeight, m_MaxHeight, m_MinHeight, m_MaxHeight);
								else
									fprintf(f, ",Return 1 count above %.2lf,Return 2 count above %.2lf,Return 3 count above %.2lf,Return 4 count above %.2lf,Return 5 count above %.2lf,Return 6 count above %.2lf,Return 7 count above %.2lf,Return 8 count above %.2lf,Return 9 count above %.2lf,Other return count above %.2lf", m_MinHeight, m_MinHeight, m_MinHeight, m_MinHeight, m_MinHeight, m_MinHeight, m_MinHeight, m_MinHeight, m_MinHeight, m_MinHeight);
							}
							else if (m_UseHeightMax)
									fprintf(f, ",Return 1 count below %.2lf,Return 2 count below %.2lf,Return 3 count below %.2lf,Return 4 count below %.2lf,Return 5 count below %.2lf,Return 6 count below %.2lf,Return 7 count below %.2lf,Return 8 count below %.2lf,Return 9 count below %.2lf,Other return count below %.2lf", m_MaxHeight, m_MaxHeight, m_MaxHeight, m_MaxHeight, m_MaxHeight, m_MaxHeight, m_MaxHeight, m_MaxHeight, m_MaxHeight, m_MaxHeight);
							else
								fprintf(f, ",Return 1 count,Return 2 count,Return 3 count,Return 4 count,Return 5 count,Return 6 count,Return 7 count,Return 8 count,Return 9 count,Other return count");
						}

						fprintf(f, ",Elev minimum,Elev maximum,Elev mean,Elev mode,Elev stddev,Elev variance,Elev CV,Elev IQ,Elev skewness,Elev kurtosis,Elev AAD,Elev MAD median,Elev MAD mode,Elev L1,Elev L2,Elev L3,Elev L4,Elev L CV,Elev L skewness,Elev L kurtosis,Elev P01,Elev P05,Elev P10,Elev P20,Elev P25,Elev P30,Elev P40,Elev P50,Elev P60,Elev P70,Elev P75,Elev P80,Elev P90,Elev P95,Elev P99,Canopy relief ratio");
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

						if (m_DoKDEStats) {
							// column headings for KDE stuff
							fprintf(f, ",KDE elev modes,KDE elev min mode,KDE elev max mode,KDE elev mode range");
						}

						if (m_DoHeightStrata) {
							// print column labels...don't use last strata as it was "added" to provide upper bound
							// first strata
							fprintf(f, ",Elev strata (below %.2lf) total return count,Elev strata (below %.2lf) return proportion,Elev strata (below %.2lf) min,Elev strata (below %.2lf) max,Elev strata (below %.2lf) mean,Elev strata (below %.2lf) mode,Elev strata (below %.2lf) median,Elev strata (below %.2lf) stddev,Elev strata (below %.2lf) CV,Elev strata (below %.2lf) skewness,Elev strata (below %.2lf) kurtosis", 
									m_HeightStrata[0], 
									m_HeightStrata[0], 
									m_HeightStrata[0], 
									m_HeightStrata[0], 
									m_HeightStrata[0], 
									m_HeightStrata[0], 
									m_HeightStrata[0], 
									m_HeightStrata[0], 
									m_HeightStrata[0], 
									m_HeightStrata[0], 
									m_HeightStrata[0]);

							for (k = 1; k < m_HeightStrataCount - 1; k ++) {
								fprintf(f, ",Elev strata (%.2lf to %.2lf) total return count,Elev strata (%.2lf to %.2lf) return proportion,Elev strata (%.2lf to %.2lf) min,Elev strata (%.2lf to %.2lf) max,Elev strata (%.2lf to %.2lf) mean,Elev strata (%.2lf to %.2lf) mode,Elev strata (%.2lf to %.2lf) median,Elev strata (%.2lf to %.2lf) stddev,Elev strata (%.2lf to %.2lf) CV,Elev strata (%.2lf to %.2lf) skewness,Elev strata (%.2lf to %.2lf) kurtosis", 
										m_HeightStrata[k - 1], m_HeightStrata[k],
										m_HeightStrata[k - 1], m_HeightStrata[k],
										m_HeightStrata[k - 1], m_HeightStrata[k],
										m_HeightStrata[k - 1], m_HeightStrata[k],
										m_HeightStrata[k - 1], m_HeightStrata[k],
										m_HeightStrata[k - 1], m_HeightStrata[k],
										m_HeightStrata[k - 1], m_HeightStrata[k],
										m_HeightStrata[k - 1], m_HeightStrata[k],
										m_HeightStrata[k - 1], m_HeightStrata[k],
										m_HeightStrata[k - 1], m_HeightStrata[k], 
										m_HeightStrata[k - 1], m_HeightStrata[k]);
							}

							// last strata
							fprintf(f, ",Elev strata (above %.2lf) total return count,Elev strata (above %.2lf) return proportion,Elev strata (above %.2lf) min,Elev strata (above %.2lf) max,Elev strata (above %.2lf) mean,Elev strata (above %.2lf) mode,Elev strata (above %.2lf) median,Elev strata (above %.2lf) stddev,Elev strata (above %.2lf) CV,Elev strata (above %.2lf) skewness,Elev strata (above %.2lf) kurtosis", 
								m_HeightStrata[m_HeightStrataCount - 2], 
								m_HeightStrata[m_HeightStrataCount - 2], 
								m_HeightStrata[m_HeightStrataCount - 2], 
								m_HeightStrata[m_HeightStrataCount - 2], 
								m_HeightStrata[m_HeightStrataCount - 2], 
								m_HeightStrata[m_HeightStrataCount - 2], 
								m_HeightStrata[m_HeightStrataCount - 2], 
								m_HeightStrata[m_HeightStrataCount - 2], 
								m_HeightStrata[m_HeightStrataCount - 2], 
								m_HeightStrata[m_HeightStrataCount - 2], 
								m_HeightStrata[m_HeightStrataCount - 2]);
						}

						if (m_DoHeightStrataIntensity) {
							// print column labels...don't use last strata as it was "added" to provide upper bound
							// first strata
							fprintf(f, ",Int strata (below %.2lf) total return count,Int strata (below %.2lf) return proportion,Int strata (below %.2lf) min,Int strata (below %.2lf) max,Int strata (below %.2lf) mean,Int strata (below %.2lf) mode,Int strata (below %.2lf) median,Int strata (below %.2lf) stddev,Int strata (below %.2lf) CV,Int strata (below %.2lf) skewness,Int strata (below %.2lf) kurtosis", 
									m_HeightStrataIntensity[0], 
									m_HeightStrataIntensity[0], 
									m_HeightStrataIntensity[0], 
									m_HeightStrataIntensity[0], 
									m_HeightStrataIntensity[0], 
									m_HeightStrataIntensity[0], 
									m_HeightStrataIntensity[0], 
									m_HeightStrataIntensity[0], 
									m_HeightStrataIntensity[0], 
									m_HeightStrataIntensity[0], 
									m_HeightStrataIntensity[0]);

							for (k = 1; k < m_HeightStrataIntensityCount - 1; k ++) {
								fprintf(f, ",Int strata (%.2lf to %.2lf) total return count,Int strata (%.2lf to %.2lf) return proportion,Int strata (%.2lf to %.2lf) min,Int strata (%.2lf to %.2lf) max,Int strata (%.2lf to %.2lf) mean,Int strata (%.2lf to %.2lf) mode,Int strata (%.2lf to %.2lf) median,Int strata (%.2lf to %.2lf) stddev,Int strata (%.2lf to %.2lf) CV,Int strata (%.2lf to %.2lf) skewness,Int strata (%.2lf to %.2lf) kurtosis", 
										m_HeightStrataIntensity[k - 1], m_HeightStrataIntensity[k],
										m_HeightStrataIntensity[k - 1], m_HeightStrataIntensity[k],
										m_HeightStrataIntensity[k - 1], m_HeightStrataIntensity[k],
										m_HeightStrataIntensity[k - 1], m_HeightStrataIntensity[k],
										m_HeightStrataIntensity[k - 1], m_HeightStrataIntensity[k],
										m_HeightStrataIntensity[k - 1], m_HeightStrataIntensity[k],
										m_HeightStrataIntensity[k - 1], m_HeightStrataIntensity[k],
										m_HeightStrataIntensity[k - 1], m_HeightStrataIntensity[k],
										m_HeightStrataIntensity[k - 1], m_HeightStrataIntensity[k],
										m_HeightStrataIntensity[k - 1], m_HeightStrataIntensity[k], 
										m_HeightStrataIntensity[k - 1], m_HeightStrataIntensity[k]);
							}

							// last strata
							fprintf(f, ",Int strata (above %.2lf) total return count,Int strata (above %.2lf) return proportion,Int strata (above %.2lf) min,Int strata (above %.2lf) max,Int strata (above %.2lf) mean,Int strata (above %.2lf) mode,Int strata (above %.2lf) median,Int strata (above %.2lf) stddev,Int strata (above %.2lf) CV,Int strata (above %.2lf) skewness,Int strata (above %.2lf) kurtosis", 
								m_HeightStrataIntensity[m_HeightStrataIntensityCount - 2], 
								m_HeightStrataIntensity[m_HeightStrataIntensityCount - 2], 
								m_HeightStrataIntensity[m_HeightStrataIntensityCount - 2], 
								m_HeightStrataIntensity[m_HeightStrataIntensityCount - 2], 
								m_HeightStrataIntensity[m_HeightStrataIntensityCount - 2], 
								m_HeightStrataIntensity[m_HeightStrataIntensityCount - 2], 
								m_HeightStrataIntensity[m_HeightStrataIntensityCount - 2], 
								m_HeightStrataIntensity[m_HeightStrataIntensityCount - 2], 
								m_HeightStrataIntensity[m_HeightStrataIntensityCount - 2], 
								m_HeightStrataIntensity[m_HeightStrataIntensityCount - 2], 
								m_HeightStrataIntensity[m_HeightStrataIntensityCount - 2]);
						}

						// print end of line for header
						fprintf(f, "\n");
					}
				}
				else
					f = fopen(OutputFileCL, "at");

				if (f) {
					// iterate through the list of data files and compute metrics
					for (i = 0; i < int(DataFiles.size()); i ++) {
						ce = DataFiles[i];

						PointCount = 0;
						TotalPointCount = 0;

						// intialize return counts
						for (k = 0; k < 10; k ++)
							ReturnCounts[k] = 0;

						// initialize strata counts and std dev (will be used to accumulate values)
						for (k = 0; k < MAX_NUMBER_OF_STRATA; k ++) {
							ElevStrataCount[k] = 0;
							ElevStrataVariance[k] = 0.0;
							ElevStrataMean[k] = 0.0;
							ElevStrataMin[k] = DBL_MAX;
							ElevStrataMax[k] = DBL_MIN;
							ElevStrataMedian[k] = 0.0;
							ElevStrataMode[k] = 0.0;
							ElevStrataSkewness[k] = 0.0;
							ElevStrataKurtosis[k] = 0.0;
							ElevStrataM2[k] = 0.0;
							ElevStrataM3[k] = 0.0;
							ElevStrataM4[k] = 0.0;
							for (j = 0; j < 10; j ++)
								ElevStrataCountReturn[k][j] = 0;

							IntStrataCount[k] = 0;
							IntStrataVariance[k] = 0.0;
							IntStrataMean[k] = 0.0;
							IntStrataMin[k] = DBL_MAX;
							IntStrataMax[k] = DBL_MIN;
							IntStrataMedian[k] = 0.0;
							IntStrataMode[k] = 0.0;
							IntStrataSkewness[k] = 0.0;
							IntStrataKurtosis[k] = 0.0;
							IntStrataM2[k] = 0.0;
							IntStrataM3[k] = 0.0;
							IntStrataM4[k] = 0.0;
							for (j = 0; j < 10; j ++)
								IntStrataCountReturn[k][j] = 0;
						}

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
							CanopyReliefRatio = 0.0;
							LastReturn = 9999;

							// read all returns and do simple statistics
							while (dat.ReadNextRecord(&pt)) {
								TotalPointCount ++;

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

							// must have at least 4 points for most metrics
							if (PointCount >= 4) {
								// compute canopy relief ratio...doesn't have any meaning with less than 4 points
								if (ElevMax != ElevMin)
									CanopyReliefRatio = (ElevMean - ElevMin) / (ElevMax - ElevMin);
								else
									CanopyReliefRatio = 0.0;

								// allocate memory for list to get median and Quartile values
								ElevValueList = new float[TotalPointCount];
								IntValueList = new float[TotalPointCount];

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
									// compute cover relative to the mean and mode
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

// ****************************
								// compute kernal density stuff
								if (m_DoKDEStats) {
									// should be able to use ElevValueList...it has sorted elevation values
									double BW;
									BW = 0.9 * (min(ElevStdDev, ElevIQDist) / 1.34) * pow((double) PointCount, -0.2);
									GaussianKDE(ElevValueList, PointCount, BW * m_KDEBandwidthMultiplier, m_KDEWindowSize, KDE_ModeCount, KDE_MinMode, KDE_MaxMode);
									KDE_ModeRange = KDE_MaxMode - KDE_MinMode;
								}

								// go through data and compute median absolute deviations from median and mode
								// use ElevValueList and IntValueList to accumulate values
								TempPointCount = 0;
								dat.Rewind();
								while (dat.ReadNextRecord(&pt)) { 
									if (m_UseHeightMin && pt.Elevation <= m_MinHeight) {
										LastReturn = pt.ReturnNumber;
										continue;
									}

									if (m_UseHeightMax && pt.Elevation >= m_MaxHeight) {
										LastReturn = pt.ReturnNumber;
										continue;
									}

									if (m_UseFirstReturns && pt.ReturnNumber > 1) {
										LastReturn = pt.ReturnNumber;
										continue;
									}

									if (m_UseFirstReturnInPulse && (pt.ReturnNumber > 1 && pt.ReturnNumber > LastReturn)) {
										LastReturn = pt.ReturnNumber;
										continue;
									}

									ElevValueList[TempPointCount] = (float) fabs((double) pt.Elevation - ElevMedian);
									IntValueList[TempPointCount] = (float) fabs((double) pt.Elevation - ElevMode);

									TempPointCount ++;
								}

								// sort value list
								qsort(ElevValueList, (size_t) TempPointCount, sizeof(float), compareflt);
								qsort(IntValueList, (size_t) TempPointCount, sizeof(float), compareflt);

								// compute median values
								// source http://www.resacorp.com/quartiles.htm...method 5
								WholePart = (int) ((float) (TempPointCount - 1) * 0.5);
								Fraction = ((float) (TempPointCount - 1) * 0.5) - WholePart;
							
								if (Fraction == 0.0) {
									ElevMadMedian = ElevValueList[WholePart];
									ElevMadMode = IntValueList[WholePart];
								}
								else {
									ElevMadMedian = ElevValueList[WholePart] + Fraction * (ElevValueList[WholePart + 1] - ElevValueList[WholePart]);
									ElevMadMode = IntValueList[WholePart] + Fraction * (IntValueList[WholePart + 1] - IntValueList[WholePart]);
								}

								// allocate space for a list of point elevations and intensity values
								if (m_DoHeightStrata || m_DoHeightStrataIntensity) {
									StrataPointList = (STRATAPOINT*) new STRATAPOINT[TotalPointCount];

									// fill list with points
									dat.Rewind();
									LastReturn = 9999;
									StrataPointCount = 0;
									while (dat.ReadNextRecord(&pt)) { 
										if (m_UseFirstReturns && pt.ReturnNumber > 1) {
											LastReturn = pt.ReturnNumber;
											continue;
										}

										if (m_UseFirstReturnInPulse && (pt.ReturnNumber > 1 && pt.ReturnNumber > LastReturn)) {
											LastReturn = pt.ReturnNumber;
											continue;
										}

										StrataPointList[StrataPointCount].Elevation = pt.Elevation;
										StrataPointList[StrataPointCount].Intensity = pt.Intensity;
										StrataPointList[StrataPointCount].ReturnNumber = pt.ReturnNumber;

										StrataPointCount ++;
										LastReturn = pt.ReturnNumber;
									}

									// sort value list
									qsort(StrataPointList, (size_t) StrataPointCount, sizeof(STRATAPOINT), compareSP);
								}

								// go back through data and compute height strata metrics...first do elevation strata
								if (m_DoHeightStrata) {
									for (l = 0; l < StrataPointCount; l ++) {
										// figure out which strata the point is in, accumulate counts and compute mean and variance
										// algorithm for 1-pass calculation of mean and std dev from: http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
										// modified algorithm to use (n - 1) instead of n (in final calculation outside loop) for kurtosis and skewness to match
										// the values computed in "old" logic (non-strata)
										for (k = 0; k < m_HeightStrataCount; k ++) {
											if (StrataPointList[l].Elevation < m_HeightStrata[k]) {
												// ElevStrataCount[] has total return count
												n1 = ElevStrataCount[k];
												ElevStrataCount[k] ++;

												// compute mean, variance, skewness, and kurtosis
												delta = (double) StrataPointList[l].Elevation - ElevStrataMean[k];
												delta_n = delta / (double) ElevStrataCount[k];
												delta_n2 = delta_n * delta_n;

												term1 = delta * delta_n * (double) n1;
												ElevStrataMean[k] += delta_n;
												ElevStrataM4[k] += term1 * delta_n2 * ((double) ElevStrataCount[k] * (double) ElevStrataCount[k] - 3.0 * (double) ElevStrataCount[k] + 3.0) + 6.0 * delta_n2 * ElevStrataM2[k] - 4.0 * delta_n * ElevStrataM3[k];
												ElevStrataM3[k] += term1 * delta_n * ((double) ElevStrataCount[k] - 2.0) - 3.0 * delta_n * ElevStrataM2[k];
												ElevStrataM2[k] += term1;

												// bins 0-8 have counts for returns 1-9
												// bin 9 has count of "other" returns
												if (StrataPointList[l].ReturnNumber < 10 && StrataPointList[l].ReturnNumber > 0) {
													ElevStrataCountReturn[k][StrataPointList[l].ReturnNumber - 1] ++;
												}
												else {
													ElevStrataCountReturn[k][9] ++;
												}

												// do min/max
												ElevStrataMin[k] = std::min<double>(ElevStrataMin[k], StrataPointList[l].Elevation);
												ElevStrataMax[k] = std::max<double>(ElevStrataMax[k], StrataPointList[l].Elevation);

												break;
											}
										}
									}

									// compute the final values
									for (k = 0; k < m_HeightStrataCount; k ++) {
										if (ElevStrataCount[k]) {
											ElevStrataVariance[k] = ElevStrataM2[k] / (double) (ElevStrataCount[k] - 1);

											// kurtosis and skewness use (n - 1) to match values computed for entire point cloud
//											ElevStrataKurtosis[k] = ((double) ElevStrataCount[k] * ElevStrataM4[k]) / (ElevStrataM2[k] * ElevStrataM2[k]) - 3.0;
											ElevStrataKurtosis[k] = (((double) ElevStrataCount[k] - 1.0) * ElevStrataM4[k]) / (ElevStrataM2[k] * ElevStrataM2[k]);
											ElevStrataSkewness[k] = (sqrt((double) ElevStrataCount[k] - 1.0) * ElevStrataM3[k]) / sqrt(ElevStrataM2[k] * ElevStrataM2[k] * ElevStrataM2[k]);
										}
									}

									// compute median and mode
									TempPointCount = 0;
									for (k = 0; k < m_HeightStrataCount; k ++) {
										if (ElevStrataCount[k]) {
											WholePart = (int) ((float) (ElevStrataCount[k] - 1) * 0.5);
											Fraction = ((float) (ElevStrataCount[k] - 1) * 0.5) - WholePart;
										
											WholePart = TempPointCount + WholePart;

											if (Fraction == 0.0) {
												ElevStrataMedian[k] = StrataPointList[WholePart].Elevation;
											}
											else {
												ElevStrataMedian[k] = StrataPointList[WholePart].Elevation + Fraction * (StrataPointList[WholePart + 1].Elevation - StrataPointList[WholePart].Elevation);
											}
										}
										else {
											ElevStrataMedian[k] = -9999.0;
										}

										TempPointCount += ElevStrataCount[k];
									}

									// figure out the mode using 64 bins for data
									// count values using bins
									TempPointCount = 0;
									for (k = 0; k < m_HeightStrataCount; k ++) {
										if (ElevStrataCount[k]) {
											for (l = 0; l < NUMBEROFBINS; l ++)
												Bins[l] = 0;

											for (l = TempPointCount; l < TempPointCount + ElevStrataCount[k]; l ++) {
												TheBin = (int) ((((double) StrataPointList[l].Elevation - ElevStrataMin[k]) / (ElevStrataMax[k] - ElevStrataMin[k])) * (double) (NUMBEROFBINS - 1));
												Bins[TheBin] ++;
											}

											// find most frequent value
											MaxCount = -1;
											for (l = 0; l < NUMBEROFBINS; l ++) {
												if (Bins[l] > MaxCount) {
													MaxCount = Bins[l];
													TheBin = l;
												}
											}

											// compute mode by un-scaling the bin number
											ElevStrataMode[k] = ElevStrataMin[k] + ((double) TheBin / (double) (NUMBEROFBINS - 1)) * (ElevStrataMax[k] - ElevStrataMin[k]);
										}
										else {
											ElevStrataMode[k] = -9999.0;
										}

										TempPointCount += ElevStrataCount[k];
									}
								}

								// do intensity strata
								if (m_DoHeightStrataIntensity) {
									for (l = 0; l < StrataPointCount; l ++) {
										// figure out which strata the point is in, accumulate counts and compute mean and variance
										// algorithm for 1-pass calculation of mean and std dev from: http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
										for (k = 0; k < m_HeightStrataIntensityCount; k ++) {
											if (StrataPointList[l].Elevation < m_HeightStrataIntensity[k]) {
												// ElevStrataCount[] has total return count
												n1 = IntStrataCount[k];
												IntStrataCount[k] ++;

												// compute mean, variance, skewness, and kurtosis
												delta = (double) StrataPointList[l].Intensity - IntStrataMean[k];
												delta_n = delta / (double) IntStrataCount[k];
												delta_n2 = delta_n * delta_n;

												term1 = delta * delta_n * (double) n1;
												IntStrataMean[k] += delta_n;
												IntStrataM4[k] += term1 * delta_n2 * ((double) IntStrataCount[k] * (double) IntStrataCount[k] - 3.0 * (double) IntStrataCount[k] + 3.0) + 6.0 * delta_n2 * IntStrataM2[k] - 4.0 * delta_n * IntStrataM3[k];
												IntStrataM3[k] += term1 * delta_n * ((double) IntStrataCount[k] - 2.0) - 3.0 * delta_n * IntStrataM2[k];
												IntStrataM2[k] += term1;

												// bins 0-8 have counts for returns 1-9
												// bin 9 has count of "other" returns
												if (StrataPointList[l].ReturnNumber < 10 && StrataPointList[l].ReturnNumber > 0) {
													IntStrataCountReturn[k][StrataPointList[l].ReturnNumber - 1] ++;
												}
												else {
													IntStrataCountReturn[k][9] ++;
												}

												// do min/max
												IntStrataMin[k] = std::min<double>(IntStrataMin[k], StrataPointList[l].Intensity);
												IntStrataMax[k] = std::max<double>(IntStrataMax[k], StrataPointList[l].Intensity);

												break;
											}
										}
									}

									// compute the final values
									for (k = 0; k < m_HeightStrataIntensityCount; k ++) {
										if (IntStrataCount[k]) {
											IntStrataVariance[k] = IntStrataM2[k] / (double) (IntStrataCount[k] - 1);

											// kurtosis and skewness use (n - 1) to match values computed for entire point cloud
//											IntStrataKurtosis[k] = ((double) IntStrataCount[k] * IntStrataM4[k]) / (IntStrataM2[k] * IntStrataM2[k]) - 3.0;
											IntStrataKurtosis[k] = (((double) IntStrataCount[k] - 1.0) * IntStrataM4[k]) / (IntStrataM2[k] * IntStrataM2[k]);
											IntStrataSkewness[k] = (sqrt((double) IntStrataCount[k] - 1.0) * IntStrataM3[k]) / sqrt(IntStrataM2[k] * IntStrataM2[k] * IntStrataM2[k]);
										}
									}

									// resort the list using intensity values within each height strata
									TempPointCount = 0;
									for (k = 0; k < m_HeightStrataIntensityCount; k ++) {
										if (IntStrataCount[k]) {
											qsort(&StrataPointList[TempPointCount], (size_t) IntStrataCount[k], sizeof(STRATAPOINT), compareSPint);
										}
										TempPointCount += IntStrataCount[k];
									}

									// compute median and mode
									TempPointCount = 0;
									for (k = 0; k < m_HeightStrataIntensityCount; k ++) {
										if (IntStrataCount[k]) {
											WholePart = (int) ((float) (IntStrataCount[k] - 1) * 0.5);
											Fraction = ((float) (IntStrataCount[k] - 1) * 0.5) - WholePart;
										
											WholePart = TempPointCount + WholePart;

											if (Fraction == 0.0) {
												IntStrataMedian[k] = StrataPointList[WholePart].Intensity;
											}
											else {
												IntStrataMedian[k] = StrataPointList[WholePart].Intensity + Fraction * (StrataPointList[WholePart + 1].Intensity - StrataPointList[WholePart].Intensity);
											}
										}
										else {
											IntStrataMedian[k] = -9999.0;
										}

										TempPointCount += IntStrataCount[k];
									}

									// figure out the mode using 64 bins for data
									// count values using bins
									TempPointCount = 0;
									for (k = 0; k < m_HeightStrataIntensityCount; k ++) {
										if (IntStrataCount[k]) {
											for (l = 0; l < NUMBEROFBINS; l ++)
												Bins[l] = 0;

											for (l = TempPointCount; l < TempPointCount + IntStrataCount[k]; l ++) {
												TheBin = (int) ((((double) StrataPointList[l].Intensity - IntStrataMin[k]) / (IntStrataMax[k] - IntStrataMin[k])) * (double) (NUMBEROFBINS - 1));
												Bins[TheBin] ++;
											}

											// find most frequent value
											MaxCount = -1;
											for (l = 0; l < NUMBEROFBINS; l ++) {
												if (Bins[l] > MaxCount) {
													MaxCount = Bins[l];
													TheBin = l;
												}
											}

											// compute mode by un-scaling the bin number
											IntStrataMode[k] = IntStrataMin[k] + ((double) TheBin / (double) (NUMBEROFBINS - 1)) * (IntStrataMax[k] - IntStrataMin[k]);
										}
										else {
											IntStrataMode[k] = -9999.0;
										}

										TempPointCount += IntStrataCount[k];
									}
								}

								if (m_DoHeightStrata || m_DoHeightStrataIntensity) {
									delete [] StrataPointList;
								}

								// report stuff to csv file
								if (ParseID) 
									fprintf(f, "%i,", ID);

								if (m_ProduceHighpointOutput) {
									fprintf(f, "\"%s\",%i,%lf,%lf,%lf\n", ce.m_FileName.c_str(), PointCount, HighX, HighY, HighElevation);
								}
								else if (m_ProduceYZLiOutput) {
									fprintf(f, "\"%s\",%i,%lf,%lf,%lf,%lf\n", ce.m_FileName.c_str(), PointCount, ElevMean, ElevStdDev, ElevP75, Cover);
								}
								else {
									// print base values
									if (!m_UseHeightMin && !m_UseHeightMax)
									fprintf(f, "\"%s\",%s,%i", ce.m_FileName.c_str(), TempfsForFileTitle.FileTitle().c_str(), PointCount);
									else
										fprintf(f, "\"%s\",%s,%i,%i", ce.m_FileName.c_str(), TempfsForFileTitle.FileTitle().c_str(), TotalPointCount, PointCount);

									if (m_CountReturns) {
										for (k = 0; k < 10; k ++)
											fprintf(f, ",%i", ReturnCounts[k]);
									}

									fprintf(f, ",%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", 
											ElevMin, ElevMax, ElevMean, ElevMode, ElevStdDev, ElevVariance, ElevStdDev / ElevMean, ElevIQDist, ElevSkewness, ElevKurtosis, ElevAAD, ElevMadMedian, ElevMadMode,
											ElevL1, ElevL2, ElevL3, ElevL4, ElevL2 / ElevL1, ElevL3 / ElevL2, ElevL4 / ElevL2,
											ElevP01, ElevP05, ElevP10, ElevP20, ElevP25, ElevP30, ElevP40, ElevP50, ElevP60, ElevP70, ElevP75, ElevP80, ElevP90, ElevP95, ElevP99, CanopyReliefRatio,
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

									if (m_DoKDEStats) {
										// KDE stuff
										fprintf(f, ",%i,%lf,%lf,%lf", KDE_ModeCount, KDE_MinMode, KDE_MaxMode, KDE_ModeRange);
									}

									if (m_DoHeightStrata) {
										for (k = 0; k < m_HeightStrataCount; k ++) {
											if (ElevStrataCount[k] > 2)
												fprintf(f, ",%i,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", ElevStrataCount[k], (double) ElevStrataCount[k] / (double) StrataPointCount, ElevStrataMin[k], ElevStrataMax[k], ElevStrataMean[k], ElevStrataMode[k], ElevStrataMedian[k], sqrt(ElevStrataVariance[k]), sqrt(ElevStrataVariance[k]) / ElevStrataMean[k], ElevStrataSkewness[k], ElevStrataKurtosis[k]);
											else if (ElevStrataCount[k])
												fprintf(f, ",%i,%lf,-9999.0,-9999.0,%lf,-9999.0,-9999.0,-9999.0,-9999.0,-9999.0,-9999.0", ElevStrataCount[k], (double) ElevStrataCount[k] / (double) StrataPointCount, ElevStrataMean[k]);
											else
												fprintf(f, ",%i,0.0,-9999.0,-9999.0,%lf,-9999.0,-9999.0,-9999.0,-9999.0,-9999.0,-9999.0", ElevStrataCount[k], ElevStrataMean[k]);
										}
									}

									if (m_DoHeightStrataIntensity) {
										for (k = 0; k < m_HeightStrataIntensityCount; k ++) {
											if (IntStrataCount[k] > 2)
												fprintf(f, ",%i,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", IntStrataCount[k], (double) IntStrataCount[k] / (double) StrataPointCount, IntStrataMin[k], IntStrataMax[k], IntStrataMean[k], IntStrataMode[k], IntStrataMedian[k], sqrt(IntStrataVariance[k]), sqrt(IntStrataVariance[k]) / IntStrataMean[k], IntStrataSkewness[k], IntStrataKurtosis[k]);
											else if (IntStrataCount[k])
												fprintf(f, ",%i,%lf,-9999.0,-9999.0,%lf,-9999.0,-9999.0,-9999.0,-9999.0,-9999.0,-9999.0", IntStrataCount[k], (double) IntStrataCount[k] / (double) StrataPointCount, IntStrataMean[k]);
											else
												fprintf(f, ",%i,0.0,-9999.0,-9999.0,%lf,-9999.0,-9999.0,-9999.0,-9999.0,-9999.0,-9999.0", IntStrataCount[k], IntStrataMean[k]);
										}
									}

									// print end of line
									fprintf(f, "\n");
								}

								// clean up
								delete [] ElevValueList;
								delete [] IntValueList;

								// status info
								csTemp.Format("   %s: %i points", ce.m_FileName.c_str(), PointCount);
								LTKCL_PrintStatus(csTemp);
							}
							else if (PointCount >= 1) {
								// only report min, max, mean
								
								// report stuff to csv file
								if (ParseID) 
									fprintf(f, "%i,", ID);

								if (m_ProduceHighpointOutput) {
									fprintf(f, "\"%s\",%i,%lf,%lf,%lf\n", ce.m_FileName.c_str(), PointCount, HighX, HighY, HighElevation);
								}
								else if (m_ProduceYZLiOutput) {
									fprintf(f, "\"%s\",%i,%lf,%lf,%lf,%lf\n", ce.m_FileName.c_str(), PointCount, ElevMean, ElevStdDev, ElevP75, Cover);
								}
								else {
									// print base values
									if (!m_UseHeightMin && !m_UseHeightMax)
									fprintf(f, "\"%s\",%s,%i", ce.m_FileName.c_str(), TempfsForFileTitle.FileTitle().c_str(), PointCount);
									else
										fprintf(f, "\"%s\",%s,%i,%i", ce.m_FileName.c_str(), TempfsForFileTitle.FileTitle().c_str(), TotalPointCount, PointCount);
	//								fprintf(f, "\"%s\",%i", ce.m_FileName, PointCount);

									if (m_CountReturns) {
										for (k = 0; k < 10; k ++)
											fprintf(f, ",%i", ReturnCounts[k]);
									}

									fprintf(f, ",%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", 
											ElevMin, ElevMax, ElevMean, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
											0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
											0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, CanopyReliefRatio,
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

									if (m_DoKDEStats) {
										// KDE stuff
										fprintf(f, ",0,0.0,0.0,0.0");
									}

									if (m_DoHeightStrata) {
										for (k = 0; k < m_HeightStrataCount; k ++) {
											fprintf(f, ",0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0");
										}
									}

									if (m_DoHeightStrataIntensity) {
										for (k = 0; k < m_HeightStrataIntensityCount; k ++) {
											fprintf(f, ",0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0");
										}
									}

									fprintf(f, "\n");
								}
							}
							else {
								// not enough points...print zeros to output file
								if (ParseID) 
									fprintf(f, "0,");

								if (m_ProduceHighpointOutput) {
									fprintf(f, "\"%s\",%i,0.0,0.0,0.0\n", ce.m_FileName.c_str(), PointCount);
								}
								else if (m_ProduceYZLiOutput) {
									fprintf(f, "\"%s\",%i,0.0,0.0,0.0,0.0\n", ce.m_FileName.c_str(), PointCount);
								}
								else {
									if (!m_UseHeightMin && !m_UseHeightMax)
									fprintf(f, "\"%s\",%s,%i", ce.m_FileName.c_str(), TempfsForFileTitle.FileTitle().c_str(), PointCount);
									else
										fprintf(f, "\"%s\",%s,%i,%i", ce.m_FileName.c_str(), TempfsForFileTitle.FileTitle().c_str(), TotalPointCount, PointCount);
	//								fprintf(f, "\"%s\",%i", ce.m_FileName, PointCount);

									if (m_CountReturns) {
										for (k = 0; k < 10; k ++)
											fprintf(f, ",%i", ReturnCounts[k]);
									}

									fprintf(f, ",0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0");

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

									if (m_DoKDEStats) {
										// KDE stuff
										fprintf(f, ",0,0.0,0.0,0.0");
									}

									if (m_DoHeightStrata) {
										for (k = 0; k < m_HeightStrataCount; k ++) {
											fprintf(f, ",0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0");
										}
									}

									if (m_DoHeightStrataIntensity) {
										for (k = 0; k < m_HeightStrataIntensityCount; k ++) {
											fprintf(f, ",0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0");
										}
									}

									fprintf(f, "\n");
								}

								// status info
								csTemp.Format("***ERROR: Metrics NOT computed for %s...not a valid data file", ce.m_FileName.c_str());
								LTKCL_PrintStatus(csTemp);
							}
						}
						else {
							// bad file...print zeros to output file
							if (ParseID) 
								fprintf(f, "0,");

							if (m_ProduceHighpointOutput) {
								fprintf(f, "\"%s\",%i,0.0,0.0,0.0\n", ce.m_FileName.c_str(), PointCount);
							}
							else if (m_ProduceYZLiOutput) {
								fprintf(f, "\"%s\",%i,0.0,0.0,0.0,0.0\n", ce.m_FileName.c_str(), PointCount);
							}
							else {
								if (!m_UseHeightMin && !m_UseHeightMax)
								fprintf(f, "\"%s\",%s,%i", ce.m_FileName.c_str(), TempfsForFileTitle.FileTitle().c_str(), PointCount);
								else
									fprintf(f, "\"%s\",%s,%i,%i", ce.m_FileName.c_str(), TempfsForFileTitle.FileTitle().c_str(), TotalPointCount, PointCount);
	//							fprintf(f, "\"%s\",%i", ce.m_FileName, PointCount);

								if (m_CountReturns) {
									for (k = 0; k < 10; k ++)
										fprintf(f, ",%i", ReturnCounts[k]);
								}

								//                                                1                                       2                                       3                                       4                                       5
								//            1   2   3   4   5   6   7   8   9   0   1   2   3   4   5   6   7   8   9   0   1   2   3   4   5   6   7   8   9   0   1   2   3   4   5   6   7   8   9   0   1   2   3   4   5   6   7   8   9   0   1   2
								fprintf(f, ",0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0");

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

								if (m_DoKDEStats) {
									// KDE stuff
									fprintf(f, ",0,0.0,0.0,0.0");
								}

								if (m_DoHeightStrata) {
									for (k = 0; k < m_HeightStrataCount; k ++) {
										fprintf(f, ",0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0");
									}
								}

								if (m_DoHeightStrataIntensity) {
									for (k = 0; k < m_HeightStrataIntensityCount; k ++) {
										fprintf(f, ",0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0");
									}
								}

								fprintf(f, "\n");
							}

							// status info
							csTemp.Format("***ERROR: Metrics NOT computed for %s...not a valid data file", ce.m_FileName.c_str());
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

#include "Command_line_core_functions.cpp"

#define STEPS	512
#define MIN_KDE_POINTS	10
#define MIN_KDE_RANGE	3.0

void GaussianKDE(float* PointData, int Pts, double BW, double SmoothWindow, int& ModeCount, double& MinMode, double& MaxMode)
{
	int i, j;
	double MinElev, MaxElev;
	double DataMinElev, DataMaxElev;
	int UseSmoothedCurve = TRUE;
	int ModeCnt;

	if (SmoothWindow == 0.0)
		UseSmoothedCurve = FALSE;

	// get the min/max elevation
	MinElev = DBL_MAX;
	MaxElev = DBL_MIN;
	for (i = 0; i < Pts; i ++) {
		MinElev = min(MinElev, (double) PointData[i]);
		MaxElev = max(MaxElev, (double) PointData[i]);
	}

	// save actual data range
	DataMinElev = MinElev;
	DataMaxElev = MaxElev;

	// check for meaningful data
	if (Pts < MIN_KDE_POINTS || (MaxElev - MinElev) < SmoothWindow || (MaxElev - MinElev) < MIN_KDE_RANGE) {
		ModeCount = 0;
		MinMode = 0.0;
		MaxMode = 0.0;

		return;
	}

	// adjust min/max
	MinElev -= BW * 3.0;
	MaxElev += BW * 3.0;

	double X[STEPS];		// elevation/height
	double Y[STEPS];		// density
	double SmoothX[STEPS];		// elevation/height
	double SmoothY[STEPS];		// density
	char sign[STEPS];	// + or -
	double Step = (MaxElev - MinElev) / (double) (STEPS - 1);
	double Constant1 = 1.0 / ((double) Pts * sqrt(2.0 * 3.141592653589793 * BW * BW));
	double Constant2 = 2.0 * BW * BW;
	double ExpTerm;
	double AveY;		// used for sliding window

	// compute probabilites
	double Ht = MinElev;
	for (i = 0; i < STEPS; i ++) {
		ExpTerm = 0.0;
		for (j = 0; j < Pts; j ++) {
			ExpTerm += exp(-1.0 * (Ht - (double) PointData[j]) * (Ht - (double) PointData[j]) / Constant2);
		}

		X[i] = Ht;
		Y[i] = Constant1 * ExpTerm;

		Ht += Step;

//		printf("%.4lf,%.4lf\n", X[i], Y[i]);
	}

	// do a sliding window average to smooth
	if (UseSmoothedCurve) {
		int EndHalfWindow;
		int CellCnt;
		int HalfWindow = (int) (SmoothWindow / Step);

		// force HalfWindow to be odd to make sure window is centered on the point being modified
		if (HalfWindow % 2 == 0)
			HalfWindow ++;

		EndHalfWindow = 0;
		for (i = 0; i < HalfWindow; i ++) {
			AveY = 0.0;
			CellCnt = 0;
			for (j = i - EndHalfWindow; j <= i + EndHalfWindow; j ++) {
	//		for (j = i - HalfWindow; j <= i + HalfWindow; j ++) {
				if (j >= 0) {
					AveY += Y[j];
					CellCnt ++;
				}
			}
			SmoothX[i] = X[i];
			SmoothY[i] = AveY / ((double) CellCnt);

			EndHalfWindow ++;
		}

		// this loop should handle only complete windows...window fits within the array bounds
		for (i = HalfWindow; i < STEPS - HalfWindow; i ++) {
			if (i == HalfWindow) {
				// first time...need to get all values
				AveY = 0.0;
				CellCnt = 0;
				for (j = i - HalfWindow; j <= i + HalfWindow; j ++) {
					if (j >= 0 && j < STEPS) {
						AveY += Y[j];
						CellCnt ++;
					}
				}
			}
			else {
				// after first time only need to change first and last values
				AveY -= Y[(i - HalfWindow) - 1];		// subtract 1 since i has been advanced by 1 since "group" was formed
				AveY += Y[(i + HalfWindow)];
			}
			SmoothX[i] = X[i];
			SmoothY[i] = AveY / ((double) CellCnt);
		}

		EndHalfWindow = 0;
		for (i = STEPS - 1; i >= max(HalfWindow, STEPS - HalfWindow); i --) {
	//	for (i = STEPS - HalfWindow; i < STEPS; i ++) {
			AveY = 0.0;
			CellCnt = 0;
			for (j = i - EndHalfWindow; j <= i + EndHalfWindow; j ++) {
	//		for (j = i - HalfWindow; j <= i + HalfWindow; j ++) {
				if (j >= 0 && j < STEPS) {
					AveY += Y[j];
					CellCnt ++;
				}
			}
			SmoothX[i] = X[i];
			SmoothY[i] = AveY / ((double) CellCnt);

			EndHalfWindow ++;
		}
	}

	// figure out the signs
	int FirstMin = FALSE;
	sign[0] = -1;
	sign[STEPS - 1] = 1;
	if (!UseSmoothedCurve) {
		for (i = 0; i < STEPS; i ++) {
			// make sure we don't do a mode outside the actual data range
			if (X[i] >= DataMinElev && X[i] <= DataMaxElev) {
				if (!FirstMin) {
					sign[i] = -1;
					FirstMin = TRUE;
				}
				else if (Y[i] > Y[i - 1])
					sign[i] = 1;
				else if (Y[i] < Y[i - 1])
					sign[i] = -1;
				else
					sign[i] = 0;
			}
			else
				sign[i] = 0;
		}
	}
	else {
		// do signs using smoothed values
		for (i = 0; i < STEPS; i ++) {
			// make sure we don't do a mode outside the actual data range
			if (SmoothX[i] >= DataMinElev && SmoothX[i] <= DataMaxElev) {
				if (!FirstMin) {
					sign[i] = -1;
					FirstMin = TRUE;
				}
				else if (SmoothY[i] > SmoothY[i - 1])
					sign[i] = 1;
				else if (SmoothY[i] < SmoothY[i - 1])
					sign[i] = -1;
				else
					sign[i] = 0;
			}
			else
				sign[i] = 0;
		}
	}

	// get mode values
	double Modes[64];
	double ModeProb[64];
	int ModeType[64];
	ModeCnt = 0;
	for (i = 1; i < STEPS; i ++) {
		// detect transitions from positive slope to flat or negative slope...peaks
		if (sign[i] != sign[i - 1] && sign[i - 1] == 1) {
			Modes[ModeCnt] = X[i - 1];
			ModeProb[ModeCnt] = Y[i - 1];
			ModeType[ModeCnt] = 1;

//			printf("%8lf %.8lf %.8lf 255 0 0\n", 0.0, Y[i] * 100.0, X[i]);
			ModeCnt ++;
		}

		// detect transitions from negative slope to flat or positive slope...valleys
		if (sign[i] != sign[i - 1] && sign[i - 1] == -1) {
			Modes[ModeCnt] = X[i - 1];
			ModeProb[ModeCnt] = Y[i - 1];
			ModeType[ModeCnt] = -1;

			ModeCnt ++;
		}

		// check for too many modes
		if (ModeCnt >= 63)
			break;
	}

	if (ModeCnt >= 63) {
		ModeCount = 0;
		MinMode = 0.0;
		MaxMode = 0.0;

		return;
	}

	// get the min/max mode heights for peaks
	ModeCount = 0;
	MinMode = DBL_MAX;
	MaxMode = DBL_MIN;
	for (i = 0; i < ModeCnt; i ++) {
		if (ModeType[i] == 1 && (Modes[i] >= DataMinElev && Modes[i] <= DataMaxElev)) {
			MinMode = min(MinMode, Modes[i]);
			MaxMode = max(MaxMode, Modes[i]);

			ModeCount ++;
		}
	}
/*
	// output summary
	FILE* f = fopen("summary.csv", "wt");
	if (f) {
		fprintf(f, "Data minimum: %.4f\n", DataMinElev);
		fprintf(f, "Data maximum: %.4f\n", DataMaxElev);
		fprintf(f, "%i modes\n", ModeCnt);
		fprintf(f, "Minimum mode: %.4lf\n", MinMode);
		fprintf(f, "Maximum mode: %.4lf\n", MaxMode);
		fprintf(f, "Modes:\n");
		for (i = 0; i < ModeCnt; i ++) {
			fprintf(f, "   %2i   %3.4lf   %3.4lf   %2i\n", i + 1, Modes[i], ModeProb[i], ModeType[i]);
		}
		if (UseSmoothedCurve) {
			for (i = 0; i < STEPS; i ++) {
				fprintf(f, "%.8lf %.8lf %i\n", SmoothY[i] * 100, SmoothX[i], sign[i]);
			}
		}
		else {
			for (i = 0; i < STEPS; i ++) {
				fprintf(f, "%.8lf %.8lf %i\n", Y[i] * 100, X[i], sign[i]);
			}
		}
		fclose(f);
	}

	// "raw" KDE values
	for (i = 0; i < STEPS; i ++) {
		printf("%8lf %.8lf %.8lf 255 255 0\n", 0.0, Y[i] * 100, X[i]);
	}

	// smoothed values
	if (UseSmoothedCurve) {
		for (i = 0; i < STEPS; i ++) {
			printf("%8lf %.8lf %.8lf 255 0 0\n", 0.0, SmoothY[i] * 1000, SmoothX[i]);
		}
	}

	// mode lines
	double Yval;
	double YStep = 0.001;
//	double YStep = (MaxModeProb - MinModeProb) / 11.0;
	if (YStep == 0.0)
		YStep = 0.001;
	for (i = 0; i < ModeCnt; i ++) {
		if (ModeType[i] == 1) {
			Yval = ModeProb[i] - (YStep * 5.5);
			for (j = -5; j <= 5; j ++) {
				printf("%8lf %.8lf %.8lf 255 255 0\n", 0.0, Yval * 1000.0, Modes[i]);
				Yval += YStep;
			}
		}
		if (ModeType[i] == -1) {
			Yval = ModeProb[i] - (YStep * 5.5);
			for (j = -5; j <= 5; j ++) {
				printf("%8lf %.8lf %.8lf 0 255 0\n", 0.0, Yval * 1000.0, Modes[i]);
				Yval += YStep;
			}
		}
	}
*/
}

