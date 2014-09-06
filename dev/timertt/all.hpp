/*
 * TimerThreadTemplate
 */

/*!
 * \file
 * \brief All project's stuff.
 */

#pragma once

#include <atomic>
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

/*!
 * \brief Base type for timer demands.
 */
struct timer_t
{
	//! Reference counter for the demand.
	std::atomic_uint m_references;

	inline timer_t()
	{
		m_references = 0;
	}

	inline virtual ~timer_t()
	{}

	//! Increment reference counter for the demand.
	static inline void
	increment_references( timer_t * t )
	{
		++(t->m_references);
	}

	//! Decrement reference counter for the demand and destroy
	//! demand if there is no more references.
	static inline void
	decrement_references( timer_t * t )
	{
		if( 0 == --(t->m_references) )
			delete t;
	}
};

/*!
 * \brief An intrusive smart pointer to timer demand.
 */
class timer_holder_t
{
public :
	//! Default constructor.
	/*!
	 * Constructs a null pointer.
	 */
	inline timer_holder_t()
		:	m_timer( nullptr )
	{}
	//! Constructor for a raw pointer.
	inline timer_holder_t( timer_t * t )
		:	m_timer( t )
	{
		take_object();
	}
	//! Copy constructor.
	inline timer_holder_t( const timer_holder_t & o )
		:	m_timer( o.m_timer )
	{
		take_object();
	}
	//! Move constructor.
	inline timer_holder_t( timer_holder_t && o )
		:	m_timer( o.m_timer )
	{
		o.m_timer = nullptr;
	}

	//! Destructor.
	inline ~timer_holder_t()
	{
		dismiss_object();
	}

	//! Copy operator.
	inline timer_holder_t &
	operator=( const timer_holder_t & o )
	{
		timer_holder_t t( o );
		swap( t );
		return *this;
	}

	//! Move operator.
	inline timer_holder_t &
	operator=( timer_holder_t && o )
	{
		timer_holder_t t( std::move( o ) );
		swap( t );
		return *this;
	}

	//! Swap values.
	inline void
	swap( timer_holder_t & o )
	{
		timer_t * t = m_timer;
		m_timer = o.m_timer;
		o.m_timer = t;
	}

	/*!
	 * \brief Drop controlled object.
	 */
	inline void
	reset()
	{
		dismiss_object();
	}

	//! Is this a null pointer?
	/*!
		i.e. whether get() != 0.

		\retval true if *this manages an object. 
		\retval false otherwise.
	*/
	inline operator bool() const 
	{
		return nullptr != m_timer;
	}

	/*!
	 * \name Access to object.
	 * \{
	 */
	inline timer_t *
	get() const
	{
		return m_timer;
	}
	/*!
	 * \}
	 */

private :
	//! Timer controlled by a smart pointer.
	timer_t * m_timer;

	//! Increment reference count to object if it's not null.
	inline void
	take_object()
	{
		if( m_timer )
			timer_t::increment_references( m_timer );
	}

	//! Decrement reference count to object and delete it if needed.
	inline void
	dismiss_object()
	{
		if( m_timer )
		{
			timer_t::decrement_references( m_timer );
			m_timer = nullptr;
		}
	}
};

struct default_error_logger
{
	inline void
	operator()( const std::string & what )
	{
		std::cerr << what << std::endl;
	}
};

struct default_actor_exception_handler
{
	inline void
	operator()( const std::exception & x )
	{
		std::abort();
	}
};

/*!
 * \brief Type of timer action.
 */
typedef std::function< void() > timer_action_t;

/*!
 * \brief Type of clock used by all threads.
 */
typedef std::chrono::steady_clock monotonic_clock_t;

/*!
 * \brief A timer wheel thread template.
 */
template<
	typename ERROR_LOGGER,
	typename ACTOR_EXCEPTION_HANDLER >
class timer_wheel_thread_t
{
	timer_wheel_thread_t( const timer_wheel_thread_t & ) = delete;
	timer_wheel_thread_t &
	operator=( const timer_wheel_thread_t & ) = delete;

public :
	//! Default wheel size.
	static unsigned int
	default_wheel_size() { return 1000; }

	//! Default tick duration.
	static monotonic_clock_t::duration
	default_granularity() { return std::chrono::milliseconds( 10 ); }

	//! Default constructor.
	timer_wheel_thread_t()
		:	timer_wheel_thread_t(
				default_wheel_size(),
				default_granularity(),
				ERROR_LOGGER(),
				ACTOR_EXCEPTION_HANDLER() )
	{}

	//! Constructor with wheel size and granularity parameters.
	timer_wheel_thread_t(
		unsigned int wheel_size,
		monotonic_clock_t::duration granularity )
		:	timer_wheel_thread_t(
				wheel_size,
				granularity,
				ERROR_LOGGER(),
				ACTOR_EXCEPTION_HANDLER() )
	{
	}

	//! Constructor with all parameters.
	timer_wheel_thread_t(
		unsigned int wheel_size,
		monotonic_clock_t::duration granularity,
		ERROR_LOGGER error_logger,
		ACTOR_EXCEPTION_HANDLER exception_handler )
		:	m_wheel_size( wheel_size )
		,	m_granularity( granularity )
		,	m_error_logger( error_logger )
		,	m_exception_handler( exception_handler )
	{
		m_wheel.resize( wheel_size );
	}

	//! Destructor.
	~timer_wheel_thread_t()
	{
		stop();
	}

	//! Start timer thread.
	/*!
	 * \throw std::runtime_error if thread is already started.
	 */
	void
	start()
	{
		std::lock_guard< std::mutex > lock( m_lock );

		if( m_thread )
			throw std::runtime_error( "timer_wheel_thread is already started" );

		m_thread.reset(
				new std::thread(
						std::bind( &timer_wheel_thread_t::body, this ) ) );
	}

	//! Finish timer thread and wait for completion.
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

	//! Create timer to be activated later.
	timer_holder_t
	allocate()
	{
		return timer_holder_t( new wheel_timer_t() );
	}

	//! Activate timer and schedule it for execution.
	template< class DURATION_1 >
	void
	activate(
		//! Timer to be activated.
		timer_holder_t timer,
		//! Pause for timer execution.
		DURATION_1 pause,
		//! Action for the timer.
		timer_action_t action )
	{
		activate(
				std::move( timer ),
				pause,
				monotonic_clock_t::duration::zero(),
				std::move( action ) );
	}

	//! Activate timer and schedule it for execution.
	template< class DURATION_1, class DURATION_2 >
	void
	activate(
		//! Timer to be activated.
		timer_holder_t timer,
		//! Pause for timer execution.
		DURATION_1 pause,
		//! Repetition period.
		DURATION_2 period,
		//! Action for the timer.
		timer_action_t action )
	{
		std::lock_guard< std::mutex > lock( m_lock );

		if( !m_thread )
			throw std::runtime_error( "timer_thread is not started" );

		wheel_timer_t * wheel_timer = cast_timer_pointer( timer.get() );
		ensure_timer_not_activated( wheel_timer );

		wheel_timer->m_action = std::move(action);

		// Calculate the demand position in the wheel.
		set_position_in_the_wheel(
				wheel_timer,
				duration_to_ticks( pause ) );

		// Special calculations for the periodic demand.
		if( monotonic_clock_t::duration::zero() != period )
			wheel_timer->m_period = duration_to_ticks( period );
		else
			wheel_timer->m_period = 0;

		insert_demand_to_wheel( wheel_timer );
	}

	//! Deactivate timer and remove it from the wheel.
	void
	deactivate( timer_holder_t timer )
	{
		if( !timer )
			return;

		std::lock_guard< std::mutex > lock( m_lock );

		wheel_timer_t * wheel_timer = cast_timer_pointer( timer.get() );
		if( is_active_timer( wheel_timer ) )
		{
			remove_timer_from_wheel( wheel_timer );

			deactivate_timer( wheel_timer );

			// Release timer object.
			timer_t::decrement_references( wheel_timer );
		}
	}

private :
	//! Type of wheel timer.
	struct wheel_timer_t : public timer_t
	{
		//! Special value as indicator of inactive timer.
		static const unsigned int inactive = static_cast< unsigned int >( -1 );

		//! Position in the wheel.
		unsigned int m_position = inactive;
		//! Full rolls of wheel before execution of demand.
		unsigned int m_full_rolls_left = 0;

		//! Period in ticks.
		/*!
		 * Zero means that demand is single shot.
		 */
		unsigned int m_period = 0;

		//! Timer action.
		timer_action_t m_action;

		//! Previous demand in the list.
		wheel_timer_t * m_prev = nullptr;
		//! Next demand in the list.
		wheel_timer_t * m_next = nullptr;
	};

	//! Type of wheel's item.
	struct wheel_item_t
	{
		//! Head of the demand's list.
		wheel_timer_t * m_head = nullptr;
		//! Tail of the demand's list.
		wheel_timer_t * m_tail = nullptr;
	};

	/*!
	 * \name Object's attributes.
	 * \{
	 */
	//! Shutdown flag.
	bool m_shutdown = false;

	//! Object's lock.
	std::mutex m_lock;

	//! Condition variable for sleeping on the timer.
	std::condition_variable m_condition;

	//! Thread object.
	/*!
	 * Created in start() method.
	 */
	std::unique_ptr< std::thread > m_thread;

	//! Size of the wheel.
	const unsigned int m_wheel_size;

	//! Granularity of one time step.
	const monotonic_clock_t::duration m_granularity;

	//! Index of the current position in the wheel.
	unsigned int m_current_position = 0;

	//! The wheel data.
	std::vector< wheel_item_t > m_wheel;

	//! Error logger.
	ERROR_LOGGER m_error_logger;

	//! Exception handler.
	ACTOR_EXCEPTION_HANDLER m_exception_handler;
	/*!
	 * \}
	 */

	static wheel_timer_t *
	cast_timer_pointer( timer_t * timer )
	{
		if( !timer )
			throw std::runtime_error( "timer is nullptr" );

		return static_cast< wheel_timer_t * >(timer);
	}

	static bool
	is_active_timer( const wheel_timer_t * timer )
	{
		return wheel_timer_t::inactive != timer->m_position;
	}

	static void
	ensure_timer_not_activated( const wheel_timer_t * timer )
	{
		if( is_active_timer( timer ) )
			throw std::runtime_error( "timer is already activated" );
	}

	static void
	deactivate_timer( wheel_timer_t * timer )
	{
		timer->m_position = wheel_timer_t::inactive;
	}

	template< class DURATION >
	unsigned int
	duration_to_ticks( DURATION d ) const
	{
		auto d_units = 
				std::chrono::duration_cast< monotonic_clock_t::duration >( d )
				.count();
		auto g_units = m_granularity.count();

		unsigned int r = static_cast< unsigned int >( d_units / g_units );
		if( !r )
			r = 1;
		return r;
	}

	void
	set_position_in_the_wheel(
		wheel_timer_t * wheel_timer,
		unsigned int pause_in_ticks ) const
	{
		wheel_timer->m_position =
				( m_current_position + pause_in_ticks ) % m_wheel_size;
		wheel_timer->m_full_rolls_left = pause_in_ticks / m_wheel_size;
	}

	void
	insert_demand_to_wheel( wheel_timer_t * wheel_timer )
	{
		// Always increment reference count on this point.
		timer_t::increment_references( wheel_timer );

		wheel_item_t & item = m_wheel[ wheel_timer->m_position ];
		if( item.m_head )
		{
			// There is a list of demands for the wheel position.
			// New demand must be added to the end of that list.
			wheel_timer->m_prev = item.m_tail;
			item.m_tail = wheel_timer;
		}
		else
		{
			// There is no list of demands for this wheel position yet.
			// New list must be started.
			item.m_head = wheel_timer;
			item.m_tail = wheel_timer;
		}
	}

	void
	remove_timer_from_wheel( wheel_timer_t * wheel_timer )
	{
		if( wheel_timer->m_prev )
			wheel_timer->m_prev->m_next = wheel_timer->m_next;
		else
			m_wheel[ wheel_timer->m_position ].m_head = wheel_timer->m_next;

		if( wheel_timer->m_next )
			wheel_timer->m_next->m_prev = wheel_timer->m_prev;
		else
			m_wheel[ wheel_timer->m_position ].m_tail = wheel_timer->m_prev;

		wheel_timer->m_prev = nullptr;
		wheel_timer->m_next = nullptr;
	}

	//! Thread body.
	void
	body()
	{
		std::unique_lock< std::mutex > lock( m_lock );

		auto tick_start_time = monotonic_clock_t::now();

		while( !m_shutdown )
		{
			process_current_wheel_position( lock );

			// Wait for next time step.
			auto next_tick_time = tick_start_time + m_granularity;
			while( !m_shutdown && next_tick_time > monotonic_clock_t::now() )
			{
				m_condition.wait_until( lock, next_tick_time );
			}

			if( !m_shutdown )
			{
				// Switch to next wheel position on the start
				// of new time step.
				tick_start_time = next_tick_time;
				m_current_position += 1;
				if( m_current_position > m_wheel_size )
					m_current_position = 0;
			}
		}

		clear_all();
	}

	void
	process_current_wheel_position(
		std::unique_lock< std::mutex > & lock )
	{
		wheel_timer_t * timer = m_wheel[ m_current_position ].m_head;
		while( timer && !m_shutdown )
		{
			if( timer->m_full_rolls_left )
			{
				timer->m_full_rolls_left -= 1;
				timer = timer->m_next;
			}
			else
			{
				wheel_timer_t * t = timer;
				timer = timer->m_next;

				remove_timer_from_wheel( t );
				if( t->m_period )
					reschedule_periodic_timer( t );
				else
					deactivate_timer( t );

				execute_demand( lock, t );

				// Always decrement reference count on this point.
				// If this is preriodic timer the reference is
				// already incremented in reschedule_periodic_timer().
				timer_t::decrement_references( t );
			}
		}
	}

	void
	reschedule_periodic_timer( wheel_timer_t * timer )
	{
		set_position_in_the_wheel( timer, timer->m_period );

		// Reference count will be incremented during inserting
		// to the wheel.
		insert_demand_to_wheel( timer );
	}

	void
	execute_demand(
		std::unique_lock< std::mutex > & lock,
		wheel_timer_t * timer )
	{
		lock.unlock();

		try
		{
			timer->m_action();
		}
		catch( const std::exception & x )
		{
			m_exception_handler( x );
		}
		catch( ... )
		{
			std::ostringstream ss;
			ss << __FILE__ << "(" << __LINE__ 
				<< "): an unknown exception from timer action";
			m_error_logger( ss.str() );
			std::abort();
		}

		lock.lock();
	}

	void
	clear_all()
	{
		for( auto & item : m_wheel )
		{
			wheel_timer_t * timer = item.m_head;
			item = wheel_item_t();

			while( timer )
			{
				wheel_timer_t * t = timer;
				timer = timer->m_next;

				t->m_prev = nullptr;
				t->m_next = nullptr;

				timer_t::decrement_references( t );
			}
		}
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
				clear_all();
				break;
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

} /* namespace timertt */

