#ifndef SURGE_OPTIONS_HPP
#define SURGE_OPTIONS_HPP

#define SURGE_VERSION_MAJOR 1 // NOLINT(cppcoreguidelines-macro-usage)
#define SURGE_VERSION_MINOR 3 // NOLINT(cppcoreguidelines-macro-usage)
#define SURGE_VERSION_PATCH 0 // NOLINT(cppcoreguidelines-macro-usage)

#define SURGE_VERSION_MAJOR_STRING "1" // NOLINT(cppcoreguidelines-macro-usage)
#define SURGE_VERSION_MINOR_STRING "3" // NOLINT(cppcoreguidelines-macro-usage)
#define SURGE_VERSION_PATCH_STRING "0" // NOLINT(cppcoreguidelines-macro-usage)

#define SURGE_OPENGL_ERROR_BUFFER_SIZE 1024 // NOLINT(cppcoreguidelines-macro-usage)

#define SURGE_SYSTEM_Windows
#define SURGE_SYSTEM_NOT_POSIX
#define SURGE_COMPILER_MSVC
#define SURGE_BUILD_TYPE_Debug

#define SURGE_USE_LOG
#define SURGE_USE_LOG_COLOR
#define SURGE_GL_LOG
#define SURGE_LOG_GL_NOTIFICATIONS
#define SURGE_USE_VK_VALIDATION_LAYERS
/* #undef SURGE_DEBUG_MEMORY */
#define SURGE_ENABLE_HR
/* #undef SURGE_ENABLE_TRACY */
#define SURGE_ENABLE_FRAME_STEPPING

#endif // SURGE_OPTIONS_HPP
