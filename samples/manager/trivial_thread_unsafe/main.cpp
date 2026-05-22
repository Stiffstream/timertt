/*
 * Timer Thread Template sample.
 */

#include <iostream>
#include <cstdlib>

#include <timertt/all.hpp>

using namespace std;
using namespace std::chrono;
using namespace timertt;

int main()
{
	timer_list_manager_template< thread_safety::unsafe > tm;

	// The simple single-shot timer.
	tm.activate( milliseconds( 20 ),
			[] { cout << "Simple one-shot" << endl; } );

	// The simple periodic timer.
	// Will work until sample finished.
	tm.activate( milliseconds( 20 ), milliseconds( 20 ),
			[] {
				static int i = 0;
				cout << "Simple periodic (" << i << ")" << endl;
				++i;
			} );

	// Allocation of timer and explicit activation.
	auto id1 = tm.allocate();
	tm.activate( id1, milliseconds( 30 ),
			[]() {
				cout << "Preallocated single-shot timer" << endl;
			} );

	// Periodic timer with timer preallocation, explicit activation
	// and deactivation from the timer action.
	auto id2 = tm.allocate();
	tm.activate( id2, milliseconds( 40 ), milliseconds( 15 ),
			[id2, &tm]() {
				static int i = 0;
				cout << "Preallocated periodic (" << i << ")" << endl;
				++i;
				if( i > 2 )
					tm.deactivate( id2 );
			} );

	// Single-shot timer with explicit activation and deactivation
	// before timer event.
	auto id3 = tm.allocate();
	tm.activate( id3, milliseconds( 50 ),
			[]() {
				cerr << "This timer must not be called!" << endl;
				std::abort();
			} );
	tm.deactivate( id3 );

	// Do timers actions for some time.
	const auto stop_point = system_clock::now() + milliseconds( 200 );
	while( stop_point > system_clock::now() )
	{
		tm.process_expired_timers();

		bool timers_exist = false;
		timertt::monotonic_clock::time_point wake_at;
		tie( timers_exist, wake_at ) = tm.nearest_time_point();
		if( !timers_exist )
		{
			// This is imposible because there must be periodic timer.
			cerr << "Unexpected error: timer_manager has lost periodic timer"
					<< std::endl;
			std::abort();
		}
		else
			std::this_thread::sleep_until( wake_at );
	}
}

