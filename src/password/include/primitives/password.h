// Copyright (C) 2017-2018 Egor Pugin <egor.pugin@gmail.com>

#pragma once

#include <primitives/hash.h>

namespace primitives::password {

using primitives::hash::bytes;

struct salt : bytes {
    using bytes::bytes;
    salt(bytes &&b) : bytes(std::move(b)) {}
};

struct password_hash : bytes {
    using bytes::bytes;
    password_hash(bytes &&b) : bytes(std::move(b)) {}
};

inline salt generate_salt(int len) {
    return primitives::hash::generate_strong_random_bytes(len);
}

PRIMITIVES_PASSWORD_API
password_hash generate_password(const String &password, const salt &, int password_len_bytes = 100);

inline auto create_password_hash(const String &password, int salt_len_bytes = 100) {
    auto salt = generate_salt(salt_len_bytes);
    auto phash = generate_password(password, salt);
    return std::tuple{phash,salt};
}

inline bool check_password(const String &password_entered, const password_hash &phash, const salt &salt) {
    return generate_password(password_entered, salt) == phash;
}

PRIMITIVES_PASSWORD_API
std::tuple<bool, std::string> is_valid_password(const String &p);

}
