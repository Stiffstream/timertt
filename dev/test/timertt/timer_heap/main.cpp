#include <vector>
#include <random>
#include <algorithm>

#include <timertt/all.hpp>

#include <utest_helper_1/h/helper.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

using namespace std::chrono;

UT_UNIT_TEST( execution_order )
{
	using timer_thread_t = timertt::timer_heap_thread_template<
		timertt::default_timer_action_type,
		timertt::default_error_logger,
		timertt::default_actor_exception_handler >;

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
						[&dest, i] () {
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

UT_UNIT_TEST( manager_remove_timers )
{
	using timer_manager_t = timertt::timer_heap_manager_template<
		timertt::thread_safety::unsafe >;
	using timer_holder = timer_manager_t::timer_holder;

	const int test_size = 1000;
	const int timers_to_remove = 125;

	std::mt19937 gen{ std::random_device{}() };

	std::vector< int > timeouts;
	timeouts.reserve( test_size );
	for( int i = 0; i < test_size; ++i )
		timeouts.push_back( i );
	std::shuffle( timeouts.begin(), timeouts.end(), gen );

	std::vector< timer_holder > timers;
	timers.reserve( test_size );

	std::vector< int > dest;
	dest.reserve( test_size );

	timer_manager_t manager;

	for( int i = 0; i < test_size; ++i )
	{
		timers.push_back( manager.allocate() );
		manager.activate(
				timers.back(),
				milliseconds{ timeouts[ i ] },
				[&dest, v = timeouts[ i ]]() {
					dest.push_back( v );
				} );
	}

	std::uniform_int_distribution<> distrib{ 0, test_size - 1 };
	for( int i = 0; i < timers_to_remove; ++i )
	{
		manager.deactivate(
				timers[ static_cast<std::size_t>(distrib( gen )) ] );
	}

	std::this_thread::sleep_for( milliseconds{ test_size + 200 } );
	manager.process_expired_timers();

	UT_CHECK_EQ( true, manager.empty() );
	UT_CHECK_GT( static_cast<std::size_t>(test_size), dest.size() );

	const std::size_t i_max = dest.size() - 1u;
	for( std::size_t i = 0; i < i_max; ++i )
		UT_CHECK_LT( dest[ i ], dest[ i + 1 ] );
}

// Source: https://github.com/Stiffstream/sobjectizer/issues/105
UT_UNIT_TEST( sobjectizer_issue_105 )
{
	using timer_manager_t = timertt::timer_heap_manager_template<
		timertt::thread_safety::unsafe >;
	using timer_holder = timer_manager_t::timer_holder;

	std::vector< timer_holder > timers;

	std::vector< int > dest;

	timer_manager_t manager;

	const auto append = [&timers, &dest, &manager]( int t ) {
		timers.push_back( manager.allocate() );
		manager.activate( timers.back(),
				microseconds{ t },
				[&dest, t]() {
					dest.push_back( t );
				} );
	};

	for( int i : std::initializer_list< int >{
			0, 0, 25, 49, 26, 25, 49, 2051, 2051, 2051,
			1058, 50, 998, 298025, 4060, 8025, 3025, 7998,
			2051, 28057, 3057, 863980, 28998, 118079, 60,
			58070, 998 } )
	{
		append( i );
	}

	UT_CHECK_EQ( std::size_t{ 27 }, timers.size() );

	manager.deactivate( timers[ 22 ] );
	
	std::this_thread::sleep_for( milliseconds{ 1500 } );
	manager.process_expired_timers();

	UT_CHECK_EQ( true, manager.empty() );
	UT_CHECK_EQ( timers.size() - 1, dest.size() );

	const std::size_t i_max = dest.size() - 1u;
	for( std::size_t i = 0; i < i_max; ++i )
		UT_CHECK_LE( dest[ i ], dest[ i + 1 ] );
}

int main()
{
	UT_RUN_UNIT_TEST( execution_order )
	UT_RUN_UNIT_TEST( manager_remove_timers )
	UT_RUN_UNIT_TEST( sobjectizer_issue_105 )
}

