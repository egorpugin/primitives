// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/pack.h>

#include <primitives/templates.h>

#include <libarchive/archive.h>
#include <libarchive/archive_entry.h>

#if !defined(_WIN32) && !defined(__APPLE__)
#include <linux/limits.h>
#endif

#define BLOCK_SIZE 8192

namespace primitives::pack
{

std::unordered_map<path, path> prepare_files(const Files &files, const path &root_dir, const path &dir_prefix, const path &file_prefix)
{
    std::unordered_map<path, path> files2;
    for (auto &f : files)
    {
        if (!fs::exists(f))
        {
            files2[f]; // still add to return false later
            continue;
        }

        // skip symlinks too
        if (!fs::is_regular_file(f))
            continue;

        auto r = fs::relative(f, root_dir);
        if (!file_prefix.empty())
            r = file_prefix.string() + r.string();
        if (!dir_prefix.empty())
            r = dir_prefix / r;
        files2[f] = r;
    }
    return files2;
}

}

bool pack_files(const path &fn, const std::unordered_map<path, path> &files)
{
    bool result = true;
    auto a = archive_write_new();

    SCOPE_EXIT
    {
        archive_write_free(a);
    };

    archive_write_add_filter_gzip(a);
    archive_write_set_format_pax_restricted(a);
    archive_write_open_filename(a, fn.u8string().c_str());
    for (auto &[f, n] : files)
    {
        if (!fs::exists(f))
        {
            result = false;
            continue;
        }

        // skip symlinks too
        if (!fs::is_regular_file(f))
            continue;

        ScopedFile fp(f);
        auto sz = fs::file_size(f);
        auto e = archive_entry_new();
        archive_entry_set_pathname(e, n.u8string().c_str());
        archive_entry_set_size(e, sz);
        archive_entry_set_filetype(e, AE_IFREG);
        archive_entry_set_perm(e, 0644);
        archive_write_header(a, e);
        char buff[BLOCK_SIZE];
        while (auto len = fread(buff, 1, sizeof(buff), fp.getHandle()))
            archive_write_data(a, buff, len);
        archive_entry_free(e);
    }
    return result;
}

bool pack_files(const path &fn, const Files &files, const path &root_dir)
{
    return pack_files(fn, primitives::pack::prepare_files(files, root_dir));
}

Files unpack_file(const path &fn, const path &dst)
{
    if (!fs::exists(dst))
        fs::create_directories(dst);

    Files files;

    auto a = archive_read_new();

    SCOPE_EXIT
    {
        archive_read_free(a);
    };

    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    auto r = archive_read_open_filename(a, fn.u8string().c_str(), BLOCK_SIZE);
    if (r != ARCHIVE_OK)
        throw std::runtime_error(archive_error_string(a));
    archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        // act on regular files only
        auto type = archive_entry_filetype(entry);
        if (type != AE_IFREG)
            continue;

        path f = dst / archive_entry_pathname(entry);
        path fdir = f.parent_path();
        if (!fs::exists(fdir))
            fs::create_directories(fdir);
        path filename = f.filename();
        if (filename == "." || filename == "..")
            continue;

        f = fs::absolute(f);
        ScopedFile o(f, "wb");
        while (1)
        {
            const void *buff;
            size_t size;
            int64_t offset;
            r = archive_read_data_block(a, &buff, &size, &offset);
            if (r == ARCHIVE_EOF)
                break;
            if (r < ARCHIVE_OK)
                throw std::runtime_error(archive_error_string(a));
            fwrite(buff, size, 1, o.getHandle());
        }
        files.insert(f);
    }
    return files;
}
