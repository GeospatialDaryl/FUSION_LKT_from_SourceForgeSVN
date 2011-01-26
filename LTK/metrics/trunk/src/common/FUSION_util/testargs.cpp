// File: D:\local\wc5\common\public\argslib\testargs.cpp

#include <stdio.h>
#include <afx.h>

#include "argslib.cpp"

void usage()
{
  printf("usage: <p1> [-p2 <v2>] [[/p3 [v3]] [/p4]]\n");
}

void main()
{

  CCommandLineParameters clp;

  if (clp.CheckHelp(TRUE)) {
      usage();
      return;
  }

  //---------------------------------------------------------
  // Syntax checking for p1.
  // First parameter must be a non-switch
  //---------------------------------------------------------

  if (clp.FirstNonSwitchIndex() != 1) {
      printf("! syntax error: p1 required\n");
      usage();
      return;
  }

  CString p1 = clp.FirstNonSwitchStr();

  //---------------------------------------------------------
  // Syntax checking for p2. If p2 is used, then
  // the next argument must be a non-switch.
  //---------------------------------------------------------

  BOOL p2    = clp.Switch("p2")>0;
  CString v2 = clp.GetSwitchStr("p2");

  if (p2 && v2.IsEmpty()) {
      printf("! syntax error: v2 required when /p2 is used\n");
      usage();
      return;
  }

  //---------------------------------------------------------
  // Syntax checking for p2.
  // For p3, since v3 is optional, we use a default "" value.
  // p4 is only valid if p3 is enabled
  //---------------------------------------------------------

  BOOL p3    = clp.Switch("p3")>0;
  CString v3 = clp.GetSwitchStr("p3","");
  BOOL p4    = p3 && (clp.Switch("p4")>0);

  printf("p1 : %s\n",p1);
  printf("p2 : %d  v2: %s\n",p2,v2);
  printf("p3 : %d  v3: %s\n",p3,v3);
  printf("p4 : %d\n",p4);

}
