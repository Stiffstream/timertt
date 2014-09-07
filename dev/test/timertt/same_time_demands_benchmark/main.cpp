#include <iostream>
#include <cstring>
#include <cstdlib>
#include <algorithm>

#include <timertt/all.hpp>

#include "../benchmark_helpers.hpp"

using namespace std::chrono;

typedef timertt::timer_wheel_thread_t<
		timertt::default_error_logger,
		timertt::default_actor_exception_handler > timer_thread_t;

struct cfg_t
{
	unsigned int m_demand_count = 0;
	bool m_periodic_demands = false;
};

void
show_usage()
{
	std::cout <<
		"_test.timertt.same_time_demands_benchmark <args>\n\n"
		"Arguments:\n"
		"-h            show help\n"
		"-d <demands>  set demands count\n"
		"-p            use periodic demands\n"
		<< std::flush;
}

cfg_t
parse_args( int argc, char ** argv )
{
	cfg_t result;

	char ** current = &argv[ 1 ];
	char ** last = &argv[ argc ];

	while( current != last )
	{
		if( 0 == std::strcmp( "-h", *current ) )
		{
			show_usage();
			std::exit(0);
		}
		else if( 0 == std::strcmp( "-d", *current ) )
		{
			++current;
			if( current != last )
				result.m_demand_count = static_cast< unsigned int >(
						std::atoi( *current ) );
			else
				throw std::runtime_error( "-d requires argument" );
		}
		else if( 0 == std::strcmp( "-p", *current ) )
			result.m_periodic_demands = true;
		else
			throw std::runtime_error( "unknown argument: " +
					std::string( *current ) );

		++current;
	}

	if( !result.m_demand_count )
		result.m_demand_count = 10000;

	return result;
}

void
do_benchmark( const cfg_t cfg )
{
	timer_thread_t tt;
	tt.start();

	const auto pause = milliseconds{ 250 }; 
	const auto period = seconds{ cfg.m_periodic_demands ? 25 : 0 };

	std::mutex mutex;
	std::condition_variable condition;

	std::unique_lock< std::mutex > lock( mutex );

	unsigned int counter = 0;

	benchmarker_t benchmarker;

	timertt::timer_action_t first = [&]() {
			std::lock_guard< std::mutex > l( mutex );
			benchmarker.start();
			++counter;
		};
	timertt::timer_action_t common = [&counter]() {
			++counter;
		};
	timertt::timer_action_t last = [&]() {
			++counter;
			benchmarker.finish_and_show_stats( counter, "invocations" );

			std::lock_guard< std::mutex > l( mutex );
			condition.notify_one();
		};

	unsigned int demands = cfg.m_demand_count;
	if( demands < 3 )
		demands = 1;
	else
		demands -= 2;

	tt.activate( pause, first );
	for( unsigned int i = 0; i != demands; ++i )
		tt.activate( pause, period, common );
	tt.activate( pause, last );

	condition.wait( lock );
}

int main( int argc, char ** argv )
{
	try
	{
		cfg_t cfg = parse_args( argc, argv );

		do_benchmark( cfg );
	}
	catch( const std::exception & x )
	{
		std::cerr << "*** Exception: " << x.what() << std::endl;
		return 2;
	}

	return 0;
}

