// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <string>
#include <vector>

struct PRIMITIVES_WIN32HELPERS_API Service
{
    virtual ~Service() = 0;

    virtual std::string getName() const = 0;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void reportError(const std::string &) const;
};

PRIMITIVES_WIN32HELPERS_API
void runServices(const std::vector<Service*> &);
