#include "qcommon/safe/string.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( safe )

BOOST_AUTO_TEST_SUITE( string )

BOOST_AUTO_TEST_CASE( literals )
{
	auto test = CSTRING_VIEW( "test" );
	auto foo = CSTRING_VIEW( "foo" );

	BOOST_CHECK_EQUAL( test, CSTRING_VIEW( "test" ) );
	BOOST_CHECK_EQUAL( test.size(), 4 );

	BOOST_CHECK_EQUAL( foo, CSTRING_VIEW( "foo" ) );
	BOOST_CHECK_EQUAL( foo.size(), 3 );
	BOOST_CHECK_NE( test, foo );
}

BOOST_AUTO_TEST_CASE( stricmp )
{
	BOOST_CHECK_EQUAL( Q::stricmp( CSTRING_VIEW( "hello" ), CSTRING_VIEW( "HELLO" ) ), Q::Ordering::EQ );
	BOOST_CHECK_EQUAL( Q::stricmp( CSTRING_VIEW( "aaa" ), CSTRING_VIEW( "aab" ) ), Q::Ordering::LT );
	BOOST_CHECK_EQUAL( Q::stricmp( CSTRING_VIEW( "aab" ), CSTRING_VIEW( "aaa" ) ), Q::Ordering::GT );
	BOOST_CHECK_EQUAL( Q::stricmp( CSTRING_VIEW( "AAA" ), CSTRING_VIEW( "aab" ) ), Q::Ordering::LT );
	BOOST_CHECK_EQUAL( Q::stricmp( CSTRING_VIEW( "AAA" ), CSTRING_VIEW( "aab" ) ), Q::Ordering::LT );
	// prefix is smaller
	BOOST_CHECK_EQUAL( Q::stricmp( CSTRING_VIEW( "hello" ), CSTRING_VIEW( "hello world" ) ), Q::Ordering::LT );
	BOOST_CHECK_EQUAL( Q::stricmp( CSTRING_VIEW( "hello world" ), CSTRING_VIEW( "hello" ) ), Q::Ordering::GT );
	// edge case: empty strings
	BOOST_CHECK_EQUAL( Q::stricmp( CSTRING_VIEW( "" ), CSTRING_VIEW( "" ) ), Q::Ordering::EQ );
	BOOST_CHECK_EQUAL( Q::stricmp( CSTRING_VIEW( "" ), CSTRING_VIEW( "a" ) ), Q::Ordering::LT );
	BOOST_CHECK_EQUAL( Q::stricmp( CSTRING_VIEW( "a" ), CSTRING_VIEW( "" ) ), Q::Ordering::GT );
}

BOOST_AUTO_TEST_CASE( substr )
{
	BOOST_CHECK_EQUAL( Q::substr( CSTRING_VIEW( "Hello World" ), 6 ), CSTRING_VIEW( "World" ) );
	BOOST_CHECK_EQUAL( Q::substr( CSTRING_VIEW( "Hello World" ), 6, 100 ), CSTRING_VIEW( "World" ) );
	BOOST_CHECK_EQUAL( Q::substr( CSTRING_VIEW( "Hello World" ), 0, 5 ), CSTRING_VIEW( "Hello" ) );
	BOOST_CHECK_EQUAL( Q::substr( CSTRING_VIEW( "Hello my World!" ), 6, 2 ), CSTRING_VIEW( "my" ) );
	BOOST_CHECK_THROW( Q::substr( CSTRING_VIEW( "Hello" ), 20 ), std::out_of_range );
}

BOOST_AUTO_TEST_CASE( svtoi )
{
	BOOST_CHECK_EQUAL( Q::svtoi( CSTRING_VIEW( "" ) ), 0 );
	BOOST_CHECK_EQUAL( Q::svtoi( CSTRING_VIEW( "asdf" ) ), 0 );
	BOOST_CHECK_EQUAL( Q::svtoi( CSTRING_VIEW( "0" ) ), 0 );
	BOOST_CHECK_EQUAL( Q::svtoi( CSTRING_VIEW( "1" ) ), 1 );
	BOOST_CHECK_EQUAL( Q::svtoi( CSTRING_VIEW( "+1" ) ), 1 );
	BOOST_CHECK_EQUAL( Q::svtoi( CSTRING_VIEW( "-1" ) ), -1 );
	BOOST_CHECK_EQUAL( Q::svtoi( CSTRING_VIEW( "13foo" ) ), 13 );
	BOOST_CHECK_EQUAL( Q::svtoi( CSTRING_VIEW( "    13" ) ), 13 );
	BOOST_CHECK_EQUAL( Q::svtoi( CSTRING_VIEW( "13 27" ) ), 13 );
	BOOST_CHECK_EQUAL( Q::svtoi( CSTRING_VIEW( "12345" ) ), 12345 );
	BOOST_CHECK_EQUAL( Q::svtoi( CSTRING_VIEW( "-12345" ) ), -12345 );
}

BOOST_AUTO_TEST_CASE( sscanf )
{
	{
		float x = -1, y = -1, z = -1;
		BOOST_CHECK_EQUAL( Q::sscanf( CSTRING_VIEW( "  42  13.37" ), x, y, z ), 2 );
		BOOST_CHECK_EQUAL( x, 42.f );
		BOOST_CHECK_CLOSE_FRACTION( y, 13.37f, 0.1f ); // within 0.1% of each other
		BOOST_CHECK_EQUAL( z, -1 );
	}
	// TODO String tests
}

BOOST_AUTO_TEST_SUITE_END() // string

BOOST_AUTO_TEST_SUITE_END() // safe
