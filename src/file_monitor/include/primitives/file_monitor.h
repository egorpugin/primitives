// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>
#include <primitives/thread.h>

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/lock_types.hpp>

// after primitives and boost
#include "uv_loop_close.h"

#include <atomic>
#include <functional>
#include <memory>
#include <thread>

namespace primitives::filesystem
{

class FileMonitor
{
    struct record
    {
        using Callback = std::function<void(const path &, int event)>;

        const FileMonitor *mon;
        path dir;
        path_u8string dir_holder;
        uv_fs_event_t e;
        Callback callback;

        record(FileMonitor *mon, const path &dir, Callback callback, bool recursive)
            : mon(mon), dir(dir), dir_holder(to_path_string(dir)), callback(callback)
        {
            if (uv_fs_event_init(&mon->loop, &e))
                return;
            e.data = this;
            if (uv_fs_event_start(&e, cb, (const char *)dir_holder.c_str(),
                // was 0 = non recursive
                recursive ? UV_FS_EVENT_RECURSIVE : 0
            ) < 0)
            {
                // maybe unconditionally?
                if (uv_is_active((uv_handle_t*)&e))
                    uv_close((uv_handle_t*)&e, 0);
            }
        }
        ~record()
        {
            if (uv_is_active((uv_handle_t*)&e))
                uv_close((uv_handle_t*)&e, 0);
        }

        void stop()
        {
            uv_fs_event_stop(&e);
        }

        static void cb(uv_fs_event_t* handle, const char* filename, int event, int status)
        {
            auto r = (record*)handle->data;
            if (status < 0)
            {
                if (status == UV_EBADF)
                    return;
                return r->stop();
            }
            if (!filename)
                return;
            path fn((const path_char_t *)filename);
            if (r->mon->has_file(r->dir, fn) && r->callback)
            {
                // protected add dir
                auto p = r->dir;
                p += "/";
                p += fn;
                r->callback(p, event);
            }
        }
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
    FileMonitor()
    {
        uv_loop_init(&loop);
        uv_run(&loop, UV_RUN_ONCE); // why?
    }
    ~FileMonitor()
    {
        stop();

        Strings errors;
        // cleanup loop
        ::primitives::detail::uv_loop_close(loop, errors);
        for (auto &e : errors)
            std::cerr << e << "\n";
    }

    void addFile(const path &p, record::Callback cb, bool recursive = true)
    {
        if (stopped)
            return;
        if (p.empty())
            return;

        bool is_dir = false;
        auto dir = p;
        error_code ec;
        if (fs::is_regular_file(p, ec))
            dir = p.parent_path();
        else
            is_dir = true;
        if (ec)
            return;

    #ifdef _WIN32
        dir = normalize_path_windows(dir);
    #else
        dir = normalize_path(dir);
    #endif

        boost::upgrade_lock lk(m);
        auto di = dir_files.find(dir);
        if (di != dir_files.end())
        {
            boost::unique_lock lk2(di->second.m);
            if (!is_dir)
                di->second.files.insert(p.filename());
            return;
        }
        boost::upgrade_to_unique_lock lk2(lk);
        auto &ref = dir_files[dir];
        if (!is_dir)
            ref.files.insert(p.filename());
        ref.r = std::make_shared<record>(this, dir, cb, recursive);
    }
    void stop()
    {
        if (stopped)
            return;
        stopped = true;

        // first, stop all watchers, but not clear them!!!
        for (auto &d : dir_files)
            d.second.r->stop();

        // stop loop
        uv_stop(&loop);
    }
    void run()
    {
        uv_run(&loop, UV_RUN_DEFAULT);
    }

private:
    bool stopped = false;

    bool has_file(const path &dir, const path &fn) const
    {
        boost::upgrade_lock lk(m);
        auto di = dir_files.find(dir);
        if (di == dir_files.end())
            return false;
        boost::upgrade_lock lk2(di->second.m);
        // if empty, we consider as true for all children (files)
        return di->second.files.empty() || di->second.files.find(fn) != di->second.files.end();
    }
};

}

}
