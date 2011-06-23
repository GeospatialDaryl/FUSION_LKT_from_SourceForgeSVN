//***********************************************************************
// (c) Copyright 1999-2003 Santronics Software, Inc. All Rights Reserved.
//***********************************************************************
// File Name : argslib.cpp
// Subsystem : Command Line Parameters Wrapper
// Date      : 03/03/2003
// Author    : Hector Santos, Santronics Software, Inc.
// VERSION   : 1.00P
//
// Revision History:
// Version  Date      Author  Comments
// -------  --------  ------  -------------------------------------------
// v1.00P   03/03/03  HLS     Public Release version (non-SSL version)
//
// 8/25/2009
// RJM:  added logic to remove quotation marks from switch parameters and 
// command line arguments
//
//***********************************************************************

#include "stdafx.h"
#include "argslib.h"


///////////////////////////////////////////////////////////////
// cut/paste example usage:
/*
    #include "argslib.h"


    CCommandLineParameters clp;
    if (clp.CheckHelp(FALSE) {  // set TRUE to display help if no switches
        ... display help ...
        return;
    }
    CString sServer = clp.GetSwitchStr("server");
    if (clp.Switch("debug")) DebugLogging     = TRUE;
    if (clp.Switch("dlog+")) DailyLogsEnabled = TRUE;
    if (clp.Switch("dlog-")) DailyLogsEnabled = FALSE;
    if (clp.Switch("trace")) EnableTraceLog   = TRUE;

    if (sServer.IsEmpty()) {
        if (!WildcatServerConnect(NULL)) {
            return FALSE;
        }
    } else {
        if (!WildcatServerConnectSpecific(NULL,sServer)) {
            return FALSE;
        }
    }
*/
///////////////////////////////////////////////////////////////

//-------------------------------------------------------------
// CreateParameterFromString()
//
// Given command line string, generate array of strings

int CreateParameterFromString(char *pszParams, char *argv[], int max)
{
    int argc = 0;
    if (pszParams) {
        char *p = pszParams;
        while (*p && (argc < max)) {
            while (*p == ' ') {
                p++;
            }
            if (!*p) {
                break;
            }
            if (*p == '"') {
                p++;
                argv[argc++] = p;
                while (*p && *p != '"') {
                    p++;
                }
            } else {
                argv[argc++] = p;
                while (*p && *p != ' ') {
                    p++;
                }
            }
            if (*p) {
                *p++ = 0;
            }
        }
    }
    return argc;
}

CCommandLineParameters::CCommandLineParameters(const char *switchchars /* = "-/" */)
    : szSwitchChars(switchchars)
{
    int maxparms = 100;
    pszCmdLineDup = _strdup(GetCommandLine());
    paramcount = CreateParameterFromString(pszCmdLineDup,parms,maxparms);
}

CCommandLineParameters::~CCommandLineParameters()
{
    if (pszCmdLineDup) {
      free(pszCmdLineDup);
      pszCmdLineDup = NULL;
    }
}

BOOL CCommandLineParameters::CheckHelp(const BOOL bNoSwitches /*= FALSE */)
{
     if (bNoSwitches && (paramcount < 2)) return TRUE;
     if (paramcount < 2) return FALSE;
     if (strcmp(ParamStr(1),"-?") == 0) return TRUE;
     if (strcmp(ParamStr(1),"/?") == 0) return TRUE;
     if (strcmp(ParamStr(1),"?") == 0) return TRUE;
     return FALSE;
}

CString CCommandLineParameters::ParamStr(const int index, const BOOL bGetAll /* = FALSE */)
{
    if ((index < 0) || (index >= paramcount)) {
        return "";
    }
    CString s = parms[index];
    if (bGetAll) {
        for (int i = index+1;i < paramcount; i++) {
              s += " ";
              s += parms[i];
        }
    }
    return s;
}

int CCommandLineParameters::ParamInt(const int index)
{
    return atoi(ParamStr(index));
}

CString CCommandLineParameters::ParamLine()
{
    CString s;
    char *p = strchr(GetCommandLine(),' ');
    if (p) {
        s.Format("%s",p+1);
    }
    return s;
}

CString CCommandLineParameters::CommandLine()
{
    CString s;
    s.Format("%s",GetCommandLine());
    return s;
}

BOOL CCommandLineParameters::IsSwitch(const char *sz)
{
    return (strchr(szSwitchChars,sz[0]) != NULL);
}


int CCommandLineParameters::SwitchCount()
{
    int count = 0;
    for (int i = 1;i < paramcount; i++) {
        if (IsSwitch(parms[i])) count++;
    }
    return count;
}

int CCommandLineParameters::FirstNonSwitchIndex()
{
    for (int i = 1;i < paramcount; i++) {
        if (!IsSwitch(parms[i])) {
            return i;
        }
    }
    return 0;
}

CString CCommandLineParameters::FirstNonSwitchStr()     // 499.5 04/16/01 12:17 am
{
    // returns the first non-switch, handles lines such as:
    // [options] file [specs]
    return GetNonSwitchStr(FALSE,TRUE);
}

//////////////////////////////////////////////////////////////////////////
// Switch() will return the parameter index if the switch exist.
// Return 0 if not found.  The logic will allow for two types of
// switches:
//
//          /switch value
//          /switch:value
//
// DO NOT PASS THE COLON. IT IS AUTOMATICALLY CHECKED.  In other
// words, the following statements are the same:
//
//         Switch("server");
//         Switch("-server");
//         Switch("/server");
//
// to handle the possible arguments:
//
//         /server:value
//         /server value
//         -server:value
//         -server value
//

int CCommandLineParameters::Switch(const char *sz,
                                   const BOOL bCase /* = FALSE */
                                   )
{
    if (!sz || !sz[0]) {
        return 0;
    }

    char sz2[255];
    strncpy(sz2,sz,sizeof(sz2)-1);
    sz2[sizeof(sz2)-1] = 0;

    char *p = sz2;
    if (strchr(szSwitchChars,*p) != NULL) p++;

    // check for abbrevation
    int ful_amt = 0;
    int abr_amt = 0;
    char *abbr = strchr(p,'*');
    if (abbr) {
      *abbr = 0;
      abr_amt = strlen(p);
      strcpy(abbr,abbr+1);
    }
    ful_amt = strlen(p);
 
    for (int i = 1;i < paramcount; i++) {
      if (!IsSwitch(parms[i])) continue;
      char *pColon = strchr(&parms[i][1],':');
      int arg_amt = 0;
      if (pColon) {
        arg_amt = pColon - &parms[i][1]; 
      } else {
        arg_amt = strlen(&parms[i][1]);
      }
      if (bCase) {
        if (arg_amt == ful_amt && strncmp(p,&parms[i][1],ful_amt) == 0) return i;
        if (arg_amt == abr_amt && strncmp(p,&parms[i][1],abr_amt) == 0) return i;
      } else {
        if (arg_amt == ful_amt && _strnicmp(p,&parms[i][1],ful_amt) == 0) return i;
        if (arg_amt == abr_amt && _strnicmp(p,&parms[i][1],abr_amt) == 0) return i;
      }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// GetSwitchStr() will return the string for the given switch. The logic
// will allow for two types of switches:
//
//   /switch value
//   /switch:value
//
// RJM:  changed logic to only allow /switch:value type
// RJM:  added Replace() to remove quotation marks from switch parameters

CString CCommandLineParameters::GetSwitchStr(const char *sz,
                                             const char *szDefault, /* = "" */
                                             const BOOL bCase /* = FALSE */
                                            )
{
    int idx = Switch(sz,bCase);
    if (idx > 0) {
        CString s = ParamStr(idx);
        int n = s.Find(':');
        if (n > -1) {
			CString ts = s.Mid(n + 1);
			ts.Replace("\"", "");
            return ts;
//            return s.Mid(n+1);
        } 
//		else {
//          if ((idx+1) < paramcount) {
//              if (!IsSwitch(parms[idx+1])) {
//                  return parms[idx+1];
//              }
//          }
//        }
        //return szDefault;
    }
    return szDefault;
}

int CCommandLineParameters::GetSwitchInt(const char *sz,
                                          const int iDefault, /* = 0 */
                                          const BOOL bCase /* = FALSE */
                                         )
{
    char szDefault[25];
    sprintf(szDefault,"%d",iDefault);
    return atoi(GetSwitchStr(sz,szDefault,bCase));
}

// RJM:  added Replace() to remove quotation marks from command line arguments...this should be handled by DOS
//		 but it won't hurt to make sure
CString CCommandLineParameters::GetNonSwitchStr(
                                const BOOL bBreakAtSwitch, /* = TRUE */
                                const BOOL bFirstOnly /* = FALSE */)
{
    CString sLine = "";
    int i = 1;
    while (i < paramcount) {
        if (IsSwitch(parms[i])) {
            if (bBreakAtSwitch) break;
        } else {
            sLine += parms[i];
            if (bFirstOnly) break;
            sLine += " ";
        }
        i++;
    }
    sLine.TrimRight();
	sLine.Replace("\"", "");
    return sLine;
}


