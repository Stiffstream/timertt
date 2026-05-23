function(timertt_add_sample target source)
	timertt_add_executable(${target} ${source})
	add_dependencies(timertt_samples ${target})
endfunction()

add_custom_target(timertt_samples)

timertt_add_sample(sample_manager_trivial_thread_unsafe
	manager/trivial_thread_unsafe/main.cpp)
timertt_add_sample(sample_manager_trivial_thread_safe
	manager/trivial_thread_safe/main.cpp)
timertt_add_sample(sample_list_trivial
	list/trivial/main.cpp)
timertt_add_sample(sample_wheel_trivial
	wheel/trivial/main.cpp)
timertt_add_sample(sample_wheel_trivial_preallocated
	wheel/trivial_preallocated/main.cpp)
timertt_add_sample(sample_heap_trivial
	heap/trivial/main.cpp)
timertt_add_sample(sample_custom_handler_and_logger
	custom_handler_and_logger/main.cpp)

if(BUILD_TESTING)
	add_test(NAME sample.manager.trivial_thread_unsafe
		COMMAND sample_manager_trivial_thread_unsafe)
	add_test(NAME sample.manager.trivial_thread_safe
		COMMAND sample_manager_trivial_thread_safe)
	add_test(NAME sample.list.trivial
		COMMAND sample_list_trivial)
	add_test(NAME sample.wheel.trivial
		COMMAND sample_wheel_trivial)
	add_test(NAME sample.wheel.trivial_preallocated
		COMMAND sample_wheel_trivial_preallocated)
	add_test(NAME sample.heap.trivial
		COMMAND sample_heap_trivial)
endif()
