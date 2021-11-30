#pragma once

#include <boost/hana.hpp>

#include <string>

#define TEXT_FIELD(x)                                                                                                  \
    struct x##_ {                                                                                                      \
        char x[64];                                                                                                    \
        constexpr x##_(const char *n) : x() {                                                                          \
            for (int i = 0; *n; i++)                                                                                   \
                x[i] = *n++;                                                                                           \
        }                                                                                                              \
        constexpr auto operator<=>(const x##_ &) const = default;                                                      \
        operator std::string() const { return &x[0]; }                                                                 \
    };                                                                                                                 \
    template <x##_ v>                                                                                                  \
    constexpr x##_ operator""_##x() {                                                                                  \
        return v;                                                                                                      \
    }

namespace primitives::data::schema {

} // namespace primitives::data::schema
