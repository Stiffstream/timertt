find_package(Threads REQUIRED)

add_library(timertt INTERFACE)
add_library(timertt::timertt ALIAS timertt)

target_compile_features(timertt INTERFACE cxx_std_11)
target_include_directories(timertt INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:include>)
target_link_libraries(timertt INTERFACE Threads::Threads)

add_library(timertt_project_options INTERFACE)
target_compile_options(timertt_project_options INTERFACE
	$<$<CXX_COMPILER_ID:MSVC>:/W3>
	$<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic -Wno-unused-function>
	$<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>)

if(TIMERTT_STRICT_WARNINGS)
	target_compile_options(timertt_project_options INTERFACE
		$<$<CXX_COMPILER_ID:MSVC>:/WX>
		$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>)
endif()

function(timertt_add_executable target source)
	add_executable(${target} ${source})
	target_link_libraries(${target} PRIVATE timertt::timertt timertt_project_options)
endfunction()

if(TIMERTT_BUILD_TESTS AND BUILD_TESTING)
	add_subdirectory(test/timertt)
endif()

if(TIMERTT_BUILD_SAMPLES)
	add_subdirectory(sample/timertt)
endif()

if(TIMERTT_BUILD_DOCS)
	find_package(Doxygen)
	if(DOXYGEN_FOUND)
		add_custom_target(timertt_docs
			COMMAND ${DOXYGEN_EXECUTABLE}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../doxygen
			COMMENT "Building timertt API documentation")
	endif()
endif()

