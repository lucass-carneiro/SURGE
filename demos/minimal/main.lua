function surge.pre_loop()
  surge.log.message("Hello message from C API")
  surge.log_ffi.info("Hello message from FFI")
end

function surge.update(dt)
  -- do nothing
end

function surge.draw()
  -- do nothing
end
