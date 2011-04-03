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

#include <boost/filesystem.hpp>

#include "DeleteFile.h"

// Implement the Windows DeleteFile system function using the counterpart
// function in Boost FileSystem library.

BOOL DeleteFile(LPCTSTR filename)
{
  // According the on-line Reference Documentation for Boost FileSystem 1.39:
  //
  //   http://www.boost.org/doc/libs/1_39_0/libs/filesystem/doc/reference.html
  //
  // the "remove" operation is declared as:
  //
  //   template <class Path> void remove(const Path& p, system::error_code & ec = singular );
  //
  // But the actual source code in 1.39 declares the "remove" operation as:
  //
  //   template <class Path> bool remove(const Path& p);

  if (boost::filesystem::remove(filename))
    return TRUE;
  else
   return FALSE;
}
