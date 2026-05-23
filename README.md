# timertt

timertt (Timer Thread Template) is a small, template-based, header-only C++11
library. It implements timer threads: threads that handle timers.

timertt was developed as part of the
[SObjectizer](https://github.com/Stiffstream/sobjectizer) project, but can be
used as a standalone library.

timertt is distributed under the 3-clause BSD license. See [LICENSE](LICENSE).

## Obtaining and using

timertt is a header-only library. Add `include` to your compiler include path
and include the public header:

```cpp
#include <timertt/all.hpp>
```

CMake users can consume timertt directly from a source checkout:

```cmake
add_subdirectory(path/to/timertt)
target_link_libraries(your_target PRIVATE timertt::timertt)
```

An installed package can be consumed in the usual CMake package mode:

```cmake
find_package(timertt CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE timertt::timertt)
```

## Building with CMake

Configure and build tests and samples:

```sh
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build-cmake --config Release --parallel
```

`CMAKE_BUILD_TYPE` is used by single-configuration generators such as Ninja and
Unix Makefiles. Multi-configuration generators such as Visual Studio use the
`--config Release` argument during build, test, and install steps.

Run the test suite:

```sh
ctest --test-dir build-cmake -C Release --output-on-failure
```

Benchmarks are optional and can be enabled explicitly:

```sh
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release -DTIMERTT_BUILD_BENCHMARKS=ON
cmake --build build-cmake --config Release --target timertt_benchmarks --parallel
```

Install headers and CMake package files:

```sh
cmake --install build-cmake --config Release --prefix <install-prefix>
```

API reference documentation can be built with Doxygen -- just run `doxygen`
command and generated documentation will be stored in `_generated_docs/html`.

## Main project documentation

Main documentation for timertt is on
[SObjectizer's wiki](http://sourceforge.net/p/sobjectizer/wiki/Timer%20Thread%20Template/)
on SourceForge.

## Bug reporting and feedback

Use the SObjectizer SourceForge trackers and forums for bug reports, proposals,
discussions, and related feedback.

## References

- [SObjectizer project](https://github.com/Stiffstream/sobjectizer)
- [timertt files on SourceForge](http://sourceforge.net/projects/sobjectizer/files/timertt)
- [timertt documentation wiki](http://sourceforge.net/p/sobjectizer/wiki/Timer%20Thread%20Template/)
- [Bug tracker](http://sourceforge.net/p/sobjectizer/bugs/)
- [Feature requests](http://sourceforge.net/p/sobjectizer/feature-requests/)
- [Discussion forum](http://sourceforge.net/p/sobjectizer/discussion/)
