#include <timertt/all.hpp>

typedef timertt::timer_wheel_thread_t<
		timertt::default_error_logger,
		timertt::default_actor_exception_handler >
	timer_thread_t;

#include "test_cases.inl"

