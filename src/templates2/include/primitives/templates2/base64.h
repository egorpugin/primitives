#pragma once

#include "string.h"

#include <array>
#include <bit>
#include <numeric>

// https://www.rfc-editor.org/rfc/rfc4648
// also base 32 and base16

template <auto Nchars, auto Alphabet, auto padding = '='>
struct base_raw {
    static_assert(Alphabet.size() == Nchars);

    using u8 = unsigned char;
    static inline constexpr auto byte_bits = CHAR_BIT;
    static_assert(byte_bits == 8);
    static inline constexpr u8 base_type = Nchars;
    static inline constexpr auto n_bits = std::countr_zero(std::bit_ceil(base_type));

    static inline constexpr auto lcm = std::lcm(byte_bits, n_bits);
    static inline constexpr auto b_size = lcm / byte_bits;
    static inline constexpr auto b_chars = lcm / n_bits;
    static inline constexpr auto max_tail = b_chars - (byte_bits / n_bits + (byte_bits % n_bits ? 1 : 0));

    static consteval auto make_decoder() {
        std::array<u8, 128> alph{};
        for (int i = 0; auto &&c : Alphabet) {
            alph[c] = i++;
        }
        return alph;
    }
    static inline constexpr auto DecodeAlphabet = make_decoder();

    static auto name() {return std::format("base{}", Nchars);}

    struct b {
        // from
        // 76543210 76543210 76543210 ...
        // to         1          2
        // 01234567 89012345 67890123 ...
        template <auto start> u8 get_bits() {
            auto base = (u8*)this;
            constexpr auto b1 = start / byte_bits;
            constexpr auto b2 = (start + n_bits - 1) / byte_bits;
            if (b1 == b2) {
                constexpr auto offset = byte_bits - start % byte_bits;
                return (base[b1] >> (offset - n_bits)) & ((1 << n_bits) - 1);
            } else {
                constexpr auto bits1 = byte_bits - start % byte_bits;
                constexpr auto bits2 = n_bits - bits1;
                auto l = base[b1] & ((1 << bits1) - 1);
                auto r = base[b2] >> (byte_bits - bits2);
                return (l << bits2) | r;
            }
        }
        template <auto start> void set_bits(u8 value) {
            auto base = (u8*)this;
            constexpr auto b1 = start / byte_bits;
            constexpr auto b2 = (start + n_bits) / byte_bits;
            if (b1 == b2) {
                constexpr auto bits1 = byte_bits - start % byte_bits - n_bits;
                base[b1] |= value << bits1;
            } else {
                constexpr auto bits1 = byte_bits - start % byte_bits;
                constexpr auto bits2 = n_bits - bits1;
                base[b1] |= value >> bits2;
                base[b2] |= value << (byte_bits - bits2);
            }
        }

        // b_size -> b_chars bytes
        template <auto N> constexpr void encode(auto &s) {
#define X(v)                                                \
            if constexpr (N > v)                            \
                s[v] = Alphabet[get_bits<n_bits * v>()];    \
            else if constexpr (v < b_chars)                 \
                s[v] = padding

            X(0);
            X(1);
            X(2);
            X(3);
            X(4);
            X(5);
            X(6);
            X(7);
#undef X
        }
        // b_chars -> b_size bytes
        template <auto N> constexpr void decode(auto data) {
#define X(v) if constexpr (N > v) set_bits<n_bits * v>(DecodeAlphabet[data[v]])
            X(0);
            X(1);
            X(2);
            X(3);
            X(4);
            X(5);
            X(6);
            X(7);
#undef X
        }
    };

    static auto encode(auto &&data) {
        auto sz = data.size();
        std::string s;
        if (sz == 0) {
            return s;
        }
        s.resize((sz / b_size + (sz % b_size ? 1 : 0)) * b_chars);
        auto out = s.data();
        auto until = sz - sz % b_size;
        auto p = (b*)data.data();
        int i{};
        for (; i < until; i += b_size, out += b_chars, p += b_size) {
            p->template encode<b_chars>(out);
        }
        if constexpr (max_tail) {
            auto tail = sz - i;
            auto tailbits = tail * byte_bits;
            auto tailsize = tailbits / n_bits + (tailbits % n_bits ? 1 : 0);
            if (false) {
            } else if (tailsize == 0) {
            } else if (tailsize == 1) {
                p->template encode<1>(out);
            } else if (tailsize == 2) {
                p->template encode<2>(out);
            } else if (tailsize == 3) {
                p->template encode<3>(out);
            } else if (tailsize == 4) {
                p->template encode<4>(out);
            } else if (tailsize == 5) {
                p->template encode<5>(out);
            } else if (tailsize == 6) {
                p->template encode<6>(out);
            } else if (tailsize == 7) {
                p->template encode<7>(out);
            } else {
                throw std::logic_error{"unimplemented"};
            }
        }
        return s;
    }
    static auto decode(auto &&data) {
        auto sz = data.size();
        if (sz % b_chars) {
            throw std::runtime_error{std::format("bad {}: incorrect length", name())};
        }
        std::string s;
        if (sz == 0) {
            return s;
        }
        s.resize(sz / b_chars * b_size);
        auto p = (b*)s.data();
        for (int i = 0; i < sz; i += b_chars, p += b_size) {
            p->template decode<b_chars>(&data[i]);
        }
        if constexpr (max_tail) {
            int tailsize{};
            auto t = max_tail;
            while (t--) {
                if (data[--sz] == padding) {
                    ++tailsize;
                } else {
                    break;
                }
            }
            auto tailbits = tailsize * n_bits;
            auto tail = tailbits / byte_bits + (tailbits % byte_bits ? 1 : 0);
            s.resize(s.size() - tail);
        }
        return s;
    }
};
struct base16    : base_raw<16, "0123456789ABCDEF"_s> {};
struct base32    : base_raw<32, "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"_s> {};
struct base32extended_hex
                 : base_raw<32, "0123456789ABCDEFGHIJKLMNOPQRSTUV"_s> {};
struct base62    : base_raw<62, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"_s> {};
struct base64    : base_raw<64, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"_s> {};
struct base64url : base_raw<64, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"_s> {};
// uses variable sym length (6-7 bits), not suitable for general base-algorithm
//struct ascii85   : base_raw<85, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&()*+-;<=>?@^_`{|}~"_s> {};

/*
inline std::string operator""_b64e(const char *s, size_t len) {
    return base64::encode(std::string_view{s,len});
}
inline std::string operator""_b64d(const char *s, size_t len) {
    return base64::decode(std::string_view{s,len});
}

auto x1 = base64::encode("Many hands make light work."s);
auto x2 = base64::encode("Many hands make light work.."s);
auto x3 = base64::encode("Many hands make light work..."s);
auto x4 = "Many hands make light work."_b64e;

auto y1 = base64::decode(x1);
auto y2 = base64::decode(x2);
auto y3 = base64::decode(x3);
auto y4 = "TWFueSBoYW5kcyBtYWtlIGxpZ2h0IHdvcmsu"_b64d;

auto base64_test() {
    auto f = [](auto c, auto text) {
        auto e = c.encode(text);
        auto d = c.decode(e);
        if (text != d) {
            std::cerr << std::format("error: {}\n{}\n{}\n{}\n", c.name(), text, e, d);
        }
    };
    auto f2 = [&](auto c) {
        f(c, ""s);
        f(c, "x"s);
        f(c, "xx"s);
        f(c, "xxx"s);
        f(c, "xxxx"s);
        f(c, "xxxxx"s);
        f(c, "xxxxxx"s);
        f(c, "xxxxxxx"s);
        f(c, "xxxxxxxx"s);
        f(c, "xxxxxxxxx"s);
        f(c, "Many hands make light work."s);
        f(c, "Many hands make light work.."s);
        f(c, "Many hands make light work..."s);
        f(c, "Many hands make light work...."s);
        f(c, "Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable generation of knowledge, exceeds the short vehemence of any carnal pleasure."s);
    };

    f2(base64{});
    f2(base32{});
    f2(base16{});
    f2(base62{});
}

*/
