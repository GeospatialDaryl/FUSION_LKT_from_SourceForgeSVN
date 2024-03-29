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

#include <boost/algorithm/string.hpp>

#include "CString.h"

//-----------------------------------------------------------------------------

char CString::buffer[1000];  // for Format methods

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

CString::CString(const std::string & str)
  : str_(str)
{
}

//-----------------------------------------------------------------------------

CString::operator const char *() const
{
  return str_.c_str();
}

//-----------------------------------------------------------------------------

const char * CString::c_str() const
{
  return str_.c_str();
}

//-----------------------------------------------------------------------------

int CString::Find(const char * str) const
{
  std::string::size_type findPos = str_.find(str);
  if (findPos == std::string::npos)
    return -1;
  else
    return int(findPos);
}

//-----------------------------------------------------------------------------

int CString::Find(char ch) const
{
  std::string::size_type findPos = str_.find(ch);
  if (findPos == std::string::npos)
    return -1;
  else
    return int(findPos);
}

//-----------------------------------------------------------------------------

CString CString::Left(int nChars) const
{
  std::string substr = str_.substr(0, std::string::size_type(nChars));
  return CString(substr.c_str());
}

//-----------------------------------------------------------------------------

CString CString::Mid(int startPos) const
{
  std::string substr = str_.substr(std::string::size_type(startPos));
  return CString(substr.c_str());
}

//-----------------------------------------------------------------------------

bool CString::IsEmpty() const
{
  return str_.empty();
}

//-----------------------------------------------------------------------------

int CString::GetLength() const
{
  return int(str_.length());
}

//-----------------------------------------------------------------------------

bool CString::operator==(const char * str) const
{
  return str_ == str;
}

//-----------------------------------------------------------------------------

int CString::CompareNoCase(const char * str) const
{
  if (boost::algorithm::iequals(str_, str))
    return 0;
  else if (boost::algorithm::ilexicographical_compare(str_, str))
    return -1;
  else
    return 1;
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
  str_ += str;
  return *this;
}

//-----------------------------------------------------------------------------

void CString::TrimRight()
{
  boost::trim_right(str_);
}

//-----------------------------------------------------------------------------

int CString::Replace(const char * oldStr,
                     const char * newStr)
{
  std::string::size_type oldStrLen = strlen(oldStr);
  std::string::size_type newStrLen = strlen(newStr);
  int count = 0;
  for (std::string::size_type findPos = str_.find(oldStr);
       findPos != std::string::npos;
       findPos = str_.find(oldStr, findPos + newStrLen)) {
    str_.replace(findPos, oldStrLen, newStr);
    count++;
  }
  return count;
}

//-----------------------------------------------------------------------------

void CString::MakeLower()
{
  boost::to_lower(str_);
}

//-----------------------------------------------------------------------------

// auxiliary operators

CString operator+(const CString & str1, const char * str2)
{
  CString result(str1);
  result += str2;
  return result;
}
