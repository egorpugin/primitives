#pragma once

#include <type_traits>

template <typename E>
constexpr std::underlying_type_t<E> toIndex(E e)
{
    return static_cast<std::underlying_type_t<E>>(e);
}

// bitmask operations
#define ENABLE_ENUM_CLASS_BITMASK(t)                                                                                   \
    [[nodiscard]] constexpr t operator&(t lhs, t rhs) noexcept                                                         \
    {                                                                                                                  \
        using UT = std::underlying_type_t<t>;                                                                          \
        return static_cast<t>(static_cast<UT>(lhs) & static_cast<UT>(rhs));                                            \
    }                                                                                                                  \
                                                                                                                       \
    [[nodiscard]] constexpr t operator|(t lhs, t rhs) noexcept                                                         \
    {                                                                                                                  \
        using UT = std::underlying_type_t<t>;                                                                          \
        return static_cast<t>(static_cast<UT>(lhs) | static_cast<UT>(rhs));                                            \
    }                                                                                                                  \
                                                                                                                       \
    [[nodiscard]] constexpr t operator^(t lhs, t rhs) noexcept                                                         \
    {                                                                                                                  \
        using UT = std::underlying_type_t<t>;                                                                          \
        return static_cast<t>(static_cast<UT>(lhs) ^ static_cast<UT>(rhs));                                            \
    }                                                                                                                  \
                                                                                                                       \
    constexpr t &operator&=(t &lhs, t rhs) noexcept                                                                    \
    {                                                                                                                  \
        return lhs = lhs & rhs;                                                                                        \
    }                                                                                                                  \
                                                                                                                       \
    constexpr t &operator|=(t &lhs, t rhs) noexcept                                                                    \
    {                                                                                                                  \
        return lhs = lhs | rhs;                                                                                        \
    }                                                                                                                  \
                                                                                                                       \
    constexpr t &operator^=(t &lhs, t rhs) noexcept                                                                    \
    {                                                                                                                  \
        return lhs = lhs ^ rhs;                                                                                        \
    }                                                                                                                  \
                                                                                                                       \
    [[nodiscard]] constexpr t operator~(t lhs) noexcept                                                                \
    {                                                                                                                  \
        using UT = std::underlying_type_t<t>;                                                                          \
        return static_cast<t>(~static_cast<UT>(lhs));                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    [[nodiscard]] constexpr bool bitmask_includes(t lhs, t rhs) noexcept                                               \
    {                                                                                                                  \
        return (lhs & rhs) != t{};                                                                                     \
    }                                                                                                                  \
                                                                                                                       \
    [[nodiscard]] constexpr bool bitmask_includes_all(t lhs, t rhs) noexcept                                           \
    {                                                                                                                  \
        return (lhs & rhs) == rhs;                                                                                     \
    }
