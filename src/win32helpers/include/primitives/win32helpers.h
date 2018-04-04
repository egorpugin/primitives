// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>

bool create_link(const path &file, const path &link, const String &description = String());
void elevate();
bool is_elevated();

#ifdef _WIN32
using _DWORD = unsigned long;

std::string get_last_error();
std::string get_last_error(_DWORD ec);

void message_box(const std::string &s);
void message_box(const std::wstring &s);

void BindStdHandlesToConsole();
void SetupConsole();

struct ServiceDescriptor
{
    String name;
    //String display_name;
    path module;
    String args;
};

_DWORD InstallService(ServiceDescriptor desc);
#endif
