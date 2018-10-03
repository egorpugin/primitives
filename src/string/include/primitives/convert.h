// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "string.h"

namespace primitives
{

template <class T>
T from_string(const std::string &v)
{
    return v;
}

template <>
inline int from_string(const std::string &v)
{
    return std::stoi(v);
}

template <>
inline int64_t from_string(const std::string &v)
{
    return std::stoll(v);
}

}
