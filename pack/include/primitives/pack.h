#pragma once

#include <primitives/filesystem.h>

bool pack_files(const path &fn, const Files &files, const path &root_dir);
Files unpack_file(const path &fn, const path &dst);
