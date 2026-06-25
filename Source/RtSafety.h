#pragma once

// Expands to [[clang::nonblocking]] only under the RealtimeSanitizer build,
// where the rt_check CMake target defines TAPESTOP_RT_NONBLOCKING on the command
// line (see CMakeLists.txt, TAPESTOP_RT_SANITIZE). For every other build — MSVC,
// normal Release, static analysis — it expands to nothing, so the annotation is
// invisible and has zero effect on the shipped plugin.
#ifndef TAPESTOP_RT_NONBLOCKING
#define TAPESTOP_RT_NONBLOCKING
#endif
