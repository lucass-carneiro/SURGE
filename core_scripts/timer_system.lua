local ffi = require("ffi")

ffi.cdef [[
    typedef struct generic_timer generic_timer;
    
    generic_timer* generic_timer_new();
    void generic_timer_delete(generic_timer *this_);
    void generic_timer_start(generic_timer *this_);
    void generic_timer_stop(generic_timer *this_);
    double generic_timer_elapsed(generic_timer *this_);

    int poll(struct pollfd *fds, unsigned long nfds, int timeout);
]]

local surge_timer = ffi.load(ffi.os == "Windows" and "TODO" or
                                 "./libsurge_timer.so")

local generic_timer_index = {
  start = surge_timer.generic_timer_start,
  stop = surge_timer.generic_timer_stop,
  elapsed = surge_timer.generic_timer_elapsed,
}

local generic_timer_mt = ffi.metatype("generic_timer",
                                      {__index = generic_timer_index})

local timer = {}

function timer.generic_timer()
  local obj = surge_timer.generic_timer_new()
  ffi.gc(obj, surge_timer.generic_timer_delete)
  return obj
end

return timer
