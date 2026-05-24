timertt_add_executable(_benchmark_schedule_erase
	schedule_erase/main.cpp)
target_include_directories(_benchmark_schedule_erase PRIVATE
	${PROJECT_SOURCE_DIR}/tests/support)

timertt_add_executable(_benchmark_same_time_demands
	same_time_demands/main.cpp)
target_include_directories(_benchmark_same_time_demands PRIVATE
	${PROJECT_SOURCE_DIR}/tests/support)

add_custom_target(timertt_benchmarks
	DEPENDS
		_benchmark_schedule_erase
		_benchmark_same_time_demands)

