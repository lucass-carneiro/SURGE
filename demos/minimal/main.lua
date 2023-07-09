function surge.pre_loop()
  local t = surge.timer.generic_timer()

  local t_c = 0
  local t_ffi = 0

  local reps = 1

  for _ = 1, reps, 1 do
    t:start()
    surge.log.message("Hello message from C API")
    t:stop()
    t_c = t_c + t:elapsed()
  end

  for _ = 1, reps, 1 do
    t:start()
    surge.log_ffi.info("Hello message from FFI")
    t:stop()
    t_ffi = t_ffi + t:elapsed()
  end

  print("C log", t_c / reps)
  print("FFI log", t_ffi / reps)
end

function surge.update(dt)
  surge.log_ffi.info("dt = " .. tostring(dt))
  -- do nothing
end

function surge.draw()
  -- do nothing
end
