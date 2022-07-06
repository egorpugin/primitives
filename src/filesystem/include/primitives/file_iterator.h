// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "filesystem.h"

#include <functional>
#include <memory>

struct FileIterator
{
    using Buffer = std::vector<uint8_t>;
    using BuffersRef = std::vector<std::reference_wrapper<Buffer>>;

    struct File
    {
        path fn;
        std::unique_ptr<ScopedFile> ifile;
        uint64_t size;
        FileIterator::Buffer buf;
        uint64_t read = 0;

        File() = default;
        File(const File &) = delete;
        File(File &&) = default;
        File &operator=(const File &) = delete;
    };

    std::vector<File> files;
    int buffer_size = 8192;

    FileIterator() = default;
    FileIterator(const FilesOrdered &fns)
    {
        if (fns.empty())
            throw SW_RUNTIME_ERROR("Provide at least one file");

        for (auto &f : fns)
        {
            File d;
            d.size = fs::file_size(f);
            d.ifile = std::make_unique<ScopedFile>(f, "rb");
            files.emplace_back(std::move(d));
        }
    }
    FileIterator(const FileIterator &) = delete;
    FileIterator &operator=(const FileIterator &) = delete;
    ~FileIterator() = default;

    bool iterate(std::function<bool(const BuffersRef &, uint64_t)> f)
    {
        if (!is_same_size())
            return false;

        BuffersRef buffers;
        for (auto &f : files)
        {
            f.buf.resize(buffer_size);
            buffers.emplace_back(f.buf);
        }

        while (std::none_of(files.begin(), files.end(),
            [](const auto &f) { return feof(f.ifile->getHandle()); }))
        {
            std::for_each(files.begin(), files.end(), [this](auto &f)
            {
                f.read = f.ifile->read((char *)&f.buf[0], buffer_size);
            });
            if (!is_same_read_size())
                return false;

            if (!f(buffers, files.front().read))
                return false;
        }
        return true;
    }
    bool is_same_size() const
    {
        auto sz = files.front().size;
        return std::all_of(files.begin(), files.end(),
            [sz](const auto &f) { return f.size == sz; });
    }
    bool is_same_read_size() const
    {
        auto sz = files.front().read;
        return std::all_of(files.begin(), files.end(),
            [sz](const auto &f) { return f.read == sz; });
    }
};

inline bool compare_files(const path &fn1, const path &fn2)
{
    FileIterator fi({ fn1, fn2 });
    return fi.iterate([](const auto &bufs, auto sz)
    {
        return memcmp(bufs[0].get().data(), bufs[1].get().data(), (size_t)sz) == 0;
    });
}

inline bool compare_dirs(const path &dir1, const path &dir2)
{
    auto traverse_dir = [](const auto &dir)
    {
        std::vector<path> files;
        if (fs::exists(dir))
        {
            for (auto &f : fs::recursive_directory_iterator(dir))
            {
                if (!fs::is_regular_file(f))
                    continue;
                files.push_back(f);
            }
        }
        return files;
    };

    auto files1 = traverse_dir(dir1);
    auto files2 = traverse_dir(dir2);

    if (files1.empty())
        return false; // throw SW_RUNTIME_ERROR("left side has no files");
    if (files2.empty())
        return false; // throw SW_RUNTIME_ERROR("right side has no files");
    if (files1.size() != files2.size())
        return false; // throw SW_RUNTIME_ERROR("different number of files");

    auto sz = files1.size();
    for (size_t i = 0; i < sz; i++)
    {
        if (!compare_files(files1[i], files2[i]))
            return false;
    }

    return true;
}
