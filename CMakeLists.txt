cmake_minimum_required(VERSION 3.16)

project(timertt
	VERSION 1.2.5
	DESCRIPTION "Timer Thread Template"
	LANGUAGES CXX)

include(GNUInstallDirs)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
	set(TIMERTT_IS_TOP_LEVEL ON)
else()
	set(TIMERTT_IS_TOP_LEVEL OFF)
endif()

option(TIMERTT_BUILD_TESTS "Build timertt tests" ${TIMERTT_IS_TOP_LEVEL})
option(TIMERTT_BUILD_SAMPLES "Build timertt samples" ${TIMERTT_IS_TOP_LEVEL})
option(TIMERTT_BUILD_BENCHMARKS "Build timertt benchmark executables" OFF)
option(TIMERTT_STRICT_WARNINGS "Treat project warnings as errors" ${TIMERTT_IS_TOP_LEVEL})
option(TIMERTT_BUILD_DOCS "Add a Doxygen documentation target" OFF)
option(TIMERTT_INSTALL "Install timertt headers and CMake package files" ${TIMERTT_IS_TOP_LEVEL})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_CXX_STANDARD)
	message(STATUS "C++11 will be used by default")
	set(CMAKE_CXX_STANDARD 11)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	set(CXX_EXTENSIONS NO)
else()
	message(STATUS "C++ standard is set to C++${CMAKE_CXX_STANDARD}")
endif()

include(CTest)

find_package(Threads REQUIRED)

add_library(timertt INTERFACE)
add_library(timertt::timertt ALIAS timertt)

target_include_directories(timertt INTERFACE
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(timertt INTERFACE Threads::Threads)

add_library(timertt_project_options INTERFACE)
target_compile_options(timertt_project_options INTERFACE
	$<$<CXX_COMPILER_ID:MSVC>:/W3>
	$<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic -Wno-unused-function>
	$<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>)

if(TIMERTT_STRICT_WARNINGS)
	target_compile_options(timertt_project_options INTERFACE
		$<$<CXX_COMPILER_ID:MSVC>:/WX>
		$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
		$<$<CXX_COMPILER_ID:Clang>:-Wno-c++98-compat
			-Wno-c++98-compat-pedantic
			-Wno-padded
			-Wno-missing-noreturn
			-Wno-weak-vtables
			-Wno-missing-prototypes
			-Wno-missing-variable-declarations
			-Wno-exit-time-destructors
			-Wno-global-constructors
			-Wno-reserved-identifier
			-Wno-implicit-int-float-conversion
			-Wno-header-hygiene
		>
	)
endif()

function(timertt_add_executable target source)
	add_executable(${target} ${source})
	target_link_libraries(${target} PRIVATE timertt::timertt timertt_project_options)
endfunction()

if(TIMERTT_BUILD_TESTS AND BUILD_TESTING)
	add_subdirectory(tests)
endif()

if(TIMERTT_BUILD_SAMPLES)
	add_subdirectory(samples)
endif()

if(TIMERTT_BUILD_BENCHMARKS)
	add_subdirectory(benchmarks)
endif()

if(TIMERTT_BUILD_DOCS)
	add_subdirectory(docs)
endif()

if(TIMERTT_INSTALL)
	include(CMakePackageConfigHelpers)

	install(DIRECTORY include/timertt
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

	install(TARGETS timertt
		EXPORT timerttTargets)

	install(EXPORT timerttTargets
		FILE timerttTargets.cmake
		NAMESPACE timertt::
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/timertt)

	configure_package_config_file(
		cmake/timerttConfig.cmake.in
		${CMAKE_CURRENT_BINARY_DIR}/timerttConfig.cmake
		INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/timertt)

	write_basic_package_version_file(
		${CMAKE_CURRENT_BINARY_DIR}/timerttConfigVersion.cmake
		VERSION ${PROJECT_VERSION}
		COMPATIBILITY SameMajorVersion)

	install(FILES
		${CMAKE_CURRENT_BINARY_DIR}/timerttConfig.cmake
		${CMAKE_CURRENT_BINARY_DIR}/timerttConfigVersion.cmake
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/timertt)
endif()
