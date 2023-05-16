local queue = {}
queue.__index = queue
queue.__name = "queue"

function queue:new ()
  local q = {}
  setmetatable(q, queue)    

  q.first = 0
  q.last = -1

  return q
end

function queue:size()
  if self.first <= self.last then
    return self.last
  else
    return 0
  end
end

function queue:push_back(value)
  local last = self.last + 1
  self.last = last
  self[last] = value
end

function queue:pop_front()
  local first = self.first

  if first > self.last then
    return nil
  end
  
  local value = self[first]
  self[first] = nil
  self.first = first + 1
  
  return value
end

function queue:front()
  if self.first <= self.last then
    return self[self.first]
  else
    return nil
  end
end

function queue:replace_front(new_value)
  if self.first <= self.last then
    self[self.first] = new_value
  end
end

return queue