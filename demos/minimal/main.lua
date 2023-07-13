function surge.pre_loop()
  surge.log_ffi
      .info("The task executor ptr: " .. tostring(surge.tasks.executor))
  surge.log_ffi.info("Worker ID " .. tostring(surge.tasks.worker_id))
end

function surge.update(dt)
  -- do nothing
end

function surge.draw()
  -- do nothing
end

function surge.key_event(key, action, mods)
  -- do nothing
end

function surge.mouse_button_event(button, action, mods)
  -- do nothing
end

function surge.mouse_scroll_event(xoffset, yoffset)
  -- do nothing
end
