#include <timertt/all.hpp>

#include "throwing_logger.hpp"

typedef timertt::timer_wheel_manager_template<
		timertt::thread_safety::unsafe,
		timertt::default_timer_action_type,
		throwing_logger,
		timertt::default_actor_exception_handler >
	timer_manager_t;

#include "test_cases.inl"

