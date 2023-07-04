local ffi = require("ffi")

ffi.cdef [[
  void info(const char *);
  void error(const char *);
  void warn(const char *);
  void debug(const char *);
]]

return ffi.load(ffi.os == "Windows" and "TODO" or "./libsurge_logging.so")
