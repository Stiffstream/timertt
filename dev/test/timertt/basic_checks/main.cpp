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
			timertt::timer_thread_t tt;

			tt.start();
		},
		1,
		"no_demands" );
}

UT_UNIT_TEST( schedule_when_not_started )
{
	timertt::timer_thread_t tt;

	UT_CHECK_THROW( std::runtime_error,
			tt.schedule( milliseconds( 5 ), []() {} ) );

	UT_CHECK_THROW( std::runtime_error,
			tt.schedule( milliseconds( 5 ), microseconds( 100 ), []() {} ) );
}

UT_UNIT_TEST( single_shot )
{
	run_with_time_limit(
		[]()
		{
			timertt::timer_thread_t tt;

			tt.start();

			std::string v;

			tt.schedule( milliseconds( 20 ), [&v]() { v = "hello"; } );

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
			timertt::timer_thread_t tt;

			tt.start();

			std::string v;
			timertt::timer_thread_t::id_t id;

			id = tt.schedule(
					microseconds( 500 ),
					milliseconds( 25 ),
					[&v, &id, &tt]() {
						v += "1";
						if( v.size() >= 4 )
							tt.erase( id );
					} );

			std::this_thread::sleep_for( milliseconds( 150 ) );

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
			timertt::timer_thread_t tt;

			tt.start();

			std::string v;

			tt.schedule( milliseconds( 80 ), [&v]() { v += "(80)"; } );
			tt.schedule( milliseconds( 60 ), [&v]() { v += "(60)"; } );
			tt.schedule( milliseconds( 100 ), [&v]() { v += "(100)"; } );
			tt.schedule( milliseconds( 40 ), [&v]() { v += "(40)"; } );
			tt.schedule( milliseconds( 120 ), [&v]() { v += "(120)"; } );
			tt.schedule( milliseconds( 20 ), [&v]() { v += "(20)"; } );
			tt.schedule( milliseconds( 90 ), [&v]() { v += "(90)"; } );
			tt.schedule( milliseconds( 140 ), [&v]() { v += "(140)"; } );

			std::this_thread::sleep_for( milliseconds( 200 ) );

			tt.stop();

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
			timertt::timer_thread_t tt;

			tt.start();

			std::string v;

			tt.schedule(
					milliseconds( 150 ),
					milliseconds( 150 ),
					[&v]() { v += "(150/150)"; } );

			timertt::timer_thread_t::id_t id2;
			id2 = tt.schedule(
					milliseconds( 200 ),
					milliseconds( 200 ),
					[&v, &id2, &tt]() {
						static int i = 0;
						v += "(200/200)";
						if( ++i == 2 )
							tt.erase( id2 );
					} );

			tt.schedule(
					milliseconds( 500 ),
					milliseconds( 150 ),
					[&v]() {
						v += "(500/150)";
					} );

			timertt::timer_thread_t::id_t id3;
			id3 = tt.schedule(
					milliseconds( 940 ),
					milliseconds( 20 ),
					[&v, &id3, &tt]() {
						static int i = 0;
						v += "(940/20)";
						if( ++i == 2 )
							tt.erase( id3 );
					} );

			std::this_thread::sleep_for( milliseconds( 1000 ) );

			tt.stop();

			const std::string expected =
					"(150/150)(200/200)(150/150)(200/200)(150/150)(500/150)"
					"(150/150)(500/150)(150/150)(500/150)(150/150)(940/20)"
					"(500/150)(940/20)";
			UT_CHECK_EQ( v, expected );
		},
		5,
		"several_periodics" );
}

int main()
{
	UT_RUN_UNIT_TEST( no_demands )
	UT_RUN_UNIT_TEST( schedule_when_not_started )
	UT_RUN_UNIT_TEST( single_shot )
	UT_RUN_UNIT_TEST( single_periodic )
	UT_RUN_UNIT_TEST( several_single_shots )
	UT_RUN_UNIT_TEST( several_periodics )
}

