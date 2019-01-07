// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/hash.h>

#include <primitives/file_iterator.h>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <rhash.h>

#include <random>

// keep always digits,lowercase,uppercase
static const char alnum[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

auto get_seed()
{
    return std::random_device()();
}

String generate_random_alnum_sequence(uint32_t len)
{
    std::mt19937 g(get_seed());
    std::uniform_int_distribution<> d(1, sizeof(alnum) - 1);
    String seq(len, 0);
    while (len)
        seq[--len] = alnum[d(g) - 1];
    return seq;
}

String generate_random_bytes(uint32_t len)
{
    std::mt19937 g(get_seed());
    std::uniform_int_distribution<> d(0, 255);
    String seq(len, 0);
    while (len)
        seq[--len] = d(g);
    return seq;
}

String generate_strong_random_bytes(uint32_t len)
{
    String bytes(len, 0);
    if (!RAND_bytes((unsigned char *)&bytes[0], (int)bytes.size()))
        throw SW_RUNTIME_ERROR("Error during getting random bytes");
    return bytes;
}

String bytes_to_string(const String &bytes)
{
    return bytes_to_string((uint8_t *)bytes.c_str(), bytes.size());
}

String bytes_to_string(const uint8_t *bytes, size_t size)
{
    String s;
    s.reserve(size * 2);
    for (uint32_t i = 0; i < size; i++)
    {
        s += alnum[(bytes[i] & 0xF0) >> 4];
        s += alnum[(bytes[i] & 0x0F) >> 0];
    }
    return s;
}

String shorten_hash(const String &data, size_t size)
{
    if (data.size() <= size)
        return data;
    return data.substr(0, size);
}

String sha1(const String &data)
{
    uint8_t hash[EVP_MAX_MD_SIZE];
    uint32_t hash_size;
    EVP_Digest(data.data(), data.size(), hash, &hash_size, EVP_sha1(), nullptr);
    return bytes_to_string(hash, hash_size);
}

String sha256(const String &data)
{
    uint8_t hash[EVP_MAX_MD_SIZE];
    uint32_t hash_size;
    EVP_Digest(data.data(), data.size(), hash, &hash_size, EVP_sha256(), nullptr);
    return bytes_to_string(hash, hash_size);
}

String blake2b_512(const String &data)
{
    uint8_t hash[EVP_MAX_MD_SIZE];
    uint32_t hash_size;
    EVP_Digest(data.data(), data.size(), hash, &hash_size, EVP_blake2b512(), nullptr);
    return bytes_to_string(hash, hash_size);
}

String sha256(const path &fn)
{
    return sha256(read_file(fn));
}

String sha3_256(const String &data)
{
    std::string o(32, 0);
    rhash_msg(RHASH_SHA3_256, &data[0], data.size(), (unsigned char *)&o[0]);
    return bytes_to_string(o);
}

String sha3_256(const path &fn)
{
    std::string o(32, 0);
    auto data = read_file(fn);
    rhash_msg(RHASH_SHA3_256, &data[0], data.size(), (unsigned char *)&o[0]);
    return bytes_to_string(o);
}

String md5(const String &data)
{
    uint8_t hash[EVP_MAX_MD_SIZE];
    uint32_t hash_size;
    EVP_Digest(data.data(), data.size(), hash, &hash_size, EVP_md5(), nullptr);
    return bytes_to_string(hash, hash_size);
}

String md5(const path &fn)
{
    return md5(read_file(fn));
}

std::tuple<HashType, HashType> load_strong_hash_prefix(const String &hash)
{
    auto p = hash.find('$');
    if (p == -1)
        return { HashType::sha3_256, HashType::sha2_256 };
    auto fht_s = hash.substr(0, hash.find('_'));
    auto sht_s = hash.substr(hash.find('_') + 1, p);
    auto fht = (HashType)std::stoi(fht_s);
    auto sht = (HashType)std::stoi(sht_s);
    return { fht,sht };
}

String save_strong_hash_prefix(HashType hash1, HashType hash2)
{
    return std::to_string((int)HashType::blake2b_512) + "_" + std::to_string((int)HashType::sha3_256) + "$";
}

String strong_file_hash(const String &data)
{
    // algorithm:
    //  sha3_256(sha2(f+sz) + sha3_256(f+sz) + sz)
    // sha2, sha3_256 - 256 bit versions

    auto sz = std::to_string(data.size());
    auto f = data + sz;
    return sha3_256(sha256(f) + sha3_256(f) + sz);
}

String strong_file_hash(const path &fn)
{
    // algorithm:
    //  sha3_256(sha2(f+sz) + sha3_256(f+sz) + sz)
    // sha2, sha3_256 - 256 bit versions

    uint8_t hash2[EVP_MAX_MD_SIZE];
    uint32_t hash2_size;

    std::string hash3(32, 0);

    auto sha2_256_ctx = EVP_MD_CTX_create();
    if (!sha2_256_ctx)
        throw SW_RUNTIME_ERROR("Cannot create sha2 context");
    EVP_DigestInit(sha2_256_ctx, EVP_sha256());

    auto sha3_256_ctx = rhash_init(RHASH_SHA3_256);
    if (!sha3_256_ctx)
        throw SW_RUNTIME_ERROR("Cannot create sha3 context");

    FileIterator fi({ fn });
    auto r = fi.iterate([sha2_256_ctx, sha3_256_ctx](const auto &b, auto sz)
    {
        EVP_DigestUpdate(sha2_256_ctx, b[0].get().data(), (size_t)sz);
        rhash_update(sha3_256_ctx, b[0].get().data(), (size_t)sz);
        return true;
    });

    // add size
    auto sz = std::to_string(fi.files.front().size);
    EVP_DigestUpdate(sha2_256_ctx, sz.data(), sz.size());
    rhash_update(sha3_256_ctx, sz.data(), sz.size());

    EVP_DigestFinal(sha2_256_ctx, hash2, &hash2_size);
    rhash_final(sha3_256_ctx, (unsigned char *)&hash3[0]);

    EVP_MD_CTX_destroy(sha2_256_ctx);
    rhash_free(sha3_256_ctx);

    if (!r)
        throw SW_RUNTIME_ERROR("Error during strong_file_hash()");

    auto p1 = bytes_to_string(hash2, hash2_size);
    auto p2 = bytes_to_string(hash3);
    auto &p3 = sz;
    return sha3_256(p1 + p2 + p3);
}

String strong_file_hash_sha3_sha2(const String &data)
{
    return strong_file_hash(data);
}

String strong_file_hash_sha3_sha2(const path &fn)
{
    return strong_file_hash(fn);
}

String strong_file_hash_blake2b_sha3(const String &data)
{
    // algorithm:
    //  blake2b_512(sha3_256(data+sz) + blake2b_512(data+sz) + sz)

    auto sz = std::to_string(data.size());
    auto f = data + sz;
    return save_strong_hash_prefix(HashType::blake2b_512, HashType::sha3_256) + blake2b_512(sha3_256(f) + blake2b_512(f) + sz);
}

String strong_file_hash_blake2b_sha3(const path &fn)
{
    // algorithm:
    //  blake2b_512(sha3_256(data+sz) + blake2b_512(data+sz) + sz)

    uint8_t blake2b[EVP_MAX_MD_SIZE];
    uint32_t blake2b_size;

    std::string sha3(32, 0);

    auto blake2b_ctx = EVP_MD_CTX_create();
    if (!blake2b)
        throw SW_RUNTIME_ERROR("Cannot create blake2b context");
    EVP_DigestInit(blake2b_ctx, EVP_blake2b512());

    auto sha3_256_ctx = rhash_init(RHASH_SHA3_256);
    if (!sha3_256_ctx)
        throw SW_RUNTIME_ERROR("Cannot create sha3 context");

    FileIterator fi({ fn });
    auto r = fi.iterate([blake2b_ctx, sha3_256_ctx](const auto &b, auto sz)
    {
        EVP_DigestUpdate(blake2b_ctx, b[0].get().data(), (size_t)sz);
        rhash_update(sha3_256_ctx, b[0].get().data(), (size_t)sz);
        return true;
    });

    // add size
    auto sz = std::to_string(fi.files.front().size);
    EVP_DigestUpdate(blake2b_ctx, sz.data(), sz.size());
    rhash_update(sha3_256_ctx, sz.data(), sz.size());

    EVP_DigestFinal(blake2b_ctx, blake2b, &blake2b_size);
    rhash_final(sha3_256_ctx, (unsigned char *)&sha3[0]);

    EVP_MD_CTX_destroy(blake2b_ctx);
    rhash_free(sha3_256_ctx);

    if (!r)
        throw SW_RUNTIME_ERROR("Error during strong_file_hash_blake2b_sha3()");

    auto p1 = bytes_to_string(sha3);
    auto p2 = bytes_to_string(blake2b, blake2b_size);
    auto &p3 = sz;
    return save_strong_hash_prefix(HashType::blake2b_512, HashType::sha3_256) + blake2b_512(p1 + p2 + p3);
}
