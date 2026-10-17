#include "Check.hpp"

#ifdef _WIN32
#include <crtdbg.h>
#include <cstdlib>
#endif

int main() {
#ifdef _WIN32
    // A failed assertion of the debug runtime is written to stderr and ends the run, instead of a
    // dialog that waits for someone at the computer.
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    return check::RunAll();
}
