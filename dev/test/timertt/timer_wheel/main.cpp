#include <timertt/all.hpp>

#include <utest_helper_1/h/helper.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

using namespace std::chrono;

using timer_thread_t = timertt::timer_wheel_thread_template<
	timertt::default_timer_action_type,
	timertt::default_error_logger,
	timertt::default_actor_exception_handler >;

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

int main()
{
	UT_RUN_UNIT_TEST( several_full_rolls )
}

