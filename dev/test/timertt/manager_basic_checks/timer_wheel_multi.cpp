#include <timertt/all.hpp>

typedef timertt::timer_wheel_manager_template_t<
		timertt::thread_safety::safe,
		timertt::default_error_logger,
		timertt::default_actor_exception_handler >
	timer_manager_t;

#include "test_cases.inl"

