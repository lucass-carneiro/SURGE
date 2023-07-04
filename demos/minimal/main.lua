function surge.pre_loop()
  local t = surge.timer.generic_timer()

  t:start()
  surge.log.message("Hello message from C API")
  t:stop()
  print("C log", t:elapsed())

  t:start()
  surge.log_ffi.info("Hello message from FFI")
  t:stop()
  print("FFI log", t:elapsed())
end

function surge.update(dt)
  -- do nothing
end

function surge.draw()
  -- do nothing
end
