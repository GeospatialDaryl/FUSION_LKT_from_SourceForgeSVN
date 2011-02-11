// LIDAR Toolkit
// Bob McGaughey, USDA Forest Service
//
// header file should be added in the normal location in the module containing _tmain()
// #include "command_line_core_functions.h"
//
// code should be included at end of module containing _tmain() using the following:
// #include "command_line_core_functions.cpp"
//
// global functions that are NOT TYPICALLY modified to create a new program
//
// Jan 2007  added use of environment variable LTKLOG to provide option to use a specific log file
//           This should be easier to use in batch programs that do lots of processing as you can
//           simply set the variable at the beginning of the batch file and clear it at the end.
//
// 11/10/2010 toolkit
//	Added ability to check for bad command line switches using the LTKCL_VerifyCommandLine() function. Each program defines
//	a string containing all of the valid switch names and then calls LTKCL_VerifyCommandLine(). If the command line contains
//	a switch that is not in the list an error is reported.
//
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

BOOL LTKCL_VerifyCommandLine(const char* ValidSwitches)
{
	BOOL retcode = FALSE;
	CString csTemp;
	CString tsSwitch;
	CString tsValidSwitches(ValidSwitches);
	CString BaseSwitches(" quiet interactive newlog log version verbose ");

	// add spaces to the beginning and end of the valid switch string
	tsValidSwitches.Insert(0, " ");
	tsValidSwitches.Insert(tsValidSwitches.GetLength() + 1, " ");

	// make lower case
	tsValidSwitches.MakeLower();

	// step through command line switches and make sure they are recognized
	for (int i = 1; i <= m_clp.SwitchCount(); i ++) {
		// get next switch
		tsSwitch = m_clp.ParamStr(i);

		// make lower case
		tsSwitch.MakeLower();

		// get rid of leading "/"
		tsSwitch.Replace("/", "");

		// trim before ":"
		if (tsSwitch.Find(":") >= 0)
			tsSwitch = tsSwitch.Left(tsSwitch.Find(":"));

		// add a space to the beginning and end of the switch string...helps when a keyword is also embedded in another keyword
		// e.g. lasclass and class
		tsSwitch = _T(" ") + tsSwitch + _T(" ");

		// make sure it is valid for this program
		if (tsValidSwitches.Find(tsSwitch) < 0) {
			if (BaseSwitches.Find(tsSwitch) < 0) {
				csTemp.Format("   Invalid command line switch:%s", tsSwitch.c_str());
				LTKCL_PrintStatus(csTemp);

				m_nRetCode = 999;
				retcode = TRUE;
			}
		}
	}
	return(retcode);
}

void LTKCL_ParseStandardSwitches()
{
	m_RunQuiet = m_clp.Switch("quiet");
	m_RunInteractive = m_clp.Switch("interactive");
	m_ClearMasterLogFile = m_clp.Switch("newlog");
	m_ReportVersionInfo = m_clp.Switch("version");
	m_ShowVerboseStatus = m_clp.Switch("verbose");
		
	// look for /log:filename switch to use a specific log file...not cleared by default...must use /newlog to clear
	if (m_clp.Switch("log")) {
		CString LogFileName = m_clp.GetSwitchStr("log", "");
		if (!LogFileName.IsEmpty()) {
			m_MasterLogFileName = LogFileName;
			CFileSpec fs(LogFileName);
			if (fs.Extension().IsEmpty())
				fs.SetExt(".log");
			m_MasterLogFileName = fs.GetFullSpec();

			// do csv log file
			fs.SetExt(".csv");
			m_MasterCSVLogFileName = fs.GetFullSpec();
		}
	}
}

void LTKCL_PrintStandardSwitchInfo()
{
//			         1         2         3         4         5         6         7         8         9         10
//			1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
	printf("  Switches are preceeded by a \"/\". If a switch has multiple parameters after\n");
	printf("  the \":\", they should be separated by a single comma with no spaces before\n");
	printf("  or after the comma.\n\n");
	printf("  interactive Present a dialog-based interface\n");
	printf("  quiet       Suppress all output during the run\n");
	printf("  verbose     Display all status information during the run\n");
	printf("  version     Report version information and exit with no processing\n");
	printf("  newlog      Erase the existing log file and start a new log\n");
	printf("  log:name    Use the name specified for the log file\n");
}

BOOL LTKCL_Initialize()
{
	// initialize global variables
	m_MasterLogFileName.Empty();
	m_MasterCSVLogFileName.Empty();
	m_nRetCode = 0;
	m_RunQuiet = FALSE;
	m_ShowVerboseStatus = FALSE;
	m_RunInteractive = FALSE;
	m_ReportVersionInfo = FALSE;

	// get starting time
	time(&m_StartTime);                 // Get time in seconds

	// set up name for master log file...get application path and add "LTKCL_master.log"
	CFileSpec fs(CFileSpec::FS_APPDIR);
	fs.SetFileNameEx("LTKCL_master.log");
	m_MasterLogFileName = fs.GetFullSpec();
	fs.SetFileNameEx("LTKCL_master.csv");
	m_MasterCSVLogFileName = fs.GetFullSpec();
	
	// look for environment variable LTKLOG to set the log file...assume it is the full path for the log file
	char* logenv = getenv("LTKLOG");
	if (logenv) {
		fs.SetFullSpec(logenv);

		// look for extension
		if (fs.Extension().IsEmpty())
			fs.SetExt(".log");

		m_MasterLogFileName = fs.GetFullSpec();

		// change to csv log
		fs.SetExt(".csv");
		m_MasterCSVLogFileName = fs.GetFullSpec();
	}

	LTKCL_ParseStandardSwitches();
	
	LTKCL_PrintToLog("**********");

	// see if /version was used and report version info
	if (m_ReportVersionInfo) {
		CString csTemp;
		csTemp.Format("%s v%.2f (FUSION v%.2lf) (Built on %s %s)", PROGRAM_NAME, PROGRAM_VERSION, FUSION_VERSION, __DATE__, __TIME__);
#ifdef _DEBUG
		csTemp += _T(" DEBUG");
#endif
		// prints the program header with the program name, version, build date
		LTKCL_PrintStatus(csTemp);

		return(FALSE);
	}

	return(TRUE);
}

void LTKCL_PrintToCSVLog(LPCTSTR lpszStatus)
{
	if (!m_MasterCSVLogFileName.IsEmpty()) {
		// see if log file exists...if not we need to add header
		int AddHeader = _access(m_MasterCSVLogFileName, 0);

		FILE* f = fopen(m_MasterCSVLogFileName, "at+");
		
		if (AddHeader)
			fprintf(f, "Program,Version,Program build date,Command line,Start time,Stop time,Elapsed time (seconds),Status\n");

		fprintf(f, "%s\n", lpszStatus);
		fclose(f);
	}
}

void LTKCL_PrintToLog(LPCTSTR lpszStatus, BOOL AddNewLine)
{
	// if log file output is disabled, don't do anything
	if (m_NoLogOutput)
		return;

	if (m_ClearMasterLogFile) {
		DeleteFile(m_MasterLogFileName);
		m_ClearMasterLogFile = FALSE;

		// delete csv log file and write new header line
		DeleteFile(m_MasterCSVLogFileName);
//		LTKCL_PrintToCSVLog("Program,Version,Program build date,Command line,Start time,Stop time,Elapsed time (seconds),Status");
	}

	if (!m_MasterLogFileName.IsEmpty()) {
		FILE* f = fopen(m_MasterLogFileName, "at+");
		if (AddNewLine)
			fprintf(f, "%s\n", lpszStatus);
		else
			fprintf(f, "%s", lpszStatus);
		fclose(f);
	}
}

void LTKCL_PrintStatus(LPCTSTR lpszStatus, BOOL AddNewLine)
{
	// prints status info to stdout
	if (!m_RunQuiet) {
		if (AddNewLine)
			printf("%s\n", lpszStatus);
		else
			printf("%s", lpszStatus);
	}
	LTKCL_PrintToLog(lpszStatus, AddNewLine);
}

void LTKCL_PrintVerboseStatus(LPCTSTR lpszStatus, BOOL AddNewLine)
{
	// prints status info to stdout
	if (!m_RunQuiet && m_ShowVerboseStatus) {
		CString csTemp;

		time_t aclock;
		time(&aclock);                 // Get time in seconds
		csTemp.Format("(elapsed time since start: %i seconds)", aclock - m_StartTime);

		if (AddNewLine)
			printf("%s %s\n", lpszStatus, csTemp.c_str());
		else
			printf("%s %s", lpszStatus, csTemp.c_str());
	}
}

void LTKCL_ReportProductFile(LPCTSTR lpszFileName, LPCTSTR lpszProductType)
{
	// reports a product from processing...includes date and time for the file
	// file should be closed and should not be written to after the call to report the product
	// to maintain the time stamp
	char* months[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	char* ampm[] = {"AM", "PM"};

	if (!m_RunQuiet) {
		// get the file info
		WIN32_FILE_ATTRIBUTE_DATA attribdata;
		FILETIME filetime;
		SYSTEMTIME  systime;
		if (GetFileAttributesEx(lpszFileName, GetFileExInfoStandard, &attribdata)) {
			FileTimeToLocalFileTime(&attribdata.ftLastWriteTime, &filetime);
			FileTimeToSystemTime(&filetime, &systime);

			// check for am or pm and adjust the hours accordingly
			int pmflag = 0;
			if (systime.wHour > 12) {
				pmflag = 1;
				systime.wHour -= 12;
			}

			CString csTemp;
			csTemp.Format("%s file produced:\n   %s    %s %i, %i @ %i:%02i %s\n", lpszProductType, lpszFileName, months[systime.wMonth], systime.wDay, systime.wYear, systime.wHour, systime.wMinute, ampm[pmflag]);

			LTKCL_PrintStatus(csTemp, FALSE);
		}
	}
}

void LTKCL_PrintProgramHeader()
{
	CString csTemp;
	csTemp.Format("%s v%.2f (FUSION v%.2lf) (Built on %s %s)", PROGRAM_NAME, PROGRAM_VERSION, FUSION_VERSION, __DATE__, __TIME__);
#ifdef _DEBUG
		csTemp += _T(" DEBUG");
#endif

	// prints the program header with the program name, version, build date
	LTKCL_PrintStatus(csTemp);
}

void LTKCL_PrintProgramDescription()
{
	CString csTemp;
	csTemp.Format("%s", PROGRAM_DESCRIPTION);

	// prints the program header with the program name, version, build date
	LTKCL_PrintStatus(csTemp);
}

void LTKCL_PrintRunTime()
{
	CString csTemp;

	// prints the run header with the run date and time
	struct tm *newtime;
	newtime = localtime(&m_StartTime);  // Convert time to struct tm form 
	csTemp.Format("Run started: %s", asctime(newtime));
	csTemp.Replace("\n", "");
	LTKCL_PrintStatus(csTemp);
}

void LTKCL_PrintCommandLine()
{
	CString csTemp;
	csTemp.Format("Command line: %s", m_clp.CommandLine().c_str());

	// prints the run header with the run date and time
	LTKCL_PrintStatus(csTemp);
}

void LTKCL_PrintElapsedTime(LPCTSTR Message)
{
	CString csTemp;

	time_t aclock;
	time(&aclock);                 // Get time in seconds
	csTemp.Format("%s elapsed time since start: %i seconds", Message, aclock - m_StartTime);

	LTKCL_PrintStatus(csTemp);
}

void LTKCL_PrintEndReport(int ErrCode)
{
	CString csTemp;

	struct tm *newtime;
	time_t aclock;
	time(&aclock);                 // Get time in seconds
	newtime = localtime(&aclock);  // Convert time to struct tm form
	char timestring[26];
	strcpy(timestring, asctime(newtime));
	timestring[24] = '\0';
	csTemp.Format("Run finished: %s (elapsed time: %i seconds)", timestring, aclock - m_StartTime);

	if (ErrCode)
		csTemp += _T("\n***There were errors during the run");

	csTemp += _T("\nDone");

	LTKCL_PrintStatus(csTemp);

	// print the csv log file entry
	struct tm *temptime;
	temptime = localtime(&m_StartTime);  // Convert time to struct tm form 
	char temptimestring[26];
	strcpy(temptimestring, asctime(temptime));
	temptimestring[24] = '\0';

	const char * Status;
	if (ErrCode)
		Status = _T("Errors");
	else
		Status = _T("Success");

	csTemp.Format("\"%s\",%.2f,\"%s %s\",\"%s\",\"%s\",\"%s\",%i,%s", PROGRAM_NAME, PROGRAM_VERSION, __DATE__, __TIME__, m_clp.ParamLine().c_str(), temptimestring, timestring, aclock - m_StartTime, Status);
	LTKCL_PrintToCSVLog(csTemp);
}

//-----------------------------------------------------------------------------
// Since the call to this function is commented out in both metrics programs,
// the function is disabled (besides it's Windows specific).
/*
void LTKCL_LaunchHelpPage()
{
	// launch web page for help info...will silently fail if file doesn't exist
	CString HTMLFile;
	HTMLFile.Format("doc\\%s.htm", PROGRAM_NAME);
	::ShellExecute(NULL, "open", HTMLFile, NULL, NULL, SW_SHOWNORMAL);
}
*/
