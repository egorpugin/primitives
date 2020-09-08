// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/hasher.h>

#include <primitives/hash.h>

#include <boost/algorithm/string.hpp>

#include <algorithm>

#define DEFINE_OPERATOR(t)        \
    Hasher Hasher::operator|(t v) \
    {                             \
        auto tmp = *this;         \
        tmp |= v;                 \
        return tmp;               \
    }

DEFINE_OPERATOR(bool)
DEFINE_OPERATOR(const String &)
DEFINE_OPERATOR(const path &);

void Hasher::do_hash()
{
    hash = sha256(hash);
}

Hasher &Hasher::operator|=(bool b)
{
    hash += b ? "1" : "0";
    do_hash();
    return *this;
}

Hasher &Hasher::operator|=(const String &v)
{
    auto s = v;
    boost::trim(s);

    std::vector<String> out;
    boost::split(out, s, boost::is_any_of(" \t\r\n\v\f"));

    std::vector<String> out2;
    for (auto &o : out)
    {
        boost::trim(o);
        if (o.empty())
            continue;
        out2.push_back(o);
    }

    std::sort(out2.begin(), out2.end());

    for (auto &o : out2)
        hash += o;

    do_hash();
    return *this;
}

Hasher &Hasher::operator|=(const path &v)
{
    // will different utf8 paths have same std::string repr?
    hash += to_printable_string(v);
    do_hash();
    return *this;
}
