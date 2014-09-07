#include <timertt/all.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../time_limited_execution.hpp"

using namespace std::chrono;

typedef timertt::timer_wheel_thread_t<
		timertt::default_error_logger,
		timertt::default_actor_exception_handler >
	timer_thread_t;

UT_UNIT_TEST( several_full_rolls )
{
	run_with_time_limit(
		[]()
		{
			timer_thread_t tt( 3, milliseconds( 5 ) );

			tt.start();

			std::string v;

			auto id = tt.allocate();
			tt.activate(
					id,
					//microseconds( 100 ),
					milliseconds( 100 ),
					milliseconds( 27 ),
					[&v, &id, &tt]() {
						v += "1";
						if( v.size() >= 4 )
							tt.deactivate( id );
					} );

			std::this_thread::sleep_for( milliseconds( 250 ) );
			tt.shutdown_and_join();

			UT_CHECK_EQ( v, "1111" );
		},
		1,
		"several_full_rolls" );
}

UT_UNIT_TEST( demands_cleanup_on_shutdown )
{
	run_with_time_limit(
		[]()
		{
			timer_thread_t tt( 20, seconds(10) );

			tt.start();

			int dealloc_counter = 0;
			struct decrementer_t
			{
				int & m_var;

				decrementer_t( int & var ) : m_var( var ) { }
				~decrementer_t() { --m_var; }
			};

			{
				auto d1 = std::make_shared< decrementer_t >( std::ref(dealloc_counter) );
				auto d2 = std::make_shared< decrementer_t >( std::ref(dealloc_counter) );
				auto d3 = std::make_shared< decrementer_t >( std::ref(dealloc_counter) );

				tt.activate( tt.allocate(), seconds( 10 ), [d1]() {} );
				tt.activate( tt.allocate(), seconds( 10 ), [d2]() {} );
				tt.activate( tt.allocate(), seconds( 10 ), [d3]() {} );
			}

			tt.shutdown_and_join();

			UT_CHECK_EQ( dealloc_counter, -3 );
		},
		1,
		"demands_cleanup_on_shutdown" );
}

UT_UNIT_TEST( demands_deletion_during_processing )
{
	run_with_time_limit(
		[]()
		{
			timer_thread_t tt( 50, milliseconds(100) );

			tt.start();

			int events = 0;

			auto d1 = tt.allocate();
			auto d2 = tt.allocate();
			auto d3 = tt.allocate();
			auto d4 = tt.allocate();

			tt.activate( d1, milliseconds( 110 ),
				[&]() {
					++events;
					tt.deactivate( d1 );
					tt.deactivate( d2 );
					tt.deactivate( d3 );
					tt.deactivate( d4 );
				} );
			tt.activate( d2, milliseconds( 110 ), [&events]() { ++events; } );
			tt.activate( d3, milliseconds( 110 ), [&events]() { ++events; } );
			tt.activate( d4, milliseconds( 110 ), [&events]() { ++events; } );

			std::this_thread::sleep_for( milliseconds( 250 ) );

			tt.shutdown_and_join();

			UT_CHECK_EQ( events, 1 );
		},
		1,
		"demands_deletion_during_processing" );
}

UT_UNIT_TEST( demands_deletion_during_processing_2 )
{
	run_with_time_limit(
		[]()
		{
			timer_thread_t tt( 50, milliseconds(100) );

			tt.start();

			int events = 0;

			auto d1 = tt.allocate();
			auto d2 = tt.allocate();
			auto d3 = tt.allocate();
			auto d4 = tt.allocate();
			auto d5 = tt.allocate();
			auto d6 = tt.allocate();

			// Must be executed only once.
			// events += 1 (1).
			tt.activate( d1, milliseconds( 110 ),
				[&]() {
					++events;
					tt.deactivate( d1 );
				} );
			// Must be executed twice.
			// Will be stopped by d3.
			// events += 2 (3).
			tt.activate( d2,
					milliseconds( 110 ),
					milliseconds( 100 ),
					[&]() {
						++events;
					} );
			// Must be executed twice.
			// Will be stopped by itself.
			// events += 2 (5).
			tt.activate( d3,
					milliseconds( 110 ),
					milliseconds( 100 ),
					[&]() {
						++events;
						static int calls = 0;
						++calls;
						if( 2 == calls )
						{
							tt.deactivate( d2 );
							tt.deactivate( d3 );
						}
					} );
			// Must be executed only once.
			// events += 1 (6).
			tt.activate( d4,
					milliseconds( 110 ),
					[&]() {
						++events;
						tt.deactivate( d5 );
					} );
			// Must not be executed at all.
			// It will be stopped by d4 just before execution.
			tt.activate( d5,
					milliseconds( 110 ),
					milliseconds( 110 ),
					[&]() {
						++events;
						tt.deactivate( d6 );
					} );
			// Will be executed 6 times.
			// events += 6 (12).
			tt.activate( d6,
					milliseconds( 110 ),
					milliseconds( 110 ),
					[&]() {
						++events;
					} );

			std::this_thread::sleep_for( milliseconds( 650 ) );

			tt.shutdown_and_join();

			UT_CHECK_EQ( events, 12 );
		},
		1,
		"demands_deletion_during_processing_2" );
}

UT_UNIT_TEST( shutdown_from_the_timer )
{
	run_with_time_limit(
		[]()
		{
			timer_thread_t tt;

			tt.start();

			tt.activate( tt.allocate(), milliseconds( 25 ),
				[&]() {
					tt.shutdown();
				} );

			tt.join();
		},
		1,
		"shutdown_from_the_timer" );
}

UT_UNIT_TEST( shutdown_with_restarts )
{
	run_with_time_limit(
		[]()
		{
			timer_thread_t tt;

			int events = 0;
			for( int i = 0; i != 3; ++i )
			{
				tt.start();

				tt.activate( tt.allocate(), milliseconds( 5 ),
					[&]() {
						++events;
						tt.shutdown();
					} );

				tt.join();
			}

			UT_CHECK_EQ( events, 3 );
		},
		1,
		"shutdown_from_the_timer" );
}

int main()
{
	UT_RUN_UNIT_TEST( several_full_rolls )
	UT_RUN_UNIT_TEST( demands_cleanup_on_shutdown )
	UT_RUN_UNIT_TEST( demands_deletion_during_processing )
	UT_RUN_UNIT_TEST( demands_deletion_during_processing_2 )
	UT_RUN_UNIT_TEST( shutdown_from_the_timer )
	UT_RUN_UNIT_TEST( shutdown_with_restarts )
}

