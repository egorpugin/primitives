// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>

#define private protected // WOW!!!
#include <boost/interprocess/sync/file_lock.hpp>
#undef private

#include <memory>
#include <mutex>

#ifdef BOOST_INTERPROCESS_WINDOWS
#include <windows.h>
#define STRING_MODE wstring
#else
#define STRING_MODE string
#endif

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

namespace primitives::lock {

class file_lock : public FileLock
{
public:
    // for other than windows systems
    using FileLock::FileLock;

#ifdef BOOST_INTERPROCESS_WINDOWS
    inline file_lock(const wchar_t *name)
    {
        m_file_hnd = open_existing_file(name, boost::interprocess::read_write);

        if (m_file_hnd == boost::interprocess::ipcdetail::invalid_file()) {
            boost::interprocess::error_info err(boost::interprocess::system_error_code());
            throw boost::interprocess::interprocess_exception(err);
        }
    }

private:
    struct interprocess_security_attributes
    {
        unsigned long nLength;
        void *lpSecurityDescriptor;
        int bInheritHandle;
    };

    static const unsigned long error_sharing_violation_tries = 3L;
    static const unsigned long error_sharing_violation_sleep_ms = 250L;

    inline boost::interprocess::file_handle_t open_existing_file(const wchar_t *name, boost::interprocess::mode_t mode = boost::interprocess::read_write, bool temporary = false)
    {
        unsigned long attr = temporary ? boost::interprocess::winapi::file_attribute_temporary : 0;
        return create_file(name, (unsigned int)mode, boost::interprocess::winapi::open_existing, attr, 0);
    }

    inline void *create_file(const wchar_t *name, unsigned long access, unsigned long creation_flags, unsigned long attributes, interprocess_security_attributes *psec)
    {
        for (unsigned int attempt(0); attempt < error_sharing_violation_tries; ++attempt) {
            void * const handle = CreateFileW(name, access,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                (LPSECURITY_ATTRIBUTES)psec, creation_flags, attributes, 0);
            bool const invalid(INVALID_HANDLE_VALUE == handle);
            if (!invalid) {
                return handle;
            }
            if (ERROR_SHARING_VIOLATION != GetLastError()) {
                return handle;
            }
            Sleep(error_sharing_violation_sleep_ms);
        }
        return INVALID_HANDLE_VALUE;
    }
#endif
};

static std::STRING_MODE prepare_lock_file(const path &fn)
{
    fs::create_directories(fn.parent_path());
    auto lock_file = fn.parent_path() / (fn.filename().string() + ".lock");
    if (!fs::exists(lock_file))
        primitives::filesystem::create(lock_file);
    return lock_file.STRING_MODE();
}

} // namespace primitives::lock

class ScopedFileLock
{
public:
    ScopedFileLock(const path &fn)
    {
        lock_ = std::make_unique<primitives::lock::file_lock>(primitives::lock::prepare_lock_file(fn).c_str());
        lock_->lock();
        locked = true;
    }
    ScopedFileLock(const path &fn, std::defer_lock_t)
    {
        lock_ = std::make_unique<primitives::lock::file_lock>(primitives::lock::prepare_lock_file(fn).c_str());
    }
    ~ScopedFileLock()
    {
        if (locked)
            lock_->unlock();
    }

    bool try_lock()
    {
        return locked = lock_->try_lock();
    }
    bool is_locked() const { return locked; }
    void lock()
    {
        lock_->lock();
        locked = true;
    }

private:
    FileLockPtr lock_;
    bool locked = false;
};

class ScopedShareableFileLock
{
public:
    ScopedShareableFileLock(const path &fn)
    {
        lock_ = std::make_unique<primitives::lock::file_lock>(primitives::lock::prepare_lock_file(fn).c_str());
        lock_->lock_sharable();
    }
    ~ScopedShareableFileLock()
    {
        lock_->unlock_sharable();
    }

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

template <typename F>
bool try_single_process_job(const path &fn, F && f)
{
    ScopedFileLock fl(fn, std::defer_lock);
    if (fl.try_lock())
    {
        std::forward<F>(f)();
        return true;
    }
    return false;
}
