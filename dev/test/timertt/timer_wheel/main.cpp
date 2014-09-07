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
			tt.stop();

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

			tt.stop();

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

			tt.stop();

			UT_CHECK_EQ( events, 4 );
		},
		1,
		"demands_deletion_during_processing" );
}
int main()
{
	UT_RUN_UNIT_TEST( several_full_rolls )
	UT_RUN_UNIT_TEST( demands_cleanup_on_shutdown )
	UT_RUN_UNIT_TEST( demands_deletion_during_processing )
}
