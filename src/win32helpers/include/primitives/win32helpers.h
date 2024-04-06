// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>

PRIMITIVES_WIN32HELPERS_API
bool create_link(const path &file, const path &link, const String &description = String());

PRIMITIVES_WIN32HELPERS_API
void elevate();

PRIMITIVES_WIN32HELPERS_API
bool is_elevated();

#ifdef _WIN32
using _DWORD = unsigned long;

PRIMITIVES_WIN32HELPERS_API
std::string get_last_error();

PRIMITIVES_WIN32HELPERS_API
std::string get_last_error(_DWORD ec);

PRIMITIVES_WIN32HELPERS_API
void message_box(const std::string &caption, const std::string &s);

PRIMITIVES_WIN32HELPERS_API
void message_box(const std::string &caption, const std::wstring &s);

PRIMITIVES_WIN32HELPERS_API
void message_box(const std::string &s);

PRIMITIVES_WIN32HELPERS_API
void message_box(const std::wstring &s);

PRIMITIVES_WIN32HELPERS_API
void BindStdHandlesToConsole();

PRIMITIVES_WIN32HELPERS_API
void SetupConsole();

struct ServiceDescriptor
{
    String name;
    //String display_name;
    path module;
    String args;
};

PRIMITIVES_WIN32HELPERS_API
_DWORD InstallService(ServiceDescriptor desc);

struct VSInstanceInfo
{
  std::wstring InstanceId;
  std::wstring VSInstallLocation;
  std::wstring Version;
  ULONGLONG ullVersion{};
  bool IsWin10SDKInstalled{};
  bool IsWin81SDKInstalled{};
};

PRIMITIVES_WIN32HELPERS_API
std::vector<VSInstanceInfo> EnumerateVSInstances();

#endif
