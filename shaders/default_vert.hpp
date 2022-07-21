#ifndef SURGE_SHADER_DEFAULT_VERT_HPP
#define SURGE_SHADER_DEFAULT_VERT_HPP

namespace surge {

constexpr const char *shader_default_vert_src =
    "#version 440 core\nlayout (location = 0) in vec3 aPos;\n\nvoid main() "
    "{\ngl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n}\0";

}; // namespace surge

#endif // SURGE_SHADER_DEFAULT_VERT_HPP
