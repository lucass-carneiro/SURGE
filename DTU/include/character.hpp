#ifndef DTU_CHARACTER_HPP
#define DTU_CHARACTER_HPP

#include "player/integer_types.hpp"

#include <tuple>

namespace DTU::character {

struct sheet {
  // Spendable points
  surge::u8 attr_pts{0};

  // Attributes
  surge::u8 empathy{2};
  surge::u8 introspection{2};
  surge::u8 reasoning{2};
  surge::u8 linguistics{2};
  surge::u8 fitness{2};
  surge::u8 agility{2};

  // Conditions
  surge::u8 health_pts{6};
  surge::u8 psyche_pts{6};
  surge::u8 action_pts{6};
  surge::u8 initiative{2};

  // Skills (rank, cap, points)
  std::tuple<surge::u8, surge::u8, surge::u8> lie{0, 0, 0};
  std::tuple<surge::u8, surge::u8, surge::u8> seduce{0, 0, 0};
  std::tuple<surge::u8, surge::u8, surge::u8> persuade{0, 0, 0};
  std::tuple<surge::u8, surge::u8, surge::u8> coerce{0, 0, 0};
  std::tuple<surge::u8, surge::u8, surge::u8> deduct{0, 0, 0};
  std::tuple<surge::u8, surge::u8, surge::u8> inspect{0, 0, 0};
  std::tuple<surge::u8, surge::u8, surge::u8> academics{0, 0, 0};
  std::tuple<surge::u8, surge::u8, surge::u8> technology{0, 0, 0};
  std::tuple<surge::u8, surge::u8, surge::u8> melee{0, 0, 0};
  std::tuple<surge::u8, surge::u8, surge::u8> fists{0, 0, 0};
  std::tuple<surge::u8, surge::u8, surge::u8> dodge{0, 0, 0};
  std::tuple<surge::u8, surge::u8, surge::u8> sneak{0, 0, 0};
};

} // namespace DTU::character

#endif // DTU_CHARACTER_HPP