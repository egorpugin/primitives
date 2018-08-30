#pragma once

#include <boost/container_hash/hash.hpp>

template <class T>
inline size_t hash_combine(size_t &hash, const T &v)
{
    boost::hash_combine(hash, v);
    return hash;
}
