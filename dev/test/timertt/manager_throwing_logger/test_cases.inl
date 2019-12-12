#include <iostream>

#include <timertt/all.hpp>

#include <utest_helper_1/h/helper.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

using namespace std::chrono;

UT_UNIT_TEST( single_shot )
{
	run_with_time_limit(
		[]()
		{
			timer_manager_t tt;

			tt.activate( tt.allocate(),
					milliseconds( 20 ), []() { throw 1; } );

			std::this_thread::sleep_for( milliseconds( 100 ) );

			tt.process_expired_timers();
		},
		1,
		"single_shot" );
}

int main()
{
	UT_RUN_UNIT_TEST( single_shot )
}

