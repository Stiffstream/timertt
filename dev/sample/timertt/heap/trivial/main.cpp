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
	timer_heap_thread_t tt;

	// Timer thread must be started before activation of timers.
	tt.start();

	// The simple single-shot timer.
	tt.activate( milliseconds( 20 ),
			[]() { cout << "Simple one-shot" << endl; } );

	// The simple periodic timer.
	// Will work until timer thread finished.
	tt.activate( milliseconds( 20 ), milliseconds( 20 ),
			[]() {
				static int i = 0;
				cout << "Simple periodic (" << i << ")" << endl;
				++i;
			} );

	// Allocation of timer and explicit activation.
	auto id1 = tt.allocate();
	tt.activate( id1, milliseconds( 30 ),
			[]() {
				cout << "Preallocated single-shot timer" << endl;
			} );

	// Periodic timer with timer preallocation, explicit activation
	// and deactivation from the timer action.
	auto id2 = tt.allocate();
	tt.activate( id2, milliseconds( 40 ), milliseconds( 15 ),
			[id2, &tt]() {
				static int i = 0;
				cout << "Preallocated periodic (" << i << ")" << endl;
				++i;
				if( i > 2 )
					tt.deactivate( id2 );
			} );

	// Single-shot timer with explicit activation and deactivation
	// before timer event.
	auto id3 = tt.allocate();
	tt.activate( id3, milliseconds( 50 ),
			[]() {
				cerr << "This timer must not be called!" << endl;
				std::abort();
			} );
	tt.deactivate( id3 );

	// Wait for some time.
	this_thread::sleep_for( milliseconds( 200 ) );

	// Finish the timer thread.
	tt.shutdown_and_join();
}

