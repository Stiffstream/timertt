#include <iostream>

#include <timertt/all.hpp>

#include <utest_helper_1/h/helper.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

using namespace std::chrono;

UT_UNIT_TEST( timer_holder )
{
	run_with_time_limit(
		[]()
		{
			timer_thread_t tt;

			tt.start();

			std::string v;

			UT_CHECK_EQ( true, tt.empty() );

			// timer_holder name must be public.
			timer_thread_t::timer_holder id = tt.allocate();

			tt.activate( id,
					milliseconds( 20 ), test_action(v, "hello") );
			UT_CHECK_EQ( false, tt.empty() );

			std::this_thread::sleep_for( milliseconds( 100 ) );

			UT_CHECK_EQ( v, "hello" );
			UT_CHECK_EQ( true, tt.empty() );
		},
		1,
		"timer_holder" );
}

UT_UNIT_TEST( single_shot )
{
	run_with_time_limit(
		[]()
		{
			timer_thread_t tt;

			tt.start();

			std::string v;

			UT_CHECK_EQ( true, tt.empty() );
			tt.activate( tt.allocate(),
					milliseconds( 20 ), test_action(v, "hello") );
			UT_CHECK_EQ( false, tt.empty() );

			std::this_thread::sleep_for( milliseconds( 100 ) );

			UT_CHECK_EQ( v, "hello" );
			UT_CHECK_EQ( true, tt.empty() );
		},
		1,
		"single_shot" );
}

UT_UNIT_TEST( single_shot_scoped )
{
	run_with_time_limit(
		[]()
		{
			timer_thread_t tt;

			tt.start();

			std::string v;

			UT_CHECK_EQ( true, tt.empty() );

			timer_thread_t::scoped_timer_object timer;
			tt.activate( timer,
					milliseconds( 20 ), test_action(v, "hello") );
			UT_CHECK_EQ( false, tt.empty() );

			std::this_thread::sleep_for( milliseconds( 100 ) );

			UT_CHECK_EQ( v, "hello" );
			UT_CHECK_EQ( true, tt.empty() );
		},
		1,
		"single_shot_scoped" );
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
					milliseconds( 80 ), test_action(v, "(80)") );
			tt.activate( tt.allocate(),
					milliseconds( 60 ), test_action(v, "(60)") );
			tt.activate( tt.allocate(),
					milliseconds( 120 ), test_action(v, "(120)") );
			tt.activate( tt.allocate(),
					milliseconds( 40 ), test_action(v, "(40)") );
			tt.activate( tt.allocate(),
					milliseconds( 150 ), test_action(v, "(150)") );
			tt.activate( tt.allocate(),
					milliseconds( 20 ), test_action(v, "(20)") );
			tt.activate( tt.allocate(),
					milliseconds( 100 ), test_action(v, "(100)") );
			tt.activate( tt.allocate(),
					milliseconds( 170 ), test_action(v, "(170)") );

			std::this_thread::sleep_for( milliseconds( 200 ) );

			tt.shutdown_and_join();

			UT_CHECK_EQ( v, "(20)(40)(60)(80)(100)(120)(150)(170)" );
		},
		1,
		"several_single_shots" );
}

UT_UNIT_TEST( anonymous_timers )
{
	run_with_time_limit(
		[]()
		{
			timer_thread_t tt;

			tt.start();

			std::string v;

			tt.activate( milliseconds( 40 ), test_action(v, "(s1)") );
			tt.activate( milliseconds( 40 ),
					milliseconds( 40 ),
					test_action(v, "(s2)") );
			tt.activate( milliseconds( 100 ),
					test_action(v, "(s3)") );

			std::this_thread::sleep_for( milliseconds( 180 ) );

			tt.shutdown_and_join();

			UT_CHECK_EQ( v, "(s1)(s2)(s2)(s3)(s2)(s2)" );
		},
		1,
		"anonymous_timers" );
}

int main()
{
	UT_RUN_UNIT_TEST( timer_holder )
	UT_RUN_UNIT_TEST( single_shot )
	UT_RUN_UNIT_TEST( single_shot_scoped )
	UT_RUN_UNIT_TEST( several_single_shots )
	UT_RUN_UNIT_TEST( anonymous_timers )
}

