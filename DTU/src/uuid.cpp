#include "uuid.hpp"

#include "player/options.hpp"

#include <cstring>

#ifdef SURGE_SYSTEM_Linux
#  include <uuid/uuid.h>

DTU::uuid::uuid() noexcept {
  uuid_t out{};
  uuid_generate(static_cast<u8 *>(out));
  std::memcpy(id.data(), static_cast<void *>(out), 16);
}

#else
#  error MISSING IMPLEMENTATION
#endif

constexpr auto DTU::uuid::operator==(const uuid &rhs) noexcept -> bool {
  for (std::size_t i = 0; i < id.size(); i++) {
    if (id[i] != rhs.id[i]) {
      return false;
    }
  }
  return true;
}