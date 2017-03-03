#include <primitives/lock.h>

#include <boost/interprocess/sync/file_lock.hpp>

#include <iostream>

std::string prepare_lock_file(const path &fn)
{
    fs::create_directories(fn.parent_path());
    auto lock_file = fn.parent_path() / (fn.filename().string() + ".lock");
    if (!fs::exists(lock_file))
        std::ofstream(lock_file.string());
    return lock_file.string();
}

////////////////////////////////////////

ScopedFileLock::ScopedFileLock(const path &fn)
{
    lock_ = std::make_unique<FileLock>(prepare_lock_file(fn).c_str());
    lock_->lock();
    locked = true;
}

ScopedFileLock::ScopedFileLock(const path &fn, std::defer_lock_t)
{
    lock_ = std::make_unique<FileLock>(prepare_lock_file(fn).c_str());
}

ScopedFileLock::~ScopedFileLock()
{
    if (locked)
        lock_->unlock();
}

bool ScopedFileLock::try_lock()
{
    return locked = lock_->try_lock();
}

void ScopedFileLock::lock()
{
    lock_->lock();
    locked = true;
}

////////////////////////////////////////

ScopedShareableFileLock::ScopedShareableFileLock(const path &fn)
{
    lock = std::make_unique<FileLock>(prepare_lock_file(fn).c_str());
    lock->lock_sharable();
}

ScopedShareableFileLock::~ScopedShareableFileLock()
{
    lock->unlock_sharable();
}
