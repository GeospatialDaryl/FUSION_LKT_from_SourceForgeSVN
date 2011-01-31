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

#include <string>

#include <boost/test/unit_test.hpp>

#include "CString.h"

BOOST_AUTO_TEST_SUITE( CString_tests )

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( default_constructor )
{
  CString str;
  // Convert to std::string below so that std::string::operator== is used
  // instead of CString::operator== (want to test that separately).
  BOOST_REQUIRE_EQUAL( std::string(str), std::string("") );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( str_constructor )
{
  const char * initial_str = "Hello World!";
  CString str(initial_str);
  // Convert to std::string below so that std::string::operator== is used
  // instead of CString::operator== (want to test that separately).
  BOOST_REQUIRE_EQUAL( std::string(str), std::string(initial_str) );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( equality_operator )
{
  const char * initial_str = "foo bar";
  CString str(initial_str);

  BOOST_REQUIRE( str == initial_str );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( TrimRight_oneSpace )
{
  CString str("foo ");
  str.TrimRight();
  BOOST_REQUIRE_EQUAL( str, "foo" );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( TrimRight_manyWhitespace )
{
  CString str("foo bar \t \n");
  str.TrimRight();
  BOOST_REQUIRE_EQUAL( str, "foo bar" );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( TrimRight_allWhitespace )
{
  CString str("\t\n   \t \n");
  str.TrimRight();
  BOOST_REQUIRE_EQUAL( str, "" );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( TrimRight_noWhitespace )
{
  CString str("Four score and seven years ago");
  str.TrimRight();
  BOOST_REQUIRE_EQUAL( str, "Four score and seven years ago" );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Find_inMiddle )
{
  CString str("Hello World!");
  BOOST_REQUIRE_EQUAL( str.Find("World"), 6 );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Find_atStart )
{
  CString str("Hello World!");
  BOOST_REQUIRE_EQUAL( str.Find("He"), 0 );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Find_atEnd )
{
  CString str("Hello World!");
  BOOST_REQUIRE_EQUAL( str.Find("!"), 11 );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Find_multipleOccurrences )
{
  CString str("one:two:three");
  BOOST_REQUIRE_EQUAL( str.Find(":t"), 3 );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Find_notPresent )
{
  CString str("one:two:three");
  BOOST_REQUIRE_EQUAL( str.Find("+"), -1 );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE_END()