#pragma once

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

template< typename Callable >
void
run_with_time_limit(
	Callable callable,
	unsigned int seconds,
	const char * name )
{
	const auto started_at = std::chrono::steady_clock::now();

	callable();

	const auto finished_at = std::chrono::steady_clock::now();
	if( std::chrono::seconds{ seconds } < finished_at - started_at )
	{
		std::cerr << "time limit exceeded in " << name
			<< " after " << seconds << " second(s)" << std::endl;
		throw std::runtime_error( "time limit exceeded" );
	}
}

template< typename Callable >
void
run_with_time_limit(
	Callable callable,
	unsigned int seconds,
	const std::string & name )
{
	run_with_time_limit( callable, seconds, name.c_str() );
}
