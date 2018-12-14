// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>

PRIMITIVES_PACK_API
bool pack_files(const path &archive, const std::unordered_map<path, path> &files, String *error = nullptr);

PRIMITIVES_PACK_API
bool pack_files(const path &archive, const Files &files, const path &root_dir, String *error = nullptr);

PRIMITIVES_PACK_API
Files unpack_file(const path &archive, const path &dest_dir);

namespace primitives::pack
{

PRIMITIVES_PACK_API
std::unordered_map<path, path> prepare_files(const Files &files, const path &root_dir, const path &dir_prefix = path(), const path &file_prefix = path());

}
