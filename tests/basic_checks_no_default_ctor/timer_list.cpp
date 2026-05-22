#include <timertt/all.hpp>

#include "timer_action.hpp"

using timer_thread_t = timertt::timer_list_thread_template<
	test_action,
	timertt::default_error_logger,
	timertt::default_actor_exception_handler >;

#include "test_cases.inl"

