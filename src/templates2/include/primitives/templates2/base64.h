// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2022-2025 Egor Pugin <egor.pugin@gmail.com>

#pragma once

#include "string.h"

#include <numeric>
#include <string>

using namespace std::literals;

// https://www.rfc-editor.org/rfc/rfc4648
// also base 32 and base16

//namespace crypto {

template <auto Nchars, auto Alphabet, auto padding = '=', bool Pad = true>
struct base_raw {
    static_assert(Alphabet.size() == Nchars);

    static constexpr auto divceil(auto &&x, auto &&y) {
        return (x + y - 1) / y;
    }

    using u8 = unsigned char;
    static inline constexpr auto byte_bits = CHAR_BIT;
    static_assert(byte_bits == 8);
    static inline constexpr u8 base_type = Nchars;
    static inline constexpr auto n_bits = std::countr_zero(std::bit_ceil(base_type));

    static inline constexpr auto lcm = std::lcm(byte_bits, n_bits);
    static inline constexpr auto decoded_block_size = lcm / byte_bits;
    static inline constexpr auto encoded_block_size = lcm / n_bits;
    static inline constexpr auto can_have_tail = lcm == encoded_block_size;
    static inline constexpr auto max_tail = encoded_block_size - divceil(byte_bits, n_bits);

    static consteval auto make_decoder() {
        std::array<uint8_t, 128> alph{};
        for (int i = 0; auto &&c : Alphabet) {
            alph[c] = i++;
        }
        return alph;
    }
    static inline constexpr auto DecodeAlphabet = make_decoder();

    static auto name() {return std::format("base{}", Nchars);}
    static auto encoded_size(auto decoded_size) {
        return divceil(decoded_size, decoded_block_size) * encoded_block_size;
    }
    static auto decoded_size(auto encoded_size) {
        return divceil(encoded_size, encoded_block_size) * decoded_block_size;
    }

    template <bool Encode>
    struct b2 {
        static inline constexpr auto data_size = Encode ? decoded_block_size : encoded_block_size;
        static inline constexpr auto output_size = Encode ? encoded_block_size : decoded_block_size;

        u8 data[data_size];
        u8 sz{};

        void add(auto &&c, auto &p) {
            data[sz++ % data_size] = c;
            if (sz % data_size == 0) {
                finish(p);
            }
        }
        void finish(auto &p) {
            if (sz == 0) {
                return;
            }
            auto real_data_size = sz % data_size;
            if (real_data_size == 0) {
                real_data_size = data_size;
            }
            while (sz % data_size != 0) {
                data[sz++ % data_size] = Encode ? 0 : padding;
            }
            sz = 0;
            if constexpr (Encode) {
                int i = 0;
                for (; i < real_data_size * byte_bits; i += n_bits) {
                    *p++ = Alphabet[get_bits(i)];
                }
                auto p2 = p; // keep output pointer on before padding
                for (; i < data_size * byte_bits; i += n_bits) {
                    *p2++ = padding;
                }
            } else {
                auto outsz = output_size;
                for (int i = 0, start = 0; i < data_size; ++i, start += n_bits) {
                    if (data[i] == padding) {
                        --outsz;
                    }
                    set_bits(DecodeAlphabet[data[i]], start, p);
                }
                p += outsz;
            }
        }

        // from
        // 76543210 76543210 76543210 ...
        // to         1          2
        // 01234567 89012345 67890123 ...
        u8 get_bits(auto &&start) const {
            auto b1 = start / byte_bits;
            auto b2 = (start + n_bits - 1) / byte_bits;
            if (b1 == b2) {
                auto offset = byte_bits - start % byte_bits;
                return (data[b1] >> (offset - n_bits)) & ((1 << n_bits) - 1);
            } else {
                auto bits1 = byte_bits - start % byte_bits;
                auto bits2 = n_bits - bits1;
                auto l = data[b1] & ((1 << bits1) - 1);
                auto r = data[b2] >> (byte_bits - bits2);
                return (l << bits2) | r;
            }
        }
        static void set_bits(u8 value, auto &&start, auto &p) {
            auto b1 = start / byte_bits;
            auto b2 = (start + n_bits) / byte_bits;
            if (b1 == b2) {
                auto bits1 = byte_bits - start % byte_bits - n_bits;
                p[b1] |= value << bits1;
            } else {
                auto bits1 = byte_bits - start % byte_bits;
                auto bits2 = n_bits - bits1;
                p[b1] |= value >> bits2;
                p[b2] |= value << (byte_bits - bits2);
            }
        }
    };

    static auto encode(auto &&data) {
        auto sz = data.size();
        std::string out;
        if (sz == 0) {
            return out;
        }
        out.resize(encoded_size(sz));
        auto p = out.data();
        b2<true> conv;
        for (auto &&c : data) {
            conv.add(c, p);
        }
        conv.finish(p);
        if constexpr (!Pad) {
            out.resize(p - out.data());
        }
        return out;
    }
    template <bool IgnoreNonAlphabetChars>
    static auto decode(auto &&data) {
        auto sz = data.size();
        if ((sz % encoded_block_size) && Pad && !IgnoreNonAlphabetChars) {
            throw std::runtime_error{std::format("bad {}: incorrect length", name())};
        }
        std::string out;
        if (sz == 0) {
            return out;
        }
        out.resize(decoded_size(sz));
        std::string_view alph = Alphabet;
        auto p = out.data();
        b2<false> conv;
        for (auto &&c : data) {
            if constexpr (IgnoreNonAlphabetChars) {
                if (alph.contains(c) || c == padding) {
                    conv.add(c, p);
                }
            } else {
                // check for symbol and throw exception?
                conv.add(c, p);
            }
        }
        conv.finish(p);
        out.resize(p - out.data());
        return out;
    }
    static auto decode(auto &&data) {
        return decode<false>(data);
    }
};
struct base16    : base_raw<16, "0123456789ABCDEF"_s> {};
struct base32    : base_raw<32, "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"_s> {};
struct base32extended_hex
                 : base_raw<32, "0123456789ABCDEFGHIJKLMNOPQRSTUV"_s> {};
struct base62    : base_raw<62, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"_s> {};
//struct base58    : base_raw<58, "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"_s> {};
struct base64    : base_raw<64, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"_s> {};
template <bool Pad = false>
struct base64url : base_raw<64, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"_s, '=', Pad> {};
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

//} // namespace crypto
