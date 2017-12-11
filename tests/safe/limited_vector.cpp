#include "qcommon/safe/limited_vector.h"

#include <memory>
#include <string>
#include <iterator>

#include <boost/test/unit_test.hpp>

using IntVector = Q::LimitedVector< int, 10 >;
using StringVector = Q::LimitedVector< std::string, 10 >;
using IntPtrVector = Q::LimitedVector< std::unique_ptr< int >, 10 >;
using SharedPtrVector = Q::LimitedVector< std::shared_ptr< int >, 10 >;

BOOST_AUTO_TEST_SUITE( safe )

BOOST_AUTO_TEST_SUITE( limited_vector )

BOOST_AUTO_TEST_CASE( default_construct )
{
	IntVector intVec;
	BOOST_CHECK_EQUAL( intVec.size(), 0 );
	BOOST_CHECK( intVec.begin() == intVec.end() );
	BOOST_CHECK( intVec.empty() );
}

BOOST_AUTO_TEST_CASE( fill_and_copy )
{
	StringVector stringVec;
	BOOST_CHECK_EQUAL( stringVec.size(), 0 );
	BOOST_CHECK( stringVec.begin() == stringVec.end() );
	BOOST_CHECK( stringVec.empty() );

	BOOST_CHECK( stringVec.push_back( "hello world" ) );
	BOOST_CHECK( stringVec.emplace_back( "emplaced" ) );

	BOOST_CHECK_EQUAL( stringVec.size(), 2 );
	BOOST_CHECK( !stringVec.empty() );
	BOOST_CHECK_EQUAL( std::distance( stringVec.begin(), stringVec.end() ), 2 );

	auto it = stringVec.begin();
	auto cit = stringVec.cbegin();

	BOOST_CHECK( it != stringVec.end() );
	BOOST_CHECK( cit != stringVec.end() );
	BOOST_CHECK_EQUAL( *it, "hello world" );
	BOOST_CHECK_EQUAL( *cit, "hello world" );
	BOOST_CHECK_EQUAL( stringVec[ 0 ], "hello world" );

	++it;
	++cit;

	BOOST_CHECK( it != stringVec.end() );
	BOOST_CHECK( cit != stringVec.end() );
	BOOST_CHECK_EQUAL( *it, "emplaced" );
	BOOST_CHECK_EQUAL( *cit, "emplaced" );
	BOOST_CHECK_EQUAL( stringVec[ 1 ], "emplaced" );

	++it;
	++cit;

	BOOST_CHECK( it == stringVec.end() );
	BOOST_CHECK( cit == stringVec.end() );

	StringVector stringVecCopy = stringVec;

	BOOST_CHECK( stringVec == stringVecCopy );
	BOOST_CHECK( !( stringVec != stringVecCopy ) );
	BOOST_CHECK_EQUAL( stringVec.size(), 2 );
	BOOST_CHECK_EQUAL( stringVec.size(), stringVecCopy.size() );
	BOOST_CHECK_EQUAL( stringVec[ 0 ], stringVecCopy[ 0 ] );
	BOOST_CHECK_EQUAL( stringVec[ 1 ], stringVecCopy[ 1 ] );

	stringVec.clear();

	BOOST_CHECK( stringVec.empty() );
	BOOST_CHECK_EQUAL( stringVec.size(), 0 );
	BOOST_CHECK( stringVec != stringVecCopy );
	BOOST_CHECK( !( stringVec == stringVecCopy ) );

	stringVec = stringVecCopy;

	BOOST_CHECK( stringVec == stringVecCopy );
	BOOST_CHECK( !( stringVec != stringVecCopy ) );
	BOOST_CHECK_EQUAL( stringVec.size(), 2 );
	BOOST_CHECK_EQUAL( stringVec.size(), stringVecCopy.size() );
	BOOST_CHECK_EQUAL( stringVec[ 0 ], stringVecCopy[ 0 ] );
	BOOST_CHECK_EQUAL( stringVec[ 1 ], stringVecCopy[ 1 ] );
}

BOOST_AUTO_TEST_CASE( fill_and_move )
{
	IntPtrVector vec1, vec2;
	BOOST_CHECK( vec1.push_back( std::unique_ptr< int >( new int(42) ) ) );
	BOOST_CHECK( vec1.push_back( std::unique_ptr< int >( new int(1337) ) ) );
	BOOST_CHECK( vec2.push_back( std::unique_ptr< int >( new int(0) ) ) );
	BOOST_CHECK( vec2.emplace_back() ); // nullptr3

	BOOST_CHECK_EQUAL( vec1.size(), 2 );
	BOOST_CHECK_EQUAL( vec2.size(), 2 );
	BOOST_CHECK_EQUAL( *vec1[ 0 ], 42 );
	BOOST_CHECK_EQUAL( *vec1[ 1 ], 1337 );
	BOOST_CHECK_EQUAL( *vec2[ 0 ], 0 );
	BOOST_CHECK( !vec2[ 1 ] );

	vec1.swap( vec2 );

	BOOST_CHECK_EQUAL( vec1.size(), 2 );
	BOOST_CHECK_EQUAL( vec2.size(), 2 );
	BOOST_CHECK_EQUAL( *vec2[ 0 ], 42 );
	BOOST_CHECK_EQUAL( *vec2[ 1 ], 1337 );
	BOOST_CHECK_EQUAL( *vec1[ 0 ], 0 );
	BOOST_CHECK( !vec1[ 1 ] );

	vec1 = std::move( vec2 );

	BOOST_CHECK_EQUAL( vec1.size(), 2 );
	BOOST_CHECK_EQUAL( vec2.size(), 0 );
	BOOST_CHECK_EQUAL( *vec1[ 0 ], 42 );
	BOOST_CHECK_EQUAL( *vec1[ 1 ], 1337 );
}

BOOST_AUTO_TEST_CASE( member_destruction )
{
	// check for proper destruction of members
	std::shared_ptr< int > counter = std::make_shared< int >( 42 );
	{
		BOOST_CHECK_EQUAL( counter.use_count(), 1 );
		SharedPtrVector vec;
		BOOST_CHECK( vec.push_back( counter ) );
		BOOST_CHECK( vec.push_back( counter ) );
		BOOST_CHECK( vec.push_back( counter ) );
		BOOST_CHECK_EQUAL( counter.use_count(), 4 );
		vec.pop_back();
		BOOST_CHECK_EQUAL( counter.use_count(), 3 );
		SharedPtrVector vec2 = vec;
		BOOST_CHECK_EQUAL( counter.use_count(), 5 );
		vec = std::move( vec2 );
		BOOST_CHECK_EQUAL( counter.use_count(), 3 );
	}
	BOOST_CHECK_EQUAL( counter.use_count(), 1 );
}

BOOST_AUTO_TEST_SUITE_END() // limited_vector

BOOST_AUTO_TEST_SUITE_END() // safe

