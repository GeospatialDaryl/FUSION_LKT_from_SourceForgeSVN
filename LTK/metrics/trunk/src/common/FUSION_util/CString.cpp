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

#include "CString.h"

//-----------------------------------------------------------------------------

CString::CString()
  : str_()
{
}

//-----------------------------------------------------------------------------

CString::CString(const char * str)
  : str_(str)
{
}

//-----------------------------------------------------------------------------

CString::operator const char *() const
{
  return str_.c_str();
}

//-----------------------------------------------------------------------------

void CString::Empty()
{
  str_.clear();
}

//-----------------------------------------------------------------------------

CString & CString::operator=(const char * str)
{
  str_ = str;
  return *this;
}

//-----------------------------------------------------------------------------

CString & CString::operator+=(const char * str)
{
  str_ = str;
  return *this;
}

//-----------------------------------------------------------------------------

void CString::TrimRight()
{
  // Search backward from end of string for last non-whitespace character.
  std::string::reverse_iterator itor;
  for (itor = str_.rbegin(); itor < str_.rend(); ++itor) {
    if (! isspace(*itor))
      break;
  }
  // # of characters to keep (not erase)
  std::string::size_type nKeep = str_.rend() - itor;
  str_.erase(nKeep);
}
