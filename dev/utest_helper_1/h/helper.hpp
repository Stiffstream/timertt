#pragma once

#include <cstdlib>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace utest_helper_1
{

inline void
fail(
	const char * file,
	int line,
	const std::string & message )
{
	std::ostringstream what;
	what << file << ':' << line << ": " << message;
	throw std::runtime_error( what.str() );
}

template< typename Actual, typename Expected >
inline void
check_eq(
	const Actual & actual,
	const Expected & expected,
	const char * actual_expr,
	const char * expected_expr,
	const char * file,
	int line )
{
	if( !(actual == expected) )
	{
		std::ostringstream message;
		message << "expected " << actual_expr << " == " << expected_expr;
		fail( file, line, message.str() );
	}
}

template< typename Left, typename Right >
inline void
check_gt(
	const Left & left,
	const Right & right,
	const char * left_expr,
	const char * right_expr,
	const char * file,
	int line )
{
	if( !(left > right) )
	{
		std::ostringstream message;
		message << "expected " << left_expr << " > " << right_expr;
		fail( file, line, message.str() );
	}
}

template< typename Test >
inline void
run_unit_test(
	const char * name,
	Test test )
{
	std::cout << "[ RUN      ] " << name << std::endl;
	try
	{
		test();
		std::cout << "[       OK ] " << name << std::endl;
	}
	catch( const std::exception & x )
	{
		std::cerr << "[  FAILED  ] " << name << ": " << x.what() << std::endl;
		std::exit( 2 );
	}
	catch( ... )
	{
		std::cerr << "[  FAILED  ] " << name << ": unknown exception" << std::endl;
		std::exit( 2 );
	}
}

} /* namespace utest_helper_1 */

#define UT_UNIT_TEST(name) void name()

#define UT_RUN_UNIT_TEST(name) \
	::utest_helper_1::run_unit_test( #name, [] { name(); } );

#define UT_CHECK_EQ(actual, expected) \
	::utest_helper_1::check_eq( \
		(actual), (expected), #actual, #expected, __FILE__, __LINE__ )

#define UT_CHECK_GT(left, right) \
	::utest_helper_1::check_gt( \
		(left), (right), #left, #right, __FILE__, __LINE__ )

#define UT_CHECK_CONDITION(condition) \
	do { \
		if( !(condition) ) \
			::utest_helper_1::fail( \
				__FILE__, __LINE__, "expected condition: " #condition ); \
	} while( false )

#define UT_CHECK_THROW(exception_type, expression) \
	do { \
		bool utest_helper_caught_expected_exception = false; \
		try { \
			expression; \
		} \
		catch( const exception_type & ) { \
			utest_helper_caught_expected_exception = true; \
		} \
		catch( ... ) { \
			::utest_helper_1::fail( \
				__FILE__, __LINE__, \
				"unexpected exception type from expression: " #expression ); \
		} \
		if( !utest_helper_caught_expected_exception ) \
			::utest_helper_1::fail( \
				__FILE__, __LINE__, \
				"expected exception " #exception_type " from expression: " #expression ); \
	} while( false )

