#pragma once

#include "string.h"

#include <array>

// https://www.rfc-editor.org/rfc/rfc4648
// also base 32 and base16

template <auto Alphabet, auto padding = '='>
struct base64_raw {
    static_assert(Alphabet.size() == 64);

    using u8 = unsigned char;

    static consteval auto make_decoder() {
        std::array<u8, 128> alph{};
        for (int i = 0; auto &&c : Alphabet) {
            alph[c] = i++;
        }
        return alph;
    }
    static inline constexpr auto DecodeAlphabet = make_decoder();

    struct b64 {
        u8 b2 : 2;
        u8 a  : 6;
        u8 c1 : 4;
        u8 b1 : 4;
        u8 d  : 6;
        u8 c2 : 2;

        template <auto N> constexpr void extract(auto &s) {
            s[0] = Alphabet[a];
            s[1] = Alphabet[b1 + (b2 << 4)];
            if constexpr (N > 2)
            s[2] = Alphabet[(c1 << 2) + c2];
            else
            s[2] = padding;
            if constexpr (N > 3)
            s[3] = Alphabet[d];
            else
            s[3] = padding;
        }
        template <auto N> constexpr void assign(auto data) {
            auto &alph = DecodeAlphabet;
            b2 = alph[data[1]] >> 4;
            a  = alph[data[0]];
            if constexpr (N > 2)
            c1 = alph[data[2]] >> 2;
            b1 = alph[data[1]];
            if constexpr (N > 3)
            d  = alph[data[3]];
            if constexpr (N > 2)
            c2 = alph[data[2]];
        }
    };
    static inline constexpr auto b64size = 3;
    static inline constexpr auto b64chars = 4;
    static_assert(sizeof(b64) == b64size);

    static auto encode(auto &&data) {
        auto sz = data.size();
        std::string s;
        if (sz == 0) {
            return s;
        }
        s.resize((sz / b64size + (sz % b64size ? 1 : 0)) * b64chars);
        auto out = s.data();
        auto until = sz - sz % b64size;
        auto p = (b64*)data.data();
        int i{};
        for (; i < until; i += b64size, out += 4) {
            p++->template extract<b64chars>(out);
        }
        auto tail = sz - i;
        if (tail == 1) {
            p->template extract<b64chars-2>(out);
        } else if (tail == 2) {
            p->template extract<b64chars-1>(out);
        }
        return s;
    }
    static auto decode(auto &&data) {
        auto sz = data.size();
        if (sz % b64chars) {
            throw std::runtime_error{"bad base64: incorrect length"};
        }
        std::string s;
        if (sz == 0) {
            return s;
        }
        s.resize(sz / b64chars * b64size);
        auto p = (b64*)s.data();
        for (int i = 0; i < sz; i += b64chars) {
            p++->template assign<b64chars>(&data[i]);
        }
        int tail{};
        tail += data[sz-1] == padding;
        tail += data[sz-2] == padding;
        s.resize(s.size() - tail);
        return s;
    }
};
struct base64    : base64_raw<"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"_s> {};
struct base64url : base64_raw<"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"_s> {};

inline std::string operator""_b64e(const char *s, size_t len) {
    return base64::encode(std::string_view{s,len});
}
inline std::string operator""_b64d(const char *s, size_t len) {
    return base64::decode(std::string_view{s,len});
}

/*
    auto x1 = base64::encode("Many hands make light work."s);
    auto x2 = base64::encode("Many hands make light work.."s);
    auto x3 = base64::encode("Many hands make light work..."s);
    auto x4 = "Many hands make light work."_b64e;

    auto y1 = base64::decode(x1);
    auto y2 = base64::decode(x2);
    auto y3 = base64::decode(x3);
    auto y4 = "TWFueSBoYW5kcyBtYWtlIGxpZ2h0IHdvcmsu"_b64d;
*/
