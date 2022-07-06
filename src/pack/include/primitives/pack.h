// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>

#include <primitives/exceptions.h>
#include <primitives/templates.h>

#include <iostream>

#include <archive.h>
#include <archive_entry.h>

#if !defined(_WIN32) && !defined(__APPLE__)
#include <linux/limits.h>
#endif

namespace primitives::pack
{

inline std::map<path, path> prepare_files(const FilesSorted &files, const path &root_dir, const path &dir_prefix = path(), const path &file_prefix = path())
{
    std::map<path, path> files2;
    for (auto &f : files)
    {
        auto s = fs::status(f);

        if (s.type() == fs::file_type::not_found)
        {
            files2[f]; // still add to return false later
            continue; // report error?
        }

        // skip symlinks too
        if (s.type() != fs::file_type::regular && s.type() != fs::file_type::directory)
            continue; // report error?

        auto r = fs::relative(f, root_dir);
        if (to_printable_string(r).find("..") != String::npos)
        {
            if (!is_under_root(f, root_dir))
                continue; // report error?
        }
        if (!file_prefix.empty())
            r = file_prefix.string() + r.string();
        if (!dir_prefix.empty())
            r = dir_prefix / r;
        files2[f] = r;
    }
    return files2;
}

} // namespace primitives::pack

namespace primitives::pack::detail {

constexpr inline auto block_size = 8192;

}

// use sorted arrays to get predictable results

bool pack_files(const path &archive, const std::map<path, path> &files, String *error = nullptr)
{
    bool result = true;
    auto a = archive_write_new();

    SCOPE_EXIT
    {
        archive_write_free(a);
    };

    //archive_write_add_filter_gzip(a);
    //archive_write_set_format_pax_restricted(a);

    if (archive_write_set_format_filter_by_ext(a, (const char *)archive.filename().u8string().c_str()) != ARCHIVE_OK)
        throw SW_RUNTIME_ERROR(archive_error_string(a));

    // predictable results
    archive_write_set_filter_option(a, 0, "timestamp", 0);

    auto r = archive_write_open_filename(a, (const char *)archive.u8string().c_str());
    if (r != ARCHIVE_OK)
        throw SW_RUNTIME_ERROR(archive_error_string(a));

    SCOPE_EXIT
    {
        archive_write_close(a);
    };

    for (auto &[f, n] : files)
    {
        auto s = fs::status(f);

        if (s.type() == fs::file_type::not_found)
        {
            result = false;
            if (error)
                *error = "Missing file: " + to_printable_string(f);
            continue;
        }

        // skip symlinks too
        if (s.type() == fs::file_type::regular)
        {
            ScopedFile fp(f);
            auto sz = fs::file_size(f);
            auto e = archive_entry_new();
            archive_entry_set_pathname(e, (const char *)n.u8string().c_str());
            archive_entry_set_size(e, sz);
            archive_entry_set_filetype(e, AE_IFREG);
            archive_entry_set_perm(e, 0644);
            archive_write_header(a, e);
            char buff[primitives::pack::detail::block_size];
            while (auto len = fread(buff, 1, sizeof(buff), fp.getHandle()))
                archive_write_data(a, buff, len);
            archive_entry_free(e);
        }
        else if (s.type() == fs::file_type::directory)
        {
            auto e = archive_entry_new();
            archive_entry_set_pathname(e, (const char *)n.u8string().c_str());
            archive_entry_set_filetype(e, AE_IFDIR);
            archive_entry_set_perm(e, 0644);
            archive_write_header(a, e);
            archive_entry_free(e);
        }
    }
    return result;
}

bool pack_files(const path &archive, const FilesSorted &files, const path &root_dir, String *error = nullptr)
{
    return pack_files(archive, primitives::pack::prepare_files(files, root_dir), error);
}

FilesSorted unpack_file(const path &archive, const path &dest_dir)
{
    if (!fs::exists(dest_dir))
        fs::create_directories(dest_dir);

    FilesSorted files;

    auto a = archive_read_new();

    SCOPE_EXIT
    {
        archive_read_free(a);
    };

    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    auto r = archive_read_open_filename(a, (const char *)archive.u8string().c_str(), primitives::pack::detail::block_size);
    if (r != ARCHIVE_OK)
        throw SW_RUNTIME_ERROR(archive_error_string(a));

    SCOPE_EXIT
    {
        archive_read_close(a);
    };

    while (1)
    {
        int ec;
        archive_entry *entry;
        while ((ec = archive_read_next_header(a, &entry)) == ARCHIVE_RETRY)
            ;
        if (ec == ARCHIVE_WARN) {
            std::cerr << "libarchive warning: " << archive_error_string(a) << "\n";
        }
        if (ec == ARCHIVE_FATAL)
            throw SW_RUNTIME_ERROR(archive_error_string(a));
        if (ec == ARCHIVE_EOF)
            break;

        // act on regular files only
        auto type = archive_entry_filetype(entry);
        if (type != AE_IFREG)
            continue;

        auto fn = archive_entry_pathname(entry);
        if (!fn)
            continue;

        path f = dest_dir / fn;
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
                throw SW_RUNTIME_ERROR(archive_error_string(a));
            fwrite(buff, size, 1, o.getHandle());
        }
        files.insert(f);
    }
    return files;
}

