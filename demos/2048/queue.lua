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

function queue:push(value)
  local last = self.last + 1
  self.last = last
  self[last] = value
end

function queue:pop()
  local first = self.first

  if first > self.last then
    return nil
  end
  
  local value = self[first]
  self[first] = nil
  self.first = first + 1
  
  return value
end

return queue