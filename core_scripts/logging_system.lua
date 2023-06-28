local ffi = require("ffi")

ffi.cdef [[
  void log_info(const char *);
  void log_error(const char *);
  void log_warn(const char *);
  void log_debug(const char *);
]]

return ffi.load(ffi.os == "Windows" and "TODO" or "./libsurge_logging.so")

