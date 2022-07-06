// Copyright (C) 2017-2018 Egor Pugin <egor.pugin@gmail.com>

#pragma once

#include <primitives/hash.h>

#include <openssl/evp.h>

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

password_hash generate_password(const String &in_password, const salt &salt, int password_len_bytes = 100) {
    static const auto N = 16384;
    static const auto r = 8;
    static const auto p = 2;
    password_hash key(password_len_bytes, std::byte{ 0 });
    if (EVP_PBE_scrypt(in_password.c_str(), in_password.size(), (unsigned char *)salt.data(), salt.size(),
        N, r, p, 0 /* maxmem  = 32 MB by default */, (unsigned char *)&key[0], key.size()))
    {
        return key;
    }
    throw SW_RUNTIME_ERROR("Error during password hash");
}

inline auto create_password_hash(const String &password, int salt_len_bytes = 100) {
    auto salt = generate_salt(salt_len_bytes);
    auto phash = generate_password(password, salt);
    return std::tuple{phash,salt};
}

inline bool check_password(const String &password_entered, const password_hash &phash, const salt &salt) {
    return generate_password(password_entered, salt) == phash;
}

std::tuple<bool, std::string> is_valid_password(const String &p){
    if (p.size() < 8)
        return { false, "Minimum password length is 8 symbols" };
    int lower = 0;
    int upper = 0;
    int digit = 0;
    int special = 0;
    static const auto spaces_error = "Spaces are not allowed in password";
    for (auto &c : p)
    {
        if (c <= 0 || c >= 127)
            return { false, spaces_error };
        else if (islower(c))
            lower++;
        else if (isupper(c))
            upper++;
        else if (isdigit(c))
            digit++;
        else if (isspace(c) || iscntrl(c))
            return { false, spaces_error };
        else if (isprint(c))
            special++;
        else
            return { false, spaces_error };
    }
    bool ok = (lower || upper) && digit;
    if (!ok)
        return { false, "Password should have at least one letter and one digit" };
    return { ok, "" };
}

} // namespace primitives::password
