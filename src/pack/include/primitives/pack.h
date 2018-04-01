#pragma once

#include <primitives/filesystem.h>

bool pack_files(const path &archive, const std::unordered_map<path, path> &files);
bool pack_files(const path &archive, const Files &files, const path &root_dir);
Files unpack_file(const path &archive, const path &dest_dir);

namespace primitives::pack
{

std::unordered_map<path, path> prepare_files(const Files &files, const path &root_dir, const path &dir_prefix = path(), const path &file_prefix = path());

}
