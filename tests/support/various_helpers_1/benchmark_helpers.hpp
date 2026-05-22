#pragma once

#include <chrono>
#include <iostream>
#include <string>

class benchmarker_t
{
public:
	void
	start()
	{
		m_started_at = clock_t::now();
	}

	template< typename Count >
	void
	finish_and_show_stats(
		Count count,
		const char * unit_name )
	{
		show_stats( static_cast< double >( count ), unit_name );
	}

	template< typename Count >
	void
	finish_and_show_stats(
		Count count,
		const std::string & unit_name )
	{
		show_stats( static_cast< double >( count ), unit_name.c_str() );
	}

private:
	using clock_t = std::chrono::steady_clock;

	void
	show_stats(
		double count,
		const char * unit_name )
	{
		const auto finished_at = clock_t::now();
		const auto elapsed = std::chrono::duration_cast<
			std::chrono::duration< double > >( finished_at - m_started_at );
		const auto seconds = elapsed.count();

		std::cout << "Elapsed: " << seconds << "s";
		if( 0.0 < seconds )
			std::cout << ", " << count / seconds << ' ' << unit_name << "/s";
		std::cout << std::endl;
	}

	clock_t::time_point m_started_at{};
};

