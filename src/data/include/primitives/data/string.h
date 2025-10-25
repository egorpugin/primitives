#pragma once

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

namespace primitives::data {

template<std::size_t N>
struct static_string {
    char p[N]{};

    constexpr static_string(char const(&pp)[N]) {
        std::ranges::copy(pp, p);
    }
    operator auto() const { return &p[0]; }
};

template<static_string s>
constexpr auto operator ""_s() { return s; }

}
