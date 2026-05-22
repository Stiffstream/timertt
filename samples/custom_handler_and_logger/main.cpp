/*
 * Timer Thread Template sample.
 */

#include <iostream>
#include <cstdlib>

#include <timertt/all.hpp>

using namespace std;
using namespace std::chrono;
using namespace std::placeholders;
using namespace timertt;

// Custom error logger.
void log_error( ostream * to, const string & what )
{
	(*to) << "ERROR(from timer thread): " << what << endl;
}

int main()
{
	// Custom exception handler.
	auto handler = []( const exception & x ) {
		cout << "An exception from timer action: " << x.what() << endl;
	};

	// Custom timer thread.
	timer_list_thread_template<
					void (*)(), // Type of timer action.
					function< void(const string &) >, // Type of logger.
					decltype(handler) > // Type of exception handler.
			tt{ bind( log_error, &cout, _1 ), handler };

	// Timer thread must be started before activation of timers.
	tt.start();

	// Timer without exceptions.
	tt.activate( milliseconds( 20 ),
			[]() { cout << "Simple one-shot" << endl; } );

	// Timer with standard exception.
	tt.activate( milliseconds( 21 ),
			[]() { throw runtime_error( "STD Exception" ); } );

	// Timer with non-standard exception.
	tt.activate( milliseconds( 22 ),
			[]() { throw 42; } );

	// Wait for some time.
	this_thread::sleep_for( milliseconds( 200 ) );

	// Finish the timer thread.
	tt.shutdown_and_join();
}

