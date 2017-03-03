#pragma once

#include <primitives/filesystem.h>

String generate_random_sequence(uint32_t len);
String hash_to_string(const uint8_t *hash, size_t hash_size);
String hash_to_string(const String &hash);

String md5(const String &data);
String md5(const path &fn);
String sha1(const String &data);
String sha256(const String &data);
String sha256(const path &fn);
String sha3_256(const String &data);

String strong_file_hash(const path &fn);

String shorten_hash(const String &data, size_t size);

