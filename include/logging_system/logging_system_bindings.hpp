#ifndef SURGE_LOGGING_SYSTEM_BINDINGS_HPP
#define SURGE_LOGGING_SYSTEM_BINDINGS_HPP

extern "C" void log_info(const char *) noexcept;
extern "C" void log_error(const char *) noexcept;
extern "C" void log_warn(const char *) noexcept;
extern "C" void log_debug(const char *) noexcept;

#endif // SURGE_LOGGING_SYSTEM_BINDINGS_HPP