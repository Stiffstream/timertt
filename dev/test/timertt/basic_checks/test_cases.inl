#include <iostream>

#include <timertt/all.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../time_limited_execution.hpp"

using namespace std::chrono;

UT_UNIT_TEST( no_demands )
{
	run_with_time_limit(
		[]()
		{
			timer_thread_t tt;

			tt.start();
		},
		1,
		"no_demands" );
}

UT_UNIT_TEST( schedule_when_not_started )
{
	timer_thread_t tt;

	UT_CHECK_THROW( std::runtime_error,
			tt.activate(
					tt.allocate(),
					milliseconds( 5 ),
					[]() {} ) );

	UT_CHECK_THROW( std::runtime_error,
			tt.activate(
					tt.allocate(),
					milliseconds( 5 ),
					microseconds( 100 ),
					[]() {} ) );
}

UT_UNIT_TEST( single_shot )
{
	run_with_time_limit(
		[]()
		{
			timer_thread_t tt;

			tt.start();

			std::string v;

			tt.activate( tt.allocate(),
					milliseconds( 20 ), [&v]() { v = "hello"; } );

			std::this_thread::sleep_for( milliseconds( 100 ) );

			UT_CHECK_EQ( v, "hello" );
		},
		1,
		"single_shot" );
}

UT_UNIT_TEST( single_periodic )
{
	run_with_time_limit(
		[]()
		{
			timer_thread_t tt;

			tt.start();

			std::string v;
			auto id = tt.allocate();

			tt.activate(
					id,
					microseconds( 500 ),
					milliseconds( 25 ),
					[&v, &id, &tt]() {
						v += "1";
						if( v.size() >= 4 )
							tt.deactivate( id );
					} );

			std::this_thread::sleep_for( milliseconds( 150 ) );
			tt.shutdown_and_join();

			UT_CHECK_EQ( v, "1111" );
		},
		1,
		"single_periodic" );
}

UT_UNIT_TEST( several_single_shots )
{
	run_with_time_limit(
		[]()
		{
			timer_thread_t tt;

			tt.start();

			std::string v;

			tt.activate( tt.allocate(),
					milliseconds( 80 ), [&v]() { v += "(80)"; } );
			tt.activate( tt.allocate(),
					milliseconds( 60 ), [&v]() { v += "(60)"; } );
			tt.activate( tt.allocate(),
					milliseconds( 100 ), [&v]() { v += "(100)"; } );
			tt.activate( tt.allocate(),
					milliseconds( 40 ), [&v]() { v += "(40)"; } );
			tt.activate( tt.allocate(),
					milliseconds( 120 ), [&v]() { v += "(120)"; } );
			tt.activate( tt.allocate(),
					milliseconds( 20 ), [&v]() { v += "(20)"; } );
			tt.activate( tt.allocate(),
					milliseconds( 90 ), [&v]() { v += "(90)"; } );
			tt.activate( tt.allocate(),
					milliseconds( 140 ), [&v]() { v += "(140)"; } );

			std::this_thread::sleep_for( milliseconds( 200 ) );

			tt.shutdown_and_join();

			UT_CHECK_EQ( v, "(20)(40)(60)(80)(90)(100)(120)(140)" );
		},
		1,
		"several_single_shots" );
}

UT_UNIT_TEST( several_periodics )
{
	run_with_time_limit(
		[]()
		{
			timer_thread_t tt;

			tt.start();

			std::string v;

			tt.activate(
					tt.allocate(),
					milliseconds( 150 ),
					milliseconds( 150 ),
					[&v]() { v += "(150/150)"; } );

			auto id2 = tt.allocate();
			tt.activate(
					id2,
					milliseconds( 200 ),
					milliseconds( 200 ),
					[&v, &id2, &tt]() {
						static int i = 0;
						v += "(200/200)";
						if( ++i == 2 )
							tt.deactivate( id2 );
					} );

			tt.activate(
					tt.allocate(),
					milliseconds( 500 ),
					milliseconds( 150 ),
					[&v]() {
						v += "(500/150)";
					} );

			auto id3 = tt.allocate();
			tt.activate(
					id3,
					milliseconds( 940 ),
					milliseconds( 20 ),
					[&v, &id3, &tt]() {
						static int i = 0;
						v += "(940/20)";
						if( ++i == 2 )
							tt.deactivate( id3 );
					} );

			std::this_thread::sleep_for( milliseconds( 1000 ) );

			tt.shutdown_and_join();

			const std::string expected =
					"(150/150)(200/200)(150/150)(200/200)(150/150)(500/150)"
					"(150/150)(500/150)(150/150)(500/150)(150/150)(940/20)"
					"(500/150)(940/20)";
			UT_CHECK_EQ( v, expected );
		},
		5,
		"several_periodics" );
}

UT_UNIT_TEST( anonymous_timers )
{
	run_with_time_limit(
		[]()
		{
			timer_thread_t tt;

			tt.start();

			std::string v;

			tt.activate( milliseconds( 40 ), [&v]() { v += "(s1)"; } );
			tt.activate( milliseconds( 40 ),
					milliseconds( 40 ),
					[&v] () { v += "(s2)"; } );
			tt.activate( milliseconds( 100 ),
					[&v] () { v += "(s3)"; } );

			std::this_thread::sleep_for( milliseconds( 190 ) );

			tt.shutdown_and_join();

			UT_CHECK_EQ( v, "(s1)(s2)(s2)(s3)(s2)(s2)" );
		},
		1,
		"anonymous_timers" );
}

UT_UNIT_TEST( demands_cleanup_on_shutdown )
{
	run_with_time_limit(
		[]()
		{
			timer_thread_t tt;

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
			timer_thread_t tt;

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
			timer_thread_t tt;

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

			std::this_thread::sleep_for( milliseconds( 670 ) );

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
	UT_RUN_UNIT_TEST( no_demands )
	UT_RUN_UNIT_TEST( schedule_when_not_started )
	UT_RUN_UNIT_TEST( single_shot )
	UT_RUN_UNIT_TEST( single_periodic )
	UT_RUN_UNIT_TEST( several_single_shots )
	UT_RUN_UNIT_TEST( several_periodics )
	UT_RUN_UNIT_TEST( anonymous_timers )
	UT_RUN_UNIT_TEST( demands_cleanup_on_shutdown )
	UT_RUN_UNIT_TEST( demands_deletion_during_processing )
	UT_RUN_UNIT_TEST( demands_deletion_during_processing_2 )
	UT_RUN_UNIT_TEST( shutdown_from_the_timer )
	UT_RUN_UNIT_TEST( shutdown_with_restarts )
}

