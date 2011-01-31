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

    // modifiers
    void Empty();
    CString & operator=(const char * str);
    CString & operator+=(const char * str);

  private:
    std::string str_;
};

#endif
