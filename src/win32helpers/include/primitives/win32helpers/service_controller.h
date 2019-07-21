// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <string>

#include <windows.h>

struct PRIMITIVES_WIN32HELPERS_API ServiceController
{
    ServiceController();
    ~ServiceController();

    void install(const std::string &name, const std::string &display_name, const std::string &command_line);
    bool remove(const std::string &name);

    bool isInstalled(const std::string &name);
    bool isRunning(const std::string &name) const;

    bool start(const std::string &name);
    bool stop(const std::string &name);

private:
    SC_HANDLE manager = nullptr;
};

PRIMITIVES_WIN32HELPERS_API
void installService(const std::string &name, const std::string &display_name, const std::string &command_line);

PRIMITIVES_WIN32HELPERS_API
void removeService(const std::string &name);
