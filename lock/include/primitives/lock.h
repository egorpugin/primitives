#pragma once

#include <primitives/filesystem.h>

#include <memory>
#include <mutex>
#include <shared_mutex>

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

using shared_mutex = std::shared_timed_mutex;

class ScopedFileLock
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

class ScopedShareableFileLock
{
public:
    ScopedShareableFileLock(const path &fn);
    ~ScopedShareableFileLock();

private:
    FileLockPtr lock;
};