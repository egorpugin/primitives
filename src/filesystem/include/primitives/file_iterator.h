// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "filesystem.h"

#include <vector>

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
    };

    std::vector<File> files;
    int buffer_size = 8192;

    FileIterator(const std::vector<path> &fns);

    bool iterate(std::function<bool(const BuffersRef &, uint64_t)> f);
    bool is_same_size() const;
    bool is_same_read_size() const;
};
