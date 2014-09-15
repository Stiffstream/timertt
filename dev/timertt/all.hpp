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
#include <vector>

/*!
 * \brief Top-level project's namespace.
 */
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

	template< class T >
	T * 
	cast_to()
	{
		if( !m_timer )
			throw std::runtime_error( "timer is nullptr" );

		return static_cast< T * >(m_timer);
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
 * \brief An internal namespace with implementation details.
 */
namespace details
{

/*!
 * \brief A common base class for thread templates.
 *
 * Incapsulates basic features (like mutex, condition variable, thread
 * and so on).
 */
template<
	typename ERROR_LOGGER,
	typename ACTOR_EXCEPTION_HANDLER >
class thread_basic_t
{
	thread_basic_t( const thread_basic_t & ) = delete;
	thread_basic_t &
	operator=( const thread_basic_t & ) = delete;

public :
	//! Default constructor.
	thread_basic_t()
		:	thread_basic_t(
				ERROR_LOGGER(),
				ACTOR_EXCEPTION_HANDLER() )
	{}

	//! Constructor with all parameters.
	thread_basic_t(
		ERROR_LOGGER error_logger,
		ACTOR_EXCEPTION_HANDLER exception_handler )
		:	m_error_logger( error_logger )
		,	m_exception_handler( exception_handler )
	{
	}

	//! Destructor.
	/*!
	 * \attention Derived classes must call shutdown_and_join() in
	 * destructor by itself.
	 */
	~thread_basic_t()
	{
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
			throw std::runtime_error( "timer_thread is already started" );
		else
			m_shutdown = false;

		m_thread = std::make_shared< std::thread >(
				std::bind( &thread_basic_t::body, this ) );
	}

	//! Initiate shutdown for the timer thread without waiting for completion.
	void
	shutdown()
	{
		std::lock_guard< std::mutex > lock( m_lock );

		if( m_thread && !m_shutdown )
		{
			m_shutdown = true;
			m_condition.notify_one();
		}
	}

	//! Wait for completion of timer thread.
	/*!
	 * Method shutdown() must be called somewhere else.
	 */
	void
	join()
	{
		std::shared_ptr< std::thread > t;
		{
			std::lock_guard< std::mutex > lock( m_lock );
			t = m_thread;
		}
		if( t )
		{
			t->join();

			std::lock_guard< std::mutex > lock( m_lock );
			m_thread.reset();
		}
	}

	//! Initiate shutdown and wait for completion.
	void
	shutdown_and_join()
	{
		shutdown();
		join();
	}

protected :
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
	std::shared_ptr< std::thread > m_thread;

	//! Error logger.
	ERROR_LOGGER m_error_logger;

	//! Exception handler.
	ACTOR_EXCEPTION_HANDLER m_exception_handler;
	/*!
	 * \}
	 */

	//! Thread body.
	virtual void
	body() = 0;

};

} /* namespace details */

/*!
 * \brief A timer wheel thread template.
 */
template<
	typename ERROR_LOGGER,
	typename ACTOR_EXCEPTION_HANDLER >
class timer_wheel_thread_template_t
	:	public details::thread_basic_t< ERROR_LOGGER, ACTOR_EXCEPTION_HANDLER >
{
	//! An alias for base class.
	using base_type_t = details::thread_basic_t<
			ERROR_LOGGER, ACTOR_EXCEPTION_HANDLER >;

public :
	//! Default wheel size.
	static unsigned int
	default_wheel_size() { return 1000; }

	//! Default tick duration.
	static monotonic_clock_t::duration
	default_granularity() { return std::chrono::milliseconds( 10 ); }

	//! Default constructor.
	timer_wheel_thread_template_t()
		:	timer_wheel_thread_template_t(
				default_wheel_size(),
				default_granularity(),
				ERROR_LOGGER(),
				ACTOR_EXCEPTION_HANDLER() )
	{}

	//! Constructor with wheel size and granularity parameters.
	timer_wheel_thread_template_t(
		unsigned int wheel_size,
		monotonic_clock_t::duration granularity )
		:	timer_wheel_thread_template_t(
				wheel_size,
				granularity,
				ERROR_LOGGER(),
				ACTOR_EXCEPTION_HANDLER() )
	{
	}

	//! Constructor with all parameters.
	timer_wheel_thread_template_t(
		unsigned int wheel_size,
		monotonic_clock_t::duration granularity,
		ERROR_LOGGER error_logger,
		ACTOR_EXCEPTION_HANDLER exception_handler )
		:	base_type_t( error_logger, exception_handler )
		,	m_wheel_size( wheel_size )
		,	m_granularity( granularity )
	{
		m_wheel.resize( wheel_size );
	}

	//! Destructor.
	~timer_wheel_thread_template_t()
	{
		this->shutdown_and_join();
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
	/*!
	 * There is no need to preallocate timer object. It will
	 * be allocated automatically, but not be shown to user.
	 */
	template< class DURATION_1 >
	void
	activate(
		//! Pause for timer execution.
		DURATION_1 pause,
		//! Action for the timer.
		timer_action_t action )
	{
		activate(
				allocate(),
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
		std::lock_guard< std::mutex > lock( this->m_lock );

		if( !this->m_thread )
			throw std::runtime_error( "timer_thread is not started" );

		auto * wheel_timer = timer.cast_to< wheel_timer_t >();
		ensure_timer_deactivated( wheel_timer );

		wheel_timer->m_action = std::move(action);

		// Timer must be taken under control.
		timer_t::increment_references( wheel_timer );
		// It is an active timer now.
		wheel_timer->m_status = timer_status_t::active;

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

	//! Activate timer and schedule it for execution.
	/*!
	 * There is no need to preallocate timer object. It will
	 * be allocated automatically, but not be shown to user.
	 */
	template< class DURATION_1, class DURATION_2 >
	void
	activate(
		//! Pause for timer execution.
		DURATION_1 pause,
		//! Repetition period.
		DURATION_2 period,
		//! Action for the timer.
		timer_action_t action )
	{
		activate( allocate(), pause, period, std::move( action ) );
	}

	//! Deactivate timer and remove it from the wheel.
	void
	deactivate( timer_holder_t timer )
	{
		if( !timer )
			return;

		std::lock_guard< std::mutex > lock( this->m_lock );

		auto wheel_timer = timer.cast_to< wheel_timer_t >();
		if( timer_status_t::active == wheel_timer->m_status )
		{
			// This is normal active timer. It can be safely
			// deactivated and destroyed.
			remove_timer_from_wheel( wheel_timer );

			wheel_timer->m_status = timer_status_t::deactivated;

			// Release timer object.
			timer_t::decrement_references( wheel_timer );
		}
		else if( timer_status_t::wait_for_execution == wheel_timer->m_status )
		{
			// This timer is in execution list right now.
			// We can only changed its status.
			// Final deactivation will be done after execution of
			// timers actions.
			wheel_timer->m_status = timer_status_t::wait_for_deactivation;
		}
	}

private :
	//! Status of wheel timer.
	enum class timer_status_t : unsigned int
	{
		//! Timer is deactivated.
		/*! It can be activated or destroyed safely. */
		deactivated,
		//! Timer is activated.
		/*! It can be safely deactivated and destroyed. */
		active,
		//! Timer is in execution list and is waiting for execution.
		/*!
		 * It cannot be deactivated and destroyed right now.
		 * Status of timer can only be changed to wait_for_deactivation.
		 * And actual deactivation will be performed later, after
		 * processing of execution list.
		 */
		wait_for_execution,
		//! Timer must be deactivated after processing of execution list.
		/*!
		 * The only possible switch for the timer is to deactivated status.
		 */
		wait_for_deactivation
	};

	//! Type of wheel timer.
	struct wheel_timer_t : public timer_t
	{
		//! Status of the timer.
		std::atomic< timer_status_t > m_status;

		//! Position in the wheel.
		unsigned int m_position = 0;
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

		wheel_timer_t()
		{
			m_status = timer_status_t::deactivated;
		}
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
	//! Size of the wheel.
	const unsigned int m_wheel_size;

	//! Granularity of one time step.
	const monotonic_clock_t::duration m_granularity;

	//! Index of the current position in the wheel.
	unsigned int m_current_position = 0;

	//! The wheel data.
	std::vector< wheel_item_t > m_wheel;
	/*!
	 * \}
	 */

	static void
	ensure_timer_deactivated( const wheel_timer_t * timer )
	{
		if( timer_status_t::deactivated != timer->m_status )
			throw std::runtime_error( "timer is not in 'deactivated' state" );
	}

	template< class DURATION >
	unsigned int
	duration_to_ticks( DURATION d ) const
	{
		auto d_units = 
				std::chrono::duration_cast< monotonic_clock_t::duration >( d )
				.count();
		auto g_units = m_granularity.count();

		unsigned int r = static_cast< unsigned int >(
				/*
				 * Add g_units/2 for rounding up.
				 * For example, if d is 24ms and granularity is 10
				 * it will be (24+5)=29, and result will be 2.
				 * But if d is 25ms then (25+5)=30 and result will be 3.
				 */
				(d_units + g_units/2) / g_units );
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
		wheel_item_t & item = m_wheel[ wheel_timer->m_position ];
		if( item.m_head )
		{
			// There is a list of demands for the wheel position.
			// New demand must be added to the end of that list.
			wheel_timer->m_prev = item.m_tail;
			wheel_timer->m_next = nullptr;
			item.m_tail->m_next = wheel_timer;
			item.m_tail = wheel_timer;
		}
		else
		{
			// There is no list of demands for this wheel position yet.
			// New list must be started.
			wheel_timer->m_prev = wheel_timer->m_next = nullptr;
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
	}

	//! Thread body.
	void
	body()
	{
		std::unique_lock< std::mutex > lock( this->m_lock );

		auto tick_start_time = monotonic_clock_t::now();

		while( !this->m_shutdown )
		{
			process_current_wheel_position( lock );

			// After processing all current demands and rescheduling
			// all periodic demands the current_position must be
			// advanced.
			m_current_position += 1;
			if( m_current_position >= m_wheel_size )
				m_current_position = 0;

			// Wait for next time step.
			auto next_tick_time = tick_start_time + m_granularity;
			while( !this->m_shutdown && next_tick_time > monotonic_clock_t::now() )
			{
				this->m_condition.wait_until( lock, next_tick_time );
			}

			if( !this->m_shutdown )
			{
				tick_start_time = next_tick_time;
			}
		}

		clear_all();
	}

	void
	process_current_wheel_position(
		std::unique_lock< std::mutex > & lock )
	{
		wheel_timer_t * exec_list_head = make_exec_list();

		if( exec_list_head )
		{
			exec_actions( lock, exec_list_head );

			utilize_exec_list( exec_list_head );
		}
	}

	wheel_timer_t *
	make_exec_list()
	{
		wheel_timer_t * head = nullptr;
		wheel_timer_t * tail = nullptr;

		wheel_timer_t * timer = m_wheel[ m_current_position ].m_head;
		while( timer )
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
				t->m_status = timer_status_t::wait_for_execution;

				if( head )
				{
					tail->m_next = t;
					t->m_prev = tail;
					t->m_next = nullptr;
					tail = t;
				}
				else
				{
					head = tail = t;
					t->m_prev = t->m_next = nullptr;
				}
			}
		}

		return head;
	}

	void
	exec_actions(
		//! Object lock.
		//! This lock will be unlocked before execution of actions
		//! and locked back after.
		std::unique_lock< std::mutex > & lock,
		//! Head of execution list.
		//! Cannot be nullptr.
		wheel_timer_t * head )
	{
		lock.unlock();

		while( head )
		{
			try
			{
				// Status of timer can be changed. So it must be checked
				// just before execution. If timer is waiting for
				// deregistration it must not be executed.
				if( timer_status_t::wait_for_execution == head->m_status )
					head->m_action();
			}
			catch( const std::exception & x )
			{
				this->m_exception_handler( x );
			}
			catch( ... )
			{
				std::ostringstream ss;
				ss << __FILE__ << "(" << __LINE__ 
					<< "): an unknown exception from timer action";
				this->m_error_logger( ss.str() );
				std::abort();
			}

			head = head->m_next;
		}

		lock.lock();
	}

	void
	utilize_exec_list(
		//! Head of execution list.
		//! Cannot be null.
		wheel_timer_t * head )
	{
		while( head )
		{
			wheel_timer_t * t = head;
			head = head->m_next;

			// Actual periodic timer must be rescheduled.
			if( timer_status_t::wait_for_execution == t->m_status &&
					t->m_period )
			{
				// Timer is active again.
				t->m_status = timer_status_t::active;

				set_position_in_the_wheel( t, t->m_period );

				insert_demand_to_wheel( t );
			}
			else
			{
				// Timer must be utilized.
				t->m_status = timer_status_t::deactivated;
				timer_t::decrement_references( t );
			}
		}
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

				t->m_status = timer_status_t::deactivated;
				timer_t::decrement_references( t );
			}
		}
	}
};

//! An alias for default timer_wheel thread implementation.
using timer_wheel_thread_t = timer_wheel_thread_template_t<
		default_error_logger,
		default_actor_exception_handler >;

/*!
 * \brief A timer list thread template.
 */
template<
	typename ERROR_LOGGER,
	typename ACTOR_EXCEPTION_HANDLER >
class timer_list_thread_template_t
	:	public details::thread_basic_t< ERROR_LOGGER, ACTOR_EXCEPTION_HANDLER >
{
	//! An alias for base class.
	using base_type_t = details::thread_basic_t<
			ERROR_LOGGER, ACTOR_EXCEPTION_HANDLER >;

public :
	//! Default constructor.
	timer_list_thread_template_t()
		:	timer_list_thread_template_t(
				ERROR_LOGGER(),
				ACTOR_EXCEPTION_HANDLER() )
	{}

	//! Constructor with all parameters.
	timer_list_thread_template_t(
		ERROR_LOGGER error_logger,
		ACTOR_EXCEPTION_HANDLER exception_handler )
		:	base_type_t( error_logger, exception_handler )
	{
	}

	//! Destructor.
	~timer_list_thread_template_t()
	{
		this->shutdown_and_join();
	}

	//! Create timer to be activated later.
	timer_holder_t
	allocate()
	{
		return timer_holder_t( new list_timer_t() );
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
	/*!
	 * There is no need to preallocate timer object. It will
	 * be allocated automatically, but not be shown to user.
	 */
	template< class DURATION_1 >
	void
	activate(
		//! Pause for timer execution.
		DURATION_1 pause,
		//! Action for the timer.
		timer_action_t action )
	{
		activate(
				allocate(),
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
		std::lock_guard< std::mutex > lock( this->m_lock );

		if( !this->m_thread )
			throw std::runtime_error( "timer_thread is not started" );

		auto list_timer = timer.cast_to< list_timer_t >();
		ensure_timer_deactivated( list_timer );

		// Timer object must be correctly (re)initialized.
		list_timer->m_action = std::move( action );
		list_timer->m_when = monotonic_clock_t::now() + pause;
		list_timer->m_period = std::chrono::duration_cast<
				monotonic_clock_t::duration >( period );

		// Timer must be taken under control.
		timer_t::increment_references( list_timer );
		// It is an active timer now.
		list_timer->m_status = timer_status_t::active;

		insert_timer_to_list( list_timer );
		if( list_timer == m_head )
			// Time point for the head list item changed.
			// Work thread must handle this.
			this->m_condition.notify_one();
	}

	//! Activate timer and schedule it for execution.
	/*!
	 * There is no need to preallocate timer object. It will
	 * be allocated automatically, but not be shown to user.
	 */
	template< class DURATION_1, class DURATION_2 >
	void
	activate(
		//! Pause for timer execution.
		DURATION_1 pause,
		//! Repetition period.
		DURATION_2 period,
		//! Action for the timer.
		timer_action_t action )
	{
		activate( allocate(), pause, period, std::move( action ) );
	}

	//! Deactivate timer and remove it from the list.
	void
	deactivate(
		//! Timer to be deactivated.
		timer_holder_t timer )
	{
		if( !timer )
			return;

		std::lock_guard< std::mutex > lock( this->m_lock );

		auto list_timer = timer.cast_to< list_timer_t >();
		if( timer_status_t::active == list_timer->m_status )
		{
			// This is normal active timer. It can be safely
			// deactivated and destroyed.
			remove_timer_from_list( list_timer );

			list_timer->m_status = timer_status_t::deactivated;

			// Release timer object.
			timer_t::decrement_references( list_timer );
		}
		else if( timer_status_t::wait_for_execution == list_timer->m_status )
		{
			// This timer is in execution list right now.
			// We can only changed its status.
			// Final deactivation will be done after execution of
			// timers actions.
			list_timer->m_status = timer_status_t::wait_for_deactivation;
		}
	}

private :
//FIXME: may be this enum must be moved into details::thread_basic_t?
	//! Status of wheel timer.
	enum class timer_status_t : unsigned int
	{
		//! Timer is deactivated.
		/*! It can be activated or destroyed safely. */
		deactivated,
		//! Timer is activated.
		/*! It can be safely deactivated and destroyed. */
		active,
		//! Timer is in execution list and is waiting for execution.
		/*!
		 * It cannot be deactivated and destroyed right now.
		 * Status of timer can only be changed to wait_for_deactivation.
		 * And actual deactivation will be performed later, after
		 * processing of execution list.
		 */
		wait_for_execution,
		//! Timer must be deactivated after processing of execution list.
		/*!
		 * The only possible switch for the timer is to deactivated status.
		 */
		wait_for_deactivation
	};

	//! Type of list timer.
	struct list_timer_t : public timer_t
	{
		//! Status of the timer.
		std::atomic< timer_status_t > m_status;

		//! Time of execution for this timer.
		monotonic_clock_t::time_point m_when;

		//! Period in ticks.
		/*!
		 * Zero means that demand is single shot.
		 */
		monotonic_clock_t::duration m_period;

		//! Timer action.
		timer_action_t m_action;

		//! Previous demand in the list.
		list_timer_t * m_prev = nullptr;
		//! Next demand in the list.
		list_timer_t * m_next = nullptr;

		list_timer_t()
		{
			m_status = timer_status_t::deactivated;
		}
	};

	/*!
	 * \name Object's attributes.
	 * \{
	 */
	//! Head of the list of timers.
	list_timer_t * m_head = nullptr;

	//! Tail of the list of timers.
	list_timer_t * m_tail = nullptr;
	/*!
	 * \}
	 */

	static void
	ensure_timer_deactivated( const list_timer_t * timer )
	{
		if( timer_status_t::deactivated != timer->m_status )
			throw std::runtime_error( "timer is not in 'deactivated' state" );
	}

	void
	insert_timer_to_list( list_timer_t * timer )
	{
		list_timer_t * point = m_tail;
		while( point )
		{
			if( point->m_when > timer->m_when )
				point = point->m_prev;
			else
			{
				// This is a point to insertion (new timer must be
				// next to 'point' item).
				timer->m_next = point->m_next;

				if( point->m_next )
					point->m_next->m_prev = timer;
				point->m_next = timer;

				timer->m_prev = point;

				if( point == m_tail )
					// timer must become a new tail for the list.
					m_tail = timer;

				return;
			}
		}

		// Timer must go to the head of the list.
		timer->m_prev = nullptr;
		timer->m_next = m_head;
		if( m_head )
			m_head->m_prev = timer;
		m_head = timer;

		if( !m_tail )
			// List was empty. So there must be new tail of the list.
			m_tail = timer;
	}

	void
	remove_timer_from_list( list_timer_t * timer )
	{
		if( timer->m_prev )
			timer->m_prev->m_next = timer->m_next;
		else
			m_head = timer->m_next;

		if( timer->m_next )
			timer->m_next->m_prev = timer->m_prev;
		else
			m_tail = timer->m_prev;
	}

	//! Thread body.
	virtual void
	body() override
	{
		std::unique_lock< std::mutex > lock( this->m_lock );

		while( !this->m_shutdown )
		{
			process_ready_to_exec_timers( lock );

			sleep_for_next_event( lock );
		}

		clear_all();
	}

	void
	process_ready_to_exec_timers(
		std::unique_lock< std::mutex > & lock )
	{
		list_timer_t * exec_list_head = make_exec_list();

		if( exec_list_head )
		{
			exec_actions( lock, exec_list_head );

			utilize_exec_list( exec_list_head );
		}
	}

	void
	sleep_for_next_event(
		std::unique_lock< std::mutex > & lock )
	{
		if( !this->m_shutdown )
			if( m_head )
			{
				auto time_point = m_head->m_when;
				this->m_condition.wait_until( lock, time_point );
			}
			else
				this->m_condition.wait( lock );
	}

	list_timer_t *
	make_exec_list()
	{
		list_timer_t * head = nullptr;
		list_timer_t * tail = nullptr;

		const auto now = monotonic_clock_t::now();

		while( m_head && now >= m_head->m_when )
		{
			list_timer_t * t = m_head;
			m_head = m_head->m_next;
			if( m_head )
				m_head->m_prev = nullptr;

			if( tail )
			{
				tail->m_next = t;
				t->m_prev = tail;
				t->m_next = nullptr;
				tail = t;
			}
			else
			{
				head = tail = t;
				t->m_prev = t->m_next = nullptr;
			}

			t->m_status = timer_status_t::wait_for_execution;
		}

		if( !m_head )
			// Now the list is empty.
			// There must not be tail anymore.
			m_tail = nullptr;

		return head;
	}

	void
	exec_actions(
		//! Object lock.
		//! This lock will be unlocked before execution of actions
		//! and locked back after.
		std::unique_lock< std::mutex > & lock,
		//! Head of execution list.
		//! Cannot be nullptr.
		list_timer_t * head )
	{
		lock.unlock();

		while( head )
		{
			try
			{
				// Status of timer can be changed. So it must be checked
				// just before execution. If timer is waiting for
				// deregistration it must not be executed.
				if( timer_status_t::wait_for_execution == head->m_status )
					head->m_action();
			}
			catch( const std::exception & x )
			{
				this->m_exception_handler( x );
			}
			catch( ... )
			{
				std::ostringstream ss;
				ss << __FILE__ << "(" << __LINE__ 
					<< "): an unknown exception from timer action";
				this->m_error_logger( ss.str() );
				std::abort();
			}

			head = head->m_next;
		}

		lock.lock();
	}

	void
	utilize_exec_list(
		//! Head of execution list.
		//! Cannot be null.
		list_timer_t * head )
	{
		while( head )
		{
			auto t = head;
			head = head->m_next;

			// Actual periodic timer must be rescheduled.
			if( timer_status_t::wait_for_execution == t->m_status &&
					monotonic_clock_t::duration::zero() != t->m_period )
			{
				t->m_when += t->m_period;
				t->m_status = timer_status_t::active;

				insert_timer_to_list( t );
			}
			else
			{
				// Timer must be utilized.
				t->m_status = timer_status_t::deactivated;
				timer_t::decrement_references( t );
			}
		}
	}

	void
	clear_all()
	{
		while( m_head )
		{
			auto t = m_head;
			m_head = m_head->m_next;

			t->m_status = timer_status_t::deactivated;
			timer_t::decrement_references( t );
		}

		m_tail = nullptr;
	}
};

//! An alias for default timer_list thread implementation.
using timer_list_thread_t = timer_list_thread_template_t<
		default_error_logger,
		default_actor_exception_handler >;

/*!
 * \brief A timer heap thread template.
 */
template<
	typename ERROR_LOGGER,
	typename ACTOR_EXCEPTION_HANDLER >
class timer_heap_thread_template_t
	:	public details::thread_basic_t< ERROR_LOGGER, ACTOR_EXCEPTION_HANDLER >
{
	//! An alias for base class.
	using base_type_t = details::thread_basic_t<
			ERROR_LOGGER, ACTOR_EXCEPTION_HANDLER >;

public :
	//! Default initial capacity of heap-array.
	inline static
	std::size_t default_initial_heap_capacity()
	{
		return 64;
	}

	//! Default constructor.
	timer_heap_thread_template_t()
		:	timer_heap_thread_template_t(
				default_initial_heap_capacity(),
				ERROR_LOGGER(),
				ACTOR_EXCEPTION_HANDLER() )
	{}

	//! Constructor to specify initial capacity of heap-array.
	timer_heap_thread_template_t(
		std::size_t initial_heap_capacity )
		:	timer_heap_thread_template_t(
				initial_heap_capacity,
				ERROR_LOGGER(),
				ACTOR_EXCEPTION_HANDLER() )
	{}

	//! Constructor with all parameters.
	timer_heap_thread_template_t(
		std::size_t initial_heap_capacity,
		ERROR_LOGGER error_logger,
		ACTOR_EXCEPTION_HANDLER exception_handler )
		:	base_type_t( error_logger, exception_handler )
	{
		m_heap.reserve( initial_heap_capacity );
	}

	//! Destructor.
	~timer_heap_thread_template_t()
	{
		this->shutdown_and_join();
	}

	//! Create timer to be activated later.
	timer_holder_t
	allocate()
	{
		return timer_holder_t( new heap_timer_t() );
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
	/*!
	 * There is no need to preallocate timer object. It will
	 * be allocated automatically, but not be shown to user.
	 */
	template< class DURATION_1 >
	void
	activate(
		//! Pause for timer execution.
		DURATION_1 pause,
		//! Action for the timer.
		timer_action_t action )
	{
		activate(
				allocate(),
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
		std::lock_guard< std::mutex > lock( this->m_lock );
#if 0
std::cout << "*** activate: " << timer.get() << " ("
<< std::chrono::duration_cast< std::chrono::milliseconds >(pause).count()
<< ")" << std::endl;
#endif

		if( !this->m_thread )
			throw std::runtime_error( "timer_thread is not started" );

		auto heap_timer = timer.cast_to< heap_timer_t >();
		ensure_timer_deactivated( heap_timer );

		// Timer object must be correctly (re)initialized.
		heap_timer->m_action = std::move( action );
		heap_timer->m_when = monotonic_clock_t::now() + pause;
		heap_timer->m_period = std::chrono::duration_cast<
				monotonic_clock_t::duration >( period );

		// Timer must be taken under control.
		timer_t::increment_references( heap_timer );

		// Timer will be marked as active during insertion into
		// heap structure.
		heap_add( heap_timer );
		if( heap_timer == heap_head() )
			// Time point for the head list item changed.
			// Work thread must handle this.
			this->m_condition.notify_one();

#if 0
std::cout << "activate: " << heap_timer << " (";
for( auto t : m_heap )
std::cout << t << "-";
std::cout << ")" << std::endl;
#endif
	}

	//! Activate timer and schedule it for execution.
	/*!
	 * There is no need to preallocate timer object. It will
	 * be allocated automatically, but not be shown to user.
	 */
	template< class DURATION_1, class DURATION_2 >
	void
	activate(
		//! Pause for timer execution.
		DURATION_1 pause,
		//! Repetition period.
		DURATION_2 period,
		//! Action for the timer.
		timer_action_t action )
	{
		activate( allocate(), pause, period, std::move( action ) );
	}

	//! Deactivate timer and remove it from the list.
	void
	deactivate(
		//! Timer to be deactivated.
		timer_holder_t timer )
	{
		if( !timer )
			return;

		std::lock_guard< std::mutex > lock( this->m_lock );

		auto heap_timer = timer.cast_to< heap_timer_t >();
		if( !heap_timer->deactivated() )
		{
			// If this timer is not in processing now it can
			// be safely destroyed.
			if( heap_timer != m_timer_in_processing )
			{
				heap_remove( heap_timer );

				// We can deactivate timer only after removing.
				// Because deactivation drops actual timer position.
				heap_timer->deactivate();

				// Release timer object.
				timer_t::decrement_references( heap_timer );
			}
			else
			{
				// Otherwise m_timer_in_processing will be destroyed
				// after end of timer action processing.
				// But it must be deactivated right now.
				heap_timer->deactivate();
			}
		}
	}

private :
	//! Type of heap timer.
	struct heap_timer_t : public timer_t
	{
		//! A special value which means that timer is deactivated.
		/*!
		 * This value is illegal index in heap-array because
		 * position numbers in heap-array are started from 1, not from 0.
		 */
		static const unsigned int deactivation_indicator = 0;

		//! Time of execution for this timer.
		monotonic_clock_t::time_point m_when;

		//! Period in ticks.
		/*!
		 * Zero means that demand is single shot.
		 */
		monotonic_clock_t::duration m_period;

		//! Timer action.
		timer_action_t m_action;

		//! Position in the heap-array.
		unsigned int m_position = deactivation_indicator;

		//! Is timer deactivated.
		bool
		deactivated() const 
		{
			return deactivation_indicator == m_position;
		}

		//! Set deactivation indicator on.
		void
		deactivate()
		{
			m_position = deactivation_indicator;
		}

		//! Is this is single shot timer?
		bool
		single_shot() const
		{
			return monotonic_clock_t::duration::zero() == m_period;
		}
	};

	/*!
	 * \name Object's attributes.
	 * \{
	 */
	//! Array for holding heap data structure.
	std::vector< heap_timer_t * > m_heap;

	//! Timer which is currently in processing.
	heap_timer_t * m_timer_in_processing = nullptr;
	/*!
	 * \}
	 */

	/*!
	 * \brief Hard check for deactivation state of the timer.
	 *
	 * \throw std::runtimer_error if timer is not deactivated.
	 */
	static void
	ensure_timer_deactivated( const heap_timer_t * timer )
	{
		if( !timer->deactivated() )
			throw std::runtime_error( "timer is not in 'deactivated' state" );
	}

	//! Thread body.
	virtual void
	body() override
	{
		std::unique_lock< std::mutex > lock( this->m_lock );

		while( !this->m_shutdown )
		{
			process_ready_to_exec_timers( lock );

			sleep_for_next_event( lock );
		}

		clear_all();
	}

//FIXME: make more detailed description of this method.
//Especially that object is unlocked for execution for every demands.
	//! Processing all elapsed timers.
	void
	process_ready_to_exec_timers(
		std::unique_lock< std::mutex > & lock )
	{
		// Process timers in loop until there are elapsed timers.
		auto now = monotonic_clock_t::now();
		while( !this->m_shutdown && !heap_empty() &&
				now > heap_head()->m_when )
		{
			m_timer_in_processing = heap_head();
			heap_remove( m_timer_in_processing );

			execute_timer_in_processing( lock );

			// If timer has become deactive it must be removed even
			// it is periodic timer.
			if( m_timer_in_processing->deactivated() ||
					m_timer_in_processing->single_shot() )
			{
				m_timer_in_processing->deactivate();
				timer_t::decrement_references( m_timer_in_processing );
			}
			else
			{
				// This is active periodic timer and it must be resheduled.
				m_timer_in_processing->m_when +=
						m_timer_in_processing->m_period;
				heap_add( m_timer_in_processing );
			}

			m_timer_in_processing = nullptr;
		}
	}

	void
	sleep_for_next_event(
		std::unique_lock< std::mutex > & lock )
	{
		if( !this->m_shutdown )
			if( !heap_empty() )
			{
				auto time_point = heap_head()->m_when;
				this->m_condition.wait_until( lock, time_point );
			}
			else
				this->m_condition.wait( lock );
	}

	void
	execute_timer_in_processing(
		//! Object lock.
		//! This lock will be unlocked before execution of actions
		//! and locked back after.
		std::unique_lock< std::mutex > & lock )
	{
		lock.unlock();

		try
		{
			m_timer_in_processing->m_action();
		}
		catch( const std::exception & x )
		{
			this->m_exception_handler( x );
		}
		catch( ... )
		{
			std::ostringstream ss;
			ss << __FILE__ << "(" << __LINE__ 
				<< "): an unknown exception from timer action";
			this->m_error_logger( ss.str() );
			std::abort();
		}

		lock.lock();
	}

	//! Clear all timer demands.
	void
	clear_all()
	{
		for( auto t : m_heap )
		{
			t->deactivate();
			timer_t::decrement_references( t );
		}

		m_heap.clear();

		m_timer_in_processing = nullptr;
	}

	/*!
	 * \name Methods for work with heap data structure.
	 * \{
	 */
	//! Is heap data structure empty?
	bool
	heap_empty() const
	{
		return m_heap.empty();
	}

	//! Get the minimal timer.
	/*!
	 * \attention This method must be called only on non-empty heap.
	 */
	heap_timer_t *
	heap_head() const
	{
		return m_heap.front();
	}

	//! Add new timer to the heap data structure.
	void
	heap_add( heap_timer_t * timer )
	{
		timer->m_position = m_heap.size() + 1;
		m_heap.push_back( timer );

		while( 1 != timer->m_position )
		{
			auto parent = heap_item( timer->m_position / 2 );
#if 0
std::cout << " timer: " << timer << ", parent: " << parent << std::endl;
#endif
			if( parent->m_when > timer->m_when )
			{
#if 0
std::cout << "swapping 1(" << timer->m_position << ":" << parent->m_position << ")" << std::endl;
#endif
				// timer must be heap-up on the place of the parent node.
				heap_swap( timer, parent );
#if 0
std::cout << "swapping 2(" << timer->m_position << ":" << parent->m_position << ")" << std::endl;
#endif
			}
			else
				// There is no need to modify heap structure anymore.
				break;
		}
	}

	//! Remove timer from the heap data structure.
	void
	heap_remove( heap_timer_t * timer )
	{
		if( timer->m_position == m_heap.size() )
			// A special case: timer to remove is a last added item
			// in the heap. It could be simply removed from heap
			// without any other actions.
			m_heap.pop_back();
		else
		{
			auto last_item = m_heap.back();
			heap_swap( timer, last_item );
			m_heap.pop_back();

			// last_item must be heap-down to the appropriate place.
			while( true )
			{
				auto left_index = last_item->m_position * 2;
				auto right_index = left_index + 1;
				auto min_index = last_item->m_position;
#if 0
std::cout << " -> " << min_index << ", (" << left_index << ","
<< right_index << ")" << std::endl;
#endif

				if( left_index <= m_heap.size() &&
						heap_item( left_index )->m_when <=
								heap_item( min_index )->m_when )
					min_index = left_index;

				if( right_index <= m_heap.size() &&
						heap_item( right_index )->m_when <=
								heap_item( min_index )->m_when )
					min_index = right_index;

#if 0
std::cout << " --> " << min_index << ", (" << left_index << ","
<< right_index << ")" << std::endl;
#endif
				if( min_index != last_item->m_position )
					heap_swap( last_item, heap_item( min_index ) );
				else
					// Heap structure is correct.
					break;
			}
		}
#if 0
std::cout << "after remove(" << timer << "): " << " (";
for( auto t : m_heap )
std::cout << t << "-";
std::cout << ")" << std::endl;
#endif
	}

	//! Swap two heap nodes.
	void
	heap_swap( heap_timer_t * a, heap_timer_t * b )
	{
#if 0
std::cout << "before swap: " << " (";
for( auto t : m_heap )
std::cout << t << "-";
std::cout << ")" << std::endl;
#endif
		m_heap[ a->m_position - 1 ] = b;
		m_heap[ b->m_position - 1 ] = a;
#if 0
std::cout << " after swap: " << " (";
for( auto t : m_heap )
std::cout << t << "-";
std::cout << ")" << std::endl;
#endif

		std::swap( a->m_position, b->m_position );
	}

	//! Get timer by it index.
	/*!
	 * This accessor work with respect that positions are started from 1.
	 */
	heap_timer_t *
	heap_item( std::size_t position ) const
	{
		return m_heap[ position - 1 ];
	}
	/*!
	 * \}
	 */
};

//! An alias for default timer_list thread implementation.
using timer_heap_thread_t = timer_heap_thread_template_t<
		default_error_logger,
		default_actor_exception_handler >;
} /* namespace timertt */

