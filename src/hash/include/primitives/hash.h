// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>

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
String md5(const path &fn);

PRIMITIVES_HASH_API
String sha1(const String &data);

PRIMITIVES_HASH_API
String sha256(const String &data);

PRIMITIVES_HASH_API
String sha256(const path &fn);

PRIMITIVES_HASH_API
String sha3_256(const String &data);

PRIMITIVES_HASH_API
String sha3_256(const path &fn);

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
String strong_file_hash(const path &fn);

/// sha3_256(sha2(data+sz) + sha3_256(data+sz) + sz)
PRIMITIVES_HASH_API
String strong_file_hash_sha3_sha2(const String &data);

/// sha3_256(sha2(data+sz) + sha3_256(data+sz) + sz)
PRIMITIVES_HASH_API
String strong_file_hash_sha3_sha2(const path &fn);

/// blake2b_512(sha3_256(data+sz) + blake2b_512(data+sz) + sz)
PRIMITIVES_HASH_API
String strong_file_hash_blake2b_sha3(const String &data);

/// blake2b_512(sha3_256(data+sz) + blake2b_512(data+sz) + sz)
PRIMITIVES_HASH_API
String strong_file_hash_blake2b_sha3(const path &fn);

PRIMITIVES_HASH_API
String shorten_hash(const String &data, size_t size);

/// calculate strong_file_hash of data and compare with procided hash
/// returns true if equal
template <class T>
bool check_strong_file_hash(const T &data, const String &hash)
{
    auto[f, s] = load_strong_hash_prefix(hash);
    if (f == HashType::sha3_256 && s == HashType::sha2_256)
        return hash == strong_file_hash_sha3_sha2(data);
    if (f == HashType::blake2b_512 && s == HashType::sha3_256)
        return hash == strong_file_hash_blake2b_sha3(data);
    throw std::runtime_error("Unknown hash type(s)");
}
