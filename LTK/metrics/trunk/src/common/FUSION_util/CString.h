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

#ifndef CSTRING_H
#define CSTRING_H

// A partial implementation of the MFC CString.  Sufficient enough for FUSION
// legacy code to compile with minimal changes.  Also the implementation must
// must be cross-platform (standard C++).

#include <string>

class CString
{
  public:
    // constructors
    CString();
    CString(const char * str);

    // accessors
    operator const char *() const;
    int Find(const char * str) const;
      // Find a string within the current value.  Return its position if found;
      // -1 if not found.
    int Find(char ch) const;
      // Find a character within the current value.  Return its position if
      // found; -1 if not found.
    CString Mid(int startPos) const;
      // Extract a substring starting at the given position.  The substring
      // contains all of the characters from the start position to the end of
      // the string.

    // comparators
    bool operator==(const char * str) const;

    // modifiers
    void Empty();
    CString & operator=(const char * str);
    CString & operator+=(const char * str);
    void TrimRight();
    int Replace(const char * oldStr,
                const char * newStr);
      // Replace all occurrences of oldStr with newStr.  Return the number of
      // replacements.

    // Fill in a printf-style string with values.
    template<typename T1>
    void Format(const char * formatStr, T1 val1)
    {
      sprintf(buffer, formatStr, val1);
      str_ = buffer;
    }
    template<typename T1, typename T2>
    void Format(const char * formatStr, T1 val1, T2 val2)
    {
      sprintf(buffer, formatStr, val1, val2);
      str_ = buffer;
    }
    template<typename T1, typename T2, typename T3>
    void Format(const char * formatStr, T1 val1, T2 val2, T3 val3)
    {
      sprintf(buffer, formatStr, val1, val2, val3);
      str_ = buffer;
    }


  private:
    std::string str_;
    static char buffer[];  // for Format() method -- not thread safe
};

#endif
