execute_process(
	COMMAND "${TEST_EXECUTABLE}"
	RESULT_VARIABLE result)

if(result EQUAL 0)
	message(FATAL_ERROR "Negative test unexpectedly passed: ${TEST_EXECUTABLE}")
endif()

message(STATUS "Negative test failed as expected: ${TEST_EXECUTABLE} (${result})")

