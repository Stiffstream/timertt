#include <vector>
#include <random>
#include <algorithm>

#include <timertt/all.hpp>

#include <utest_helper_1/h/helper.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

using namespace std::chrono;

using timer_thread_t = timertt::timer_heap_thread_template<
	timertt::default_error_logger,
	timertt::default_actor_exception_handler >;

UT_UNIT_TEST( execution_order )
{
	const int test_size = 300;
	std::vector< int > src; src.reserve( test_size );
	std::vector< int > dest; dest.reserve( test_size );

	for( int i = 1; i <= test_size; ++i )
		src.push_back( i * 10 );

	std::shuffle( src.begin(), src.end(),
			std::mt19937( std::random_device()() ) );

	run_with_time_limit(
		[&src, &dest]()
		{
			timer_thread_t tt;

			tt.start();

			for( auto i : src )
				tt.activate(
						milliseconds( 300 ) + milliseconds( i ),
						[&dest, i, &tt] () {
							dest.push_back( i );
						} );

			std::this_thread::sleep_for( milliseconds( 3500 ) );
			tt.shutdown_and_join();

			std::sort( src.begin(), src.end() );

			UT_CHECK_EQ( src.size(), dest.size() );
			for( std::vector< int >::iterator s = src.begin(), d = dest.begin();
					s != src.end(); ++s, ++d )
				UT_CHECK_EQ( *s, *d );
		},
		5,
		"execution_order" );
}

int main()
{
	UT_RUN_UNIT_TEST( execution_order )
}

