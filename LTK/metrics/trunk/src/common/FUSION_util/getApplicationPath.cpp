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

#include <cstdio>
#include <sstream>
#include <stdexcept>

#include "getApplicationPath.h"

namespace
{
  char applicationPath[FILENAME_MAX + 1];
  bool firstCall = true;
  void setApplicationPath();
}

//-----------------------------------------------------------------------------

const char * const getApplicationPath()
{
  if (firstCall) {
    firstCall = false;
    setApplicationPath();
  }
  return applicationPath;
}

//-----------------------------------------------------------------------------

#if defined(__APPLE__) && defined(__MACH__)

#include <mach-o/dyld.h>

namespace
{
  void setApplicationPath()
  {
    uint32_t bufsize = uint32_t(FILENAME_MAX + 1);
    if (_NSGetExecutablePath(applicationPath, &bufsize) != 0) {
      std::ostringstream message;
      message << "buffer for application path is too small (size = "
              << FILENAME_MAX + 1 << ", required = " << bufsize << ")";
      throw std::runtime_error(message.str());
    }
  }
}

//-----------------------------------------------------------------------------

#elif defined(_WIN32)

#include <windows.h>

namespace
{
  void setApplicationPath()
  {
    if (GetModuleFileName(NULL, applicationPath, FILENAME_MAX + 1) == 0) {
      std::ostringstream message;
      message << "GetModuleFileName function failed (error code = "
              << GetLastError() << ")";
      throw std::runtime_error(message.str());
    }
  }
}

//-----------------------------------------------------------------------------

#else

namespace
{
  void setApplicationPath()
  {
    throw std::runtime_error("getApplicationPath is not implemented on this platform");
  }
}

//-----------------------------------------------------------------------------

#endif
