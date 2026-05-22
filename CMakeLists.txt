cmake_minimum_required(VERSION 3.16)

project(timertt
	VERSION 1.2.5
	DESCRIPTION "Timer Thread Template"
	LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
	set(TIMERTT_IS_TOP_LEVEL ON)
else()
	set(TIMERTT_IS_TOP_LEVEL OFF)
endif()

option(TIMERTT_BUILD_TESTS "Build timertt tests" ${TIMERTT_IS_TOP_LEVEL})
option(TIMERTT_BUILD_SAMPLES "Build timertt samples" ${TIMERTT_IS_TOP_LEVEL})
option(TIMERTT_BUILD_BENCHMARKS "Build timertt benchmark executables" OFF)
option(TIMERTT_STRICT_WARNINGS "Treat project warnings as errors" OFF)
option(TIMERTT_BUILD_DOCS "Add a Doxygen documentation target" OFF)

include(CTest)

add_subdirectory(dev)
