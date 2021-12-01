// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>
#include <primitives/exceptions.h>

#include <cstring>

PRIMITIVES_HASH_API
String generate_random_alnum_sequence(uint32_t len);

PRIMITIVES_HASH_API
String generate_random_bytes(uint32_t len);

PRIMITIVES_HASH_API
String generate_strong_random_bytes(uint32_t len);

PRIMITIVES_HASH_API
String bytes_to_string(const uint8_t *bytes, size_t size);

PRIMITIVES_HASH_API
String bytes_to_string(const String &bytes);

PRIMITIVES_HASH_API
String md5(const String &data);

PRIMITIVES_HASH_API
String md5_file(const path &fn);

PRIMITIVES_HASH_API
String sha1(const String &data);

PRIMITIVES_HASH_API
String sha256(const String &data);

PRIMITIVES_HASH_API
String sha256_file(const path &fn);

PRIMITIVES_HASH_API
String sha3_256(const String &data);

PRIMITIVES_HASH_API
String sha3_256_file(const path &fn);

PRIMITIVES_HASH_API
String blake2b_512(const String &data);

enum class HashType
{
    null            =   0,
    sha2_256        =   1,
    sha3_256        =   2,
    blake2b_512     =   3,
    max,
};

PRIMITIVES_HASH_API
std::tuple<HashType, HashType> load_strong_hash_prefix(const String &hash);

PRIMITIVES_HASH_API
String save_strong_hash_prefix(HashType hash1, HashType hash2);

/// sha3_256(sha2(data+sz) + sha3_256(data+sz) + sz)
PRIMITIVES_HASH_API
String strong_file_hash(const String &data);

/// sha3_256(sha2(data+sz) + sha3_256(data+sz) + sz)
PRIMITIVES_HASH_API
String strong_file_hash_file(const path &fn);

/// sha3_256(sha2(data+sz) + sha3_256(data+sz) + sz)
PRIMITIVES_HASH_API
String strong_file_hash_sha3_sha2(const String &data);

/// sha3_256(sha2(data+sz) + sha3_256(data+sz) + sz)
PRIMITIVES_HASH_API
String strong_file_hash_file_sha3_sha2(const path &fn);

/// blake2b_512(sha3_256(data+sz) + blake2b_512(data+sz) + sz)
PRIMITIVES_HASH_API
String strong_file_hash_blake2b_sha3(const String &data);

/// blake2b_512(sha3_256(data+sz) + blake2b_512(data+sz) + sz)
PRIMITIVES_HASH_API
String strong_file_hash_file_blake2b_sha3(const path &fn);

PRIMITIVES_HASH_API
String shorten_hash(const String &data, size_t size);

/// calculate and return strong_file_hash of data using type from hash
inline String get_strong_file_hash(const path &data, const String &hash)
{
    auto[f, s] = load_strong_hash_prefix(hash);
    if (f == HashType::sha3_256 && s == HashType::sha2_256)
        return strong_file_hash_file_sha3_sha2(data);
    if (f == HashType::blake2b_512 && s == HashType::sha3_256)
        return strong_file_hash_file_blake2b_sha3(data);
    throw SW_RUNTIME_ERROR("Unknown hash type(s)");
}

/// calculate strong_file_hash of data and compare with procided hash
/// returns true if equal
inline bool check_strong_file_hash(const path &data, const String &hash)
{
    auto[f, s] = load_strong_hash_prefix(hash);
    if (f == HashType::sha3_256 && s == HashType::sha2_256)
        return hash == strong_file_hash_file_sha3_sha2(data);
    if (f == HashType::blake2b_512 && s == HashType::sha3_256)
        return hash == strong_file_hash_file_blake2b_sha3(data);
    throw SW_RUNTIME_ERROR("Unknown hash type(s)");
}

namespace primitives::hash {

struct bytes : std::vector<std::byte> {
    using base = std::vector<std::byte>;
    using base::base;

    bytes(const std::vector<uint8_t> &v) : base((std::byte *)v.data(), (std::byte *)v.data() + v.size()) {}
    /*operator const std::vector<uint8_t> &() const {
        return (const std::vector<uint8_t> &)(*this);
    }*/
    operator std::vector<uint8_t>() const {
        std::vector<uint8_t> v(size());
        std::memcpy(v.data(), data(), size());
        return v;
    }

    bytes(const std::string &v) {
        auto sz = v.size();
        resize(sz / 2);
        if (sz % 2 != 0)
            throw SW_RUNTIME_ERROR("bad bytes string");
        for (int i = 0; i < sz; i += 2) {
            auto get_num = [](auto v) {
                if (v > '9')
                    return v - 'a' + 10;
                return v - '0';
            };
            auto hi = get_num(v[i+0]) << 4;
            auto lo = get_num(v[i+1]) << 0;
            operator[](i / 2) = std::byte(hi | lo);
        }
    }
    operator std::string() const { return bytes_to_string((const uint8_t*)data(), size()); }
};

PRIMITIVES_HASH_API
bytes generate_strong_random_bytes(uint32_t len);

template <size_t sz, auto = []{}>
struct bytes_strict : bytes {
    static_assert(sz > 0);

    bytes_strict() : bytes(generate_strong_random_bytes(sz)) {}
    bytes_strict(const std::vector<uint8_t> &v) : bytes(v) {
        if (size() != sz)
            throw SW_LOGIC_ERROR("bad bytes size, must be " + std::to_string(sz) + ", got " + std::to_string(size()));
    }
    bytes_strict(const std::string &v) : bytes(v) {
        if (size() != sz)
            throw SW_LOGIC_ERROR("bad bytes size, must be " + std::to_string(sz) + ", got " + std::to_string(size()));
    }
};

}
