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

BOOST_AUTO_TEST_CASE( CompareNoCase_same )
{
  CString str("foo bar");

  BOOST_REQUIRE( str.CompareNoCase("foo bar") == 0 );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( CompareNoCase_equal )
{
  CString str("foo bar");

  BOOST_REQUIRE( str.CompareNoCase("FOO Bar") == 0 );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( CompareNoCase_less )
{
  CString str("foo bar");

  BOOST_REQUIRE( str.CompareNoCase("Foo Bar Qux") < 0 );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( CompareNoCase_greater )
{
  CString str("foo bar");

  BOOST_REQUIRE( str.CompareNoCase("Foo BaLL") > 0 );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( compound_assignment )
{
  CString str("foo");
  str += ".bar";
  BOOST_REQUIRE_EQUAL( str, "foo.bar" );
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

BOOST_AUTO_TEST_CASE( Find_char_inMiddle )
{
  CString str("Hello World!");
  BOOST_REQUIRE_EQUAL( str.Find('W'), 6 );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Find_char_atStart )
{
  CString str("Hello World!");
  BOOST_REQUIRE_EQUAL( str.Find("H"), 0 );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Find_char_atEnd )
{
  CString str("Hello World!");
  BOOST_REQUIRE_EQUAL( str.Find('!'), 11 );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Find_char_multipleOccurrences )
{
  CString str("one:two:three");
  BOOST_REQUIRE_EQUAL( str.Find(':'), 3 );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Find_char_notPresent )
{
  CString str("one:two:three");
  BOOST_REQUIRE_EQUAL( str.Find("+"), -1 );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Left_zero )
{
  CString str("foo");
  BOOST_REQUIRE_EQUAL( str.Left(0), "" );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Left_one )
{
  CString str("foo");
  BOOST_REQUIRE_EQUAL( str.Left(1), "f" );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Left_many )
{
  CString str("Hello World!");
  BOOST_REQUIRE_EQUAL( str.Left(6), "Hello " );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Mid_atStart )
{
  CString str("Hello World!");
  BOOST_REQUIRE_EQUAL( str.Mid(0), "Hello World!" );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Mid_inMiddle )
{
  CString str("Hello World!");
  BOOST_REQUIRE_EQUAL( str.Mid(5), " World!" );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Mid_atEnd )
{
  CString str("Hello World!");
  BOOST_REQUIRE_EQUAL( str.Mid(11), "!" );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Replace_one )
{
  CString str("foo bar");
  BOOST_REQUIRE_EQUAL( str.Replace("bar", "qux"), 1 );
  BOOST_REQUIRE_EQUAL( str, "foo qux");
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Replace_many )
{
  CString str("foo ABBA bar ABBA qux ABBABBA ...");
  BOOST_REQUIRE_EQUAL( str.Replace("ABBA", "*"), 3 );
  BOOST_REQUIRE_EQUAL( str, "foo * bar * qux *BBA ...");
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Replace_none )
{
  CString str("foo bar");
  BOOST_REQUIRE_EQUAL( str.Replace("qux", "?"), 0 );
  BOOST_REQUIRE_EQUAL( str, "foo bar");
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( MakeLower_mixed )
{
  CString str("Hello World.");
  str.MakeLower();
  BOOST_REQUIRE_EQUAL( str, "hello world." );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( MakeLower_allUpper )
{
  CString str("FOO BAR?!");
  str.MakeLower();
  BOOST_REQUIRE_EQUAL( str, "foo bar?!" );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( MakeLower_alreadyLower )
{
  CString str("foo bar");
  str.MakeLower();
  BOOST_REQUIRE_EQUAL( str, "foo bar" );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( MakeLower_empty )
{
  CString str("");
  str.MakeLower();
  BOOST_REQUIRE_EQUAL( str, "" );
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Format_intArg )
{
  CString str;
  str.Format("i = %d", 45);
  BOOST_REQUIRE_EQUAL( str, "i = 45");
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Format_strArg )
{
  CString str;
  str.Format("Hello %s!", "World");
  BOOST_REQUIRE_EQUAL( str, "Hello World!");
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( Format_3Args )
{
  CString str;
  str.Format("int = %d, float = %.2f, string = \"%s\"", -123, 4.5, "Foo Bar");
  BOOST_REQUIRE_EQUAL( str, "int = -123, float = 4.50, string = \"Foo Bar\"");
}

//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE_END()
