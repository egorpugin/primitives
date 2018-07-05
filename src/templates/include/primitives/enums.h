#pragma once

#include <type_traits>

template <typename E>
constexpr std::underlying_type_t<E> toIndex(E e)
{
    return static_cast<std::underlying_type_t<E>>(e);
}
