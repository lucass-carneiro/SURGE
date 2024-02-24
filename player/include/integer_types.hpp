#ifndef SURGE_INTEGER_TYPES_HPP
#define SURGE_INTEGER_TYPES_HPP

#include <cstddef>
#include <cstdint>

namespace surge {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using usize = std::size_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

// static constexpr auto operator"" U8(u8 literal) noexcept -> u8 { return literal; }

} // namespace surge

#endif // SURGE_INTEGER_TYPES_HPP