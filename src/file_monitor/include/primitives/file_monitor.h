// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>

#include <uv.h>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/lock_types.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <thread>

namespace primitives
{

namespace detail
{

PRIMITIVES_FILE_MONITOR_API
int uv_loop_close(uv_loop_t &loop, Strings &errors);

}

namespace filesystem
{

class PRIMITIVES_FILE_MONITOR_API FileMonitor
{
    struct record
    {
        using Callback = std::function<void(const path &, int event)>;

        const FileMonitor *mon;
        path dir;
        path_u8string dir_holder;
        uv_fs_event_t e;
        Callback callback;

        record(FileMonitor *mon, const path &dir, Callback callback, bool recursive);
        ~record();

        void stop();

        static void cb(uv_fs_event_t* handle, const char* filename, int events, int status);
    };

    struct holder
    {
        std::shared_ptr<record> r;
        Files files;
        mutable boost::upgrade_mutex m;
    };

    uv_loop_t loop;
    std::unordered_map<path, holder> dir_files;
    mutable boost::upgrade_mutex m;

public:
    FileMonitor();
    ~FileMonitor();

    void addFile(const path &p, record::Callback cb, bool recursive = true);
    void stop();
    void run();

private:
    bool stopped = false;

    bool has_file(const path &dir, const path &fn) const;
};

}

}
