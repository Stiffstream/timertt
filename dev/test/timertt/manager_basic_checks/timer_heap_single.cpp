#include <timertt/all.hpp>

typedef timertt::timer_heap_manager_template<
		timertt::thread_safety::unsafe,
		timertt::default_timer_action_type,
		timertt::default_error_logger,
		timertt::default_actor_exception_handler >
	timer_manager_t;

#include "test_cases.inl"

