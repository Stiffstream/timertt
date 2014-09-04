/*
 * TimerThreadTemplate
 */

/*!
 * \file
 * \brief All project's stuff.
 */

#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>

namespace timertt
{

struct default_error_logger
{
	inline void
	operator()( const std::string & what )
	{
		std::cerr << what << std::endl;
	}
};

template< typename TIMER_ID > 
struct default_actor_exception_handler
{
	inline void
	operator()( TIMER_ID id, const std::exception & x )
	{
		std::abort();
	}
};

template< typename ACTOR,
	typename ERROR_LOGGER = default_error_logger,
	typename ACTOR_EXCEPTION_HANDLER =
			default_actor_exception_handler< std::uint_fast64_t > >
class timer_thread_template_t
{
private :
	typedef std::uint_fast64_t counter_t;
	typedef std::chrono::steady_clock clock_type_t;

	struct demand_key_t
	{
		clock_type_t::time_point m_when;
		counter_t m_id;

		bool operator<( const demand_key_t & o ) const
		{
			return m_when < o.m_when ||
					( m_when == o.m_when && m_id < o.m_id );
		}
	};

	struct demand_data_t
	{
		clock_type_t::duration m_period;
		ACTOR m_actor;
	};

	typedef std::map< demand_key_t, demand_data_t > demands_t;
	typedef std::unordered_map< counter_t, clock_type_t::time_point > id_map_t;

public :
	typedef typename id_map_t::key_type id_t;
	typedef ACTOR actor_t;

	//! Default constructor.
	timer_thread_template_t()
	{}

	//! Initializing constructor.
	/*!
	 * Allows to specify error_logger and actor_exception_handler.
	 */
	timer_thread_template_t(
		ERROR_LOGGER error_logger,
		ACTOR_EXCEPTION_HANDLER actor_exception_handler )
		:	m_error_logger( error_logger )
		,	m_exception_handler( actor_exception_handler )
	{}

	//! Desctructor.
	/*!
	 * Stopes timer thread if it is running.
	 */
	~timer_thread_template_t()
	{
		stop();
	}

	void
	start()
	{
		std::lock_guard< std::mutex > lock( m_lock );

		if( m_thread )
			throw std::runtime_error( "timer_thread is already started" );

		m_thread.reset(
				new std::thread(
						std::bind( &timer_thread_template_t::body, this ) ) );
	}

	void
	stop()
	{
		std::thread * t = nullptr;
		{
			std::lock_guard< std::mutex > lock( m_lock );

			if( m_thread )
			{
				m_shutdown = true;
				t = m_thread.release();
				m_condition.notify_one();
			}
		}
		if( t )
		{
			std::unique_ptr< std::thread > destroyer{ t };
			destroyer->join();
		}
	}

	template< typename DURATION_1 >
	id_t
	schedule( DURATION_1 pause, ACTOR actor )
	{
		return this->schedule( pause, zero, actor );
	}

	template< typename DURATION_1, typename DURATION_2 >
	id_t
	schedule( DURATION_1 pause, DURATION_2 period, ACTOR actor )
	{
		std::lock_guard< std::mutex > lock( m_lock );

		if( !m_thread )
			throw std::runtime_error( "timer_thread is not started" );

		auto id = ++m_ordinals;

		auto when = clock_type_t::now() + pause;

		m_ids.emplace( id, when );

		bool need_notify = m_demands.empty() ||
				(when < m_demands.begin()->first.m_when);

		try
		{
			m_demands.emplace(
					demand_key_t{ when, id },
					demand_data_t{ period, actor } );
		}
		catch( ... )
		{
			m_ids.erase( id );
			throw;
		}

		if( need_notify )
			m_condition.notify_one();

		return id;
	}

	void
	erase( id_t id )
	{
		std::lock_guard< std::mutex > lock( m_lock );

		auto it_id = m_ids.find( id );
		if( it_id != m_ids.end() )
		{
			m_demands.erase( demand_key_t{ it_id->second, id } );
			m_ids.erase( it_id );
		}
	}

private :
	std::mutex m_lock;
	bool m_shutdown = false;

	demands_t m_demands;
	id_map_t m_ids;

	counter_t m_ordinals = 0;

	std::condition_variable m_condition;

	std::unique_ptr< std::thread > m_thread;

	const clock_type_t::duration zero{ 0 };

	ERROR_LOGGER m_error_logger;
	ACTOR_EXCEPTION_HANDLER m_exception_handler;

	void
	body()
	{
		while( true )
		{
			std::unique_lock< std::mutex > lock( m_lock );

			if( m_shutdown )
			{
				break;
				clear_all();
			}

			if( m_demands.empty() )
				// Wait for the first demand or for the shutdown.
				m_condition.wait( lock );
			else
			{
				auto front = m_demands.begin();
				auto now = clock_type_t::now();

				if( now >= front->first.m_when )
				{
					// It is necessary to process the first demand.
					demand_key_t key = front->first;
					demand_data_t data = front->second;

					m_demands.erase( front );

					if( data.m_period != zero )
					{
						// It is periodic demand. It must be resheduled.
						auto it_id = m_ids.find( key.m_id );
						// This iterator must be valid.
						ensure_id_iterator_valid( it_id, key.m_id );

						key.m_when += data.m_period;
						it_id->second = key.m_when;

						reshedule_periodic_demand( key, data );
					}

					lock.unlock();

					execute_demand_action( key, data );

					lock.lock();
				}
				else
				{
					auto when = front->first.m_when;
					m_condition.wait_until( lock, when );
				}
			}
		}
	}

	inline void
	ensure_id_iterator_valid( typename id_map_t::iterator & it, id_t & id )
	{
		if( it == m_ids.end() )
		{
			std::ostringstream ss;
			ss << __FILE__ << "(" << __LINE__
				<< "): unable to find timer_id for periodic demand; id="
				<< id;
			m_error_logger( ss.str() );
			std::abort();
		}
	}

	inline void
	reshedule_periodic_demand(
		demand_key_t & key,
		demand_data_t & data )
	{
		// Any exception must be treated as a fatal error.
		try
		{
			m_demands.emplace( key, data );
		}
		catch( const std::exception & x )
		{
			std::ostringstream ss;
			ss << __FILE__ << "(" << __LINE__ 
				<< "): reschedule of periodic demand failed; id="
				<< key.m_id
				<< ", failure: " << x.what();
			m_error_logger( ss.str() );
			std::abort();
		}
		catch( ... )
		{
			std::ostringstream ss;
			ss << __FILE__ << "(" << __LINE__ 
				<< "): reschedule of periodic demand failed; id="
				<< key.m_id
				<< ", failure type unknown";
			m_error_logger( ss.str() );
			std::abort();
		}
	}

	inline void
	execute_demand_action(
		demand_key_t & key,
		demand_data_t & data )
	{
		try
		{
			data.m_actor();
		}
		catch( const std::exception & x )
		{
			m_exception_handler( key.m_id, x );
		}
		catch( ... )
		{
			std::ostringstream ss;
			ss << __FILE__ << "(" << __LINE__ 
				<< "): an unknown exception from actor; id="
				<< key.m_id;
			m_error_logger( ss.str() );
			std::abort();
		}
	}

	inline void
	clear_all()
	{
		demands_t demands; m_demands.swap( demands );
		id_map_t ids; m_ids.swap( ids );
	}
};

using timer_thread_t = timer_thread_template_t< std::function< void() > >;

} /* namespace timertt */

