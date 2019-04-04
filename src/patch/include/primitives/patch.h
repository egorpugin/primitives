// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>

#include <optional>

namespace primitives::patch
{

enum PatchOption
{
    None,
    Git,
};

PRIMITIVES_PATCH_API
std::pair<bool, String> patch(const String &text, const String &unidiff, int options = 0);

PRIMITIVES_PATCH_API
bool patch(const path &root_dir, const String &unidiff, int options = 0);

}
