#pragma once

#if defined(_MSC_VER)

#include <cstdlib>
#include <crtdbg.h>

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace timertt_tests
{

inline void
suppress_msvc_failure_dialogs()
{
#if defined(_WIN32)
	SetErrorMode(
		SEM_FAILCRITICALERRORS |
		SEM_NOGPFAULTERRORBOX |
		SEM_NOOPENFILEERRORBOX );
#endif

	_set_abort_behavior( 0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT );

#if defined(_DEBUG)
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDERR );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDERR );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR );
#endif
}

} /* namespace timertt_tests */

#endif /* defined(_MSC_VER) */
