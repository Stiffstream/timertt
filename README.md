timertt (Timer Thread Template) is a small, template based, header only library
for C++11. It implements timer threads: thread which handles timers. timertt
has no external dependecies except standard C++11 library.

timertt was developed as part of SObjectizer[1] project but can be used as
standalone library.

timertt is distributed under 3-clauses BSD license (see LICENSE file).

Obtaining and Using
===================

timertt is a header-only library. Add `include` to your compiler include path
and include the public header:

> #include <timertt/all.hpp>

CMake users can consume timertt directly from a source checkout:

> add_subdirectory(path/to/timertt)
> target_link_libraries(your_target PRIVATE timertt::timertt)

An installed package can be consumed in the usual CMake package mode:

> find_package(timertt CONFIG REQUIRED)
> target_link_libraries(your_target PRIVATE timertt::timertt)

Building With CMake
===================

Configure and build tests and samples:

> cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release
> cmake --build build-cmake --config Release --parallel

`CMAKE_BUILD_TYPE` is used by single-configuration generators such as Ninja and
Unix Makefiles. Multi-configuration generators such as Visual Studio use the
`--config Release` argument during build, test, and install steps.

Run the test suite:

> ctest --test-dir build-cmake -C Release --output-on-failure

Benchmarks are optional and can be enabled explicitly:

> cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release -DTIMERTT_BUILD_BENCHMARKS=ON
> cmake --build build-cmake --config Release --target timertt_benchmarks --parallel

Install headers and CMake package files:

> cmake --install build-cmake --config Release --prefix <install-prefix>

API reference documentation can be built with Doxygen:

> cmake -S . -B build-cmake -DTIMERTT_BUILD_DOCS=ON
> cmake --build build-cmake --config Release --target timertt_docs

Generated documentation will be stored in build-cmake/docs/html.

The old Ruby/Mxx_ru build files have been removed.

Main Project Documentation
==========================

Main documentation for timertt library is on SObjectizer’s wiki[3] on
SourceForge.

Bug Reporting and Feedback
==========================

For bug reporting, proposals, discussions and stuff like that the appropriate
SObjectizer’s forums on SourceForge must be used [4],[5],[6]

References
==========

[1] http://sourceforge.net/projects/sobjectizer/
[2] http://sourceforge.net/projects/sobjectizer/files/timertt
[3] http://sourceforge.net/p/sobjectizer/wiki/Timer%20Thread%20Template/
[4] http://sourceforge.net/p/sobjectizer/bugs/
[5] http://sourceforge.net/p/sobjectizer/feature-requests/
[6] http://sourceforge.net/p/sobjectizer/discussion/
