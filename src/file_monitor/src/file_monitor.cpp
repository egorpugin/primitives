// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/file_monitor.h>

#include <primitives/thread.h>

namespace primitives
{

namespace detail
{

int uv_loop_close(uv_loop_t &loop, Strings &errors)
{
    int r;
    if (r = uv_loop_close(&loop); r)
    {
        if (r == UV_EBUSY)
        {
            auto wlk = [](uv_handle_t* handle, void* arg)
            {
                if (handle)
                    uv_close(handle, 0);
            };
            uv_walk(&loop, wlk, 0);
            while (uv_run(&loop, UV_RUN_DEFAULT) != 0)
                ;
            if (r = uv_loop_close(&loop); r)
            {
                if (r == UV_EBUSY)
                    errors.push_back("resources were not cleaned up: "s + uv_strerror(r));
                else
                    errors.push_back("uv_loop_close() error: "s + uv_strerror(r));
            }
        }
        else
            errors.push_back("uv_loop_close() error: "s + uv_strerror(r));
    }
    return r;
}

}

namespace filesystem
{

FileMonitor::record::record(FileMonitor *mon, const path &dir, Callback callback, bool recursive)
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

FileMonitor::record::~record()
{
    if (uv_is_active((uv_handle_t*)&e))
        uv_close((uv_handle_t*)&e, 0);
}

void FileMonitor::record::stop()
{
    uv_fs_event_stop(&e);
}

void FileMonitor::record::cb(uv_fs_event_t* handle, const char* filename, int event, int status)
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

FileMonitor::FileMonitor()
{
    uv_loop_init(&loop);
    uv_run(&loop, UV_RUN_ONCE); // why?
}

FileMonitor::~FileMonitor()
{
    stop();

    Strings errors;
    // cleanup loop
    ::primitives::detail::uv_loop_close(loop, errors);
    for (auto &e : errors)
        std::cerr << e << "\n";
}

void FileMonitor::stop()
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

void FileMonitor::run()
{
    uv_run(&loop, UV_RUN_DEFAULT);
}

void FileMonitor::addFile(const path &p, record::Callback cb, bool recursive)
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

bool FileMonitor::has_file(const path &dir, const path &fn) const
{
    boost::upgrade_lock lk(m);
    auto di = dir_files.find(dir);
    if (di == dir_files.end())
        return false;
    boost::upgrade_lock lk2(di->second.m);
    // if empty, we consider as true for all children (files)
    return di->second.files.empty() || di->second.files.find(fn) != di->second.files.end();
}

}

}
