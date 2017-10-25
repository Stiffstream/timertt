#include <timertt/all.hpp>

using timer_thread_t = timertt::timer_heap_thread_template<
	timertt::default_timer_action_type,
	timertt::default_error_logger,
	timertt::default_actor_exception_handler >;

#include "test_cases.inl"
