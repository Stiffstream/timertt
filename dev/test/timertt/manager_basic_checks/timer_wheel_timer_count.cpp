#include <timertt/all.hpp>

#include <utest_helper_1/h/helper.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

typedef timertt::timer_wheel_manager_template<
		timertt::thread_safety::unsafe,
		timertt::default_error_logger,
		timertt::default_actor_exception_handler >
	timer_manager_t;

using namespace std::chrono;

UT_UNIT_TEST( no_timers )
{
	timer_manager_t tm;

	UT_CHECK_EQ( false, std::get<0>( tm.nearest_time_point() ) );
}

UT_UNIT_TEST( single_shot )
{
	run_with_time_limit(
		[]()
		{
			timer_manager_t tm;

			std::string v;

			tm.activate( tm.allocate(),
					milliseconds( 20 ), [&v]() { v = "hello"; } );

			UT_CHECK_EQ( true, std::get<0>( tm.nearest_time_point() ) );

			std::this_thread::sleep_for( milliseconds( 100 ) );

			tm.process_expired_timers();

			UT_CHECK_EQ( v, "hello" );
			UT_CHECK_EQ( false, std::get<0>( tm.nearest_time_point() ) );
		},
		1,
		"single_shot" );
}

UT_UNIT_TEST( single_periodic )
{
	run_with_time_limit(
		[]()
		{
			timer_manager_t tm;

			std::string v;

			tm.activate( tm.allocate(),
					milliseconds( 80 ),
					milliseconds( 200 ),
					[&v] { v = "hello"; } );

			UT_CHECK_EQ( true, std::get<0>( tm.nearest_time_point() ) );

			std::this_thread::sleep_for( milliseconds( 100 ) );

			tm.process_expired_timers();

			UT_CHECK_EQ( v, "hello" );
			UT_CHECK_EQ( true, std::get<0>( tm.nearest_time_point() ) );
		},
		1,
		"single_periodic" );
}

UT_UNIT_TEST( activate_deactivate_without_processing )
{
	run_with_time_limit(
		[]()
		{
			timer_manager_t tm;

			auto id = tm.allocate();
			tm.activate( id, milliseconds( 80 ), milliseconds( 200 ),
					[] {} );

			UT_CHECK_EQ( true, std::get<0>( tm.nearest_time_point() ) );

			tm.deactivate( id );

			UT_CHECK_EQ( false, std::get<0>( tm.nearest_time_point() ) );
		},
		1,
		"activate_deactivate_without_processing" );
}

UT_UNIT_TEST( activate_deactivate_inside_processing )
{
	run_with_time_limit(
		[]()
		{
			timer_manager_t tm;

			bool processed = false;
			auto id = tm.allocate();
			tm.activate( id, milliseconds( 80 ), milliseconds( 200 ),
					[id, &tm, &processed] {
						processed = true;
						tm.deactivate( id );
					} );

			auto r = tm.nearest_time_point();
			UT_CHECK_EQ( true, std::get<0>( r ) );

			do
			{
				std::this_thread::sleep_until( std::get<1>( r ) );
				tm.process_expired_timers();
				r = tm.nearest_time_point();
			} while( !processed );

			UT_CHECK_EQ( false, std::get<0>( r ) );
		},
		1,
		"activate_deactivate_inside_processing" );
}

UT_UNIT_TEST( reset_test )
{
	run_with_time_limit(
		[]()
		{
			timer_manager_t tm;

			tm.activate( milliseconds( 10 ), []{} );
			tm.activate( milliseconds( 100 ), milliseconds( 20 ), []{} );
			tm.activate( milliseconds( 1000 ), milliseconds( 20 ), []{} );

			UT_CHECK_EQ( true, std::get<0>( tm.nearest_time_point() ) );

			tm.reset();

			UT_CHECK_EQ( false, std::get<0>( tm.nearest_time_point() ) );
		},
		1,
		"reset_test" );
}

int main()
{
	UT_RUN_UNIT_TEST( no_timers )
	UT_RUN_UNIT_TEST( single_shot )
	UT_RUN_UNIT_TEST( single_periodic )
	UT_RUN_UNIT_TEST( activate_deactivate_without_processing )
	UT_RUN_UNIT_TEST( activate_deactivate_inside_processing )
	UT_RUN_UNIT_TEST( reset_test )
}

