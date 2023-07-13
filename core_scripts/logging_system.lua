local ffi = require("ffi")

ffi.cdef [[
  void error(const char *);
  void info(const char *);
  void warn(const char *);
]]

return ffi.load(ffi.os == "Windows" and "surge_logging.dll" or
                    "./libsurge_logging.so")
