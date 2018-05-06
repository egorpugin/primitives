// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>

#include <memory>
#include <mutex>

namespace boost
{
    namespace interprocess
    {
        class file_lock;
        class named_mutex;
    }
}

namespace Interprocess = boost::interprocess;
using FileLock = Interprocess::file_lock;
using FileLockPtr = std::unique_ptr<FileLock>;
using InterprocessMutex = Interprocess::named_mutex;

class PRIMITIVES_LOCK_API ScopedFileLock
{
public:
    ScopedFileLock(const path &fn);
    ScopedFileLock(const path &fn, std::defer_lock_t);
    ~ScopedFileLock();

    bool try_lock();
    bool is_locked() const { return locked; }
    void lock();

private:
    FileLockPtr lock_;
    bool locked = false;
};

class PRIMITIVES_LOCK_API ScopedShareableFileLock
{
public:
    ScopedShareableFileLock(const path &fn);
    ~ScopedShareableFileLock();

private:
    FileLockPtr lock_;
};

template <typename F>
void single_process_job(const path &fn, F && f)
{
    ScopedFileLock fl(fn, std::defer_lock);
    if (fl.try_lock())
    {
        std::forward<F>(f)();
    }
    else
    {
        // if we were not locked, we must wait until operation is completed
        ScopedFileLock fl2(fn);
    }
}
