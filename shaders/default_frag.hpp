#ifndef SURGE_SHADER_DEFAULT_FRAG_HPP
#define SURGE_SHADER_DEFAULT_FRAG_HPP

namespace surge {

constexpr const char *shader_default_frag_src =
    "#version 330 core\nout vec4 FragColor;\n\nvoid main()\n{\nFragColor = "
    "vec4(1.0f, 0.5f, 0.2f, 1.0f);\n} \0";

}; // namespace surge

#endif // SURGE_SHADER_DEFAULT_FRAG_HPP
