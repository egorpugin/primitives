#include <primitives/hash.h>

#include <random>

#include <openssl/evp.h>
#include <openssl/rand.h>
extern "C" {
#include <keccak-tiny.h>
}

// keep always digits,lowercase,uppercase
static const char alnum[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

String generate_random_sequence(uint32_t len)
{
    auto seed = std::random_device()();
    std::mt19937 g(seed);
    std::uniform_int_distribution<> d(1, sizeof(alnum) - 1);
    String seq(len, 0);
    while (len)
        seq[--len] = alnum[d(g) - 1];
    return seq;
}

String generate_strong_random_sequence(uint32_t len)
{
    String seq(len, 0);
    if (len == 0)
        return seq;
    String bytes(len / 2 + len % 2, 0);
    if (RAND_bytes((unsigned char *)&bytes[0], bytes.size()) != 1)
        throw std::runtime_error("Not enough entropy!");
    for (size_t i = 0; i < bytes.size(); i++)
    {
        seq[i * 2 + 0] = alnum[(bytes[i] & 0xF0) >> 4];
        seq[i * 2 + 1] = alnum[(bytes[i] & 0x0F) >> 0];
    }
    seq[seq.size()] = 0;
    return seq;
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

String sha3_256(const String &data)
{
    auto len = 256 / 8;
    std::string o(len, 0);
    sha3_256((uint8_t *)&o[0], len, (const uint8_t *)&data[0], data.size());
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
    return md5(read_file(fn, true));
}

String sha256(const path &fn)
{
    return sha256(read_file(fn, true));
}

String strong_file_hash(const path &fn)
{
    // algorithm:
    //  sha3(sha2(f+sz) + sha3(f+sz) + sz)
    // sha2, sha3 - 256 bit versions

    // TODO: switch to stream api when such sha3 alg will be available
    auto sz = std::to_string(fs::file_size(fn));
    auto f = read_file(fn, true) + sz;
    return sha3_256(sha256(f) + sha3_256(f) + sz);
}
