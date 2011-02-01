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

#define BOOST_TEST_DYN_LINK

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

#include "getApplicationPath.h"

BOOST_AUTO_TEST_SUITE( getApplicationPath_tests )

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( fileExists )
{
  const char * const appPath = getApplicationPath();
  BOOST_TEST_MESSAGE( "application path = \"" << appPath << "\"" );

  BOOST_REQUIRE( boost::filesystem::exists(appPath) );
  BOOST_REQUIRE( boost::filesystem::is_regular_file(appPath) );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE_END()
