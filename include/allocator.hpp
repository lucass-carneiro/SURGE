#ifndef SURGE_ALLOCATOR_HPP
#define SURGE_ALLOCATOR_HPP

#include <mimalloc.h>

namespace surge {

void init_mimalloc() noexcept;

}

#endif // SURGE_ALLOCATOR_HPP