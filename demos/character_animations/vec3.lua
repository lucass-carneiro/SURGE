local vec3 = {}
vec3.meta = {}

function vec3.new(x, y, z)
    local v3 = {}
    setmetatable(v3, vec3.meta)
    
    v3.x = x
    v3.y = y
    v3.z = z

    function v3:norm()
        return vec3.norm(self)
    end

    function v3:normalize()
        local norm = vec3.norm(self)
        self.x = self.x / norm
        self.y = self.y / norm
        self.z = self.z / norm
    end

    function v3:dot(other)
        return vec3.dot(self, other)
    end

    function v3:cross(other)
        return vec3.cross(self, other)
    end

    function v3:scale(scalar)
        self.x = self.x * scalar
        self.y = self.y * scalar
        self.z = self.z * scalar
    end

    function v3:unwrap()
        vec3.unwrap(self)
    end

    return v3
end

function vec3.norm(vec)
    return math.sqrt(vec3.dot(vec, vec))
end

function vec3.normalize(vec)
    local norm = vec3.norm(vec)
    return vec3.new(vec.x / norm, vec.y / norm, vec.z / norm)
end

function vec3.dot(a, b)
    return a.x * b.x + a.y * b.y + a.z * b.z
end

function vec3.cross(a, b)
    return vec3.new((a.y*b.z - a.z*b.y), (a.z*b.x - a.x*b.z), (a.x*b.y - a.y*b.x))
end

function vec3.scale(vec, scalar)
    return vec3.new(vec.x * scalar, vec.y * scalar, vec.z * scalar)
end

function vec3.unwrap(vec)
    return vec.x, vec.y, vec.z
end

function vec3.meta.__add(a, b)
    return vec3.new(a.x + b.x, a.y + b.y, a.z + b.z)
end

function vec3.meta.__sub(a, b)
    return vec3.new(a.x - b.x, a.y - b.y, a.z - b.z)
end

function vec3.meta.__mul(a, b)
    return vec3.new(a.x * b.x, a.y * b.y, a.z * b.z)
end

function vec3.meta.__div(a, b)
    return vec3.new(a.x / b.x, a.y / b.y, a.z / b.z)
end

function vec3.meta.__unm(vec)
    return vec3.new(-vec.x, -vec.y, -vec.z)
end

function vec3.meta.__tostring(vec)
    return "(" .. vec.x .. ", " .. vec.y .. ", " .. vec.z .. ")"
end

return vec3;