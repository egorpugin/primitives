// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>

struct Hasher
{
    String hash;

#define ADD_OPERATOR(t)  \
    Hasher operator|(t); \
    Hasher &operator|=(t)

    ADD_OPERATOR(bool);
    ADD_OPERATOR(const String &);
    ADD_OPERATOR(const path &);

#undef ADD_OPERATOR

private:
    void do_hash();
};
