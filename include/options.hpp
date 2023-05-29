#ifndef SURGE_OPTIONS_HPP
#define SURGE_OPTIONS_HPP

#define SURGE_VERSION_MAJOR 1 // NOLINT(cppcoreguidelines-macro-usage)
#define SURGE_VERSION_MINOR 0 // NOLINT(cppcoreguidelines-macro-usage)
#define SURGE_VERSION_PATCH 0 // NOLINT(cppcoreguidelines-macro-usage)

#define SURGE_VERSION_MAJOR_STRING "1" // NOLINT(cppcoreguidelines-macro-usage)
#define SURGE_VERSION_MINOR_STRING "0" // NOLINT(cppcoreguidelines-macro-usage)
#define SURGE_VERSION_PATCH_STRING "0" // NOLINT(cppcoreguidelines-macro-usage)

#define SURGE_OPENGL_ERROR_BUFFER_SIZE 1024 // NOLINT(cppcoreguidelines-macro-usage)
#define SURGE_FPS_COUNTER_SAMPLE_SIZE 1024 // NOLINT(cppcoreguidelines-macro-usage)

#define SURGE_SYSTEM_Windows
#define SURGE_SYSTEM_NOT_POSIX
#define SURGE_COMPILER_MSVC

#define SURGE_USE_LOG
#define SURGE_USE_LOG_COLOR
/* #undef SURGE_DEBUG_MEMORY */
#define SURGE_STBIMAGE_ERRORS
#define SURGE_ENABLE_THREADS
/* #undef SURGE_ENABLE_TRACY */


#endif // SURGE_OPTIONS_HPP
