// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Some parts of this code are taken from LLVM.Support.CommandLine.

#pragma once

#include <primitives/settings_version_string.h>

namespace sw
{

PRIMITIVES_SW_SETTINGS_API
std::string getProgramName();

PRIMITIVES_SW_SETTINGS_API
std::string getProgramNameSilent();

}
