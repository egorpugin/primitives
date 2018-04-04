// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>

String generate_random_alnum_sequence(uint32_t len);
String generate_random_bytes(uint32_t len);
String generate_strong_random_bytes(uint32_t len);

String bytes_to_string(const uint8_t *bytes, size_t size);
String bytes_to_string(const String &bytes);

String md5(const String &data);
String md5(const path &fn);

String sha1(const String &data);

String sha256(const String &data);
String sha256(const path &fn);

String sha3_256(const String &data);
String sha3_256(const path &fn);

String strong_file_hash(const String &data);
String strong_file_hash(const path &fn);

String shorten_hash(const String &data, size_t size);

template <class T>
inline size_t hash_combine(size_t &hash, const T &v)
{
    boost::hash_combine(hash, v);
    return hash;
}
