// Copyright 2011 Green Code LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef VC6_TO_STD_H
#define VC6_TO_STD_H

// Redefine types used by Visual C++ 6 (VC6) into standard C++ counterparts
// so legacy code can be compiled by newer standard-compliant compilers.

typedef bool BOOL;
#define TRUE true
#define FALSE false


typedef       char * LPSTR;
typedef const char * LPCSTR;
typedef const char * LPCTSTR;

#include "CString.h"

#define _T(x) (x)


#define TRY try
#define CATCH_ALL(pEx) catch(...)
#define END_CATCH_ALL

#endif
