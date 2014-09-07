#include <iostream>
#include <cstring>
#include <cstdlib>
#include <random>
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
};

void
show_usage()
{
	std::cout <<
		"_test.timertt.schedule_erase_benchmark <args>\n\n"
		"Arguments:\n"
		"-h            show help\n"
		"-d <demands>  set demands count\n"
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
		else
			throw std::runtime_error( "unknown argument: " +
					std::string( *current ) );

		++current;
	}

	if( !result.m_demand_count )
		result.m_demand_count = 100000;

	return result;
}

std::vector< milliseconds >
create_durations( const cfg_t & cfg )
{
	std::cout << "Generating pauses..." << std::endl;

	std::vector< milliseconds > result;
	result.reserve( cfg.m_demand_count );

	milliseconds value{ 100000 };
	for( unsigned int i = 0; i != cfg.m_demand_count; ++i )
	{
		result.push_back( value + milliseconds{ (i+1) * 15 } );
	}

	std::shuffle( result.begin(), result.end(),
			std::mt19937( std::random_device()() ) );

	return result;
}

void do_nothing()
{
}

std::vector< timertt::timer_holder_t >
create_timers(
	const cfg_t & cfg,
	timer_thread_t & tt,
	timertt::timer_action_t actor )
{
	std::vector< milliseconds > durations = create_durations( cfg );

	std::vector< timertt::timer_holder_t > result;
	result.reserve( cfg.m_demand_count );

	std::cout << "Scheduling demands..." << std::endl;
	benchmarker_t benchmarker;
	benchmarker.start();

	for( auto pause : durations )
	{
		auto timer = tt.allocate();
		result.push_back( timer );
		tt.activate( timer, pause, actor );
	}

	benchmarker.finish_and_show_stats(
			cfg.m_demand_count,
			"demands" );

	return result;
}

void
erase_demands( timer_thread_t & tt,
	std::vector< timertt::timer_holder_t > & timer_ids )
{
	std::cout << "Shuffling timer ids..." << std::endl;
	std::shuffle( timer_ids.begin(), timer_ids.end(),
			std::mt19937( std::random_device()() ) );

	std::cout << "Erasing demands..." << std::endl;
	benchmarker_t benchmarker;
	benchmarker.start();

	for( auto & id : timer_ids )
	{
		tt.deactivate( id );
	}

	benchmarker.finish_and_show_stats(
			timer_ids.size(),
			"demands" );
}

int main( int argc, char ** argv )
{
	try
	{
		cfg_t cfg = parse_args( argc, argv );

		timer_thread_t tt;
		tt.start();

		auto timer_ids = create_timers( cfg, tt,
				timertt::timer_action_t( &do_nothing ) );

		erase_demands( tt, timer_ids );
	}
	catch( const std::exception & x )
	{
		std::cerr << "*** Exception: " << x.what() << std::endl;
		return 2;
	}

	return 0;
}

