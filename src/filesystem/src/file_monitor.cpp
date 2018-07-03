// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/file_monitor.h>

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
            auto wlk = [](uv_handle_t* handle, void* arg) { uv_close(handle, 0); };
            uv_walk(&loop, wlk, 0);
            uv_run(&loop, UV_RUN_ONCE);
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

FileMonitor::record::record(FileMonitor *mon, const path &dir, Callback callback)
    : mon(mon), dir(dir), dir_holder(dir.u8string()), callback(callback)
{
    if (uv_fs_event_init(&mon->loop, &e))
        return;
    e.data = this;
    if (uv_fs_event_start(&e, cb, dir_holder.c_str(), 0))
    {
        uv_close((uv_handle_t*)&e, 0);
        return;
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

void FileMonitor::record::cb(uv_fs_event_t* handle, const char* filename, int events, int status)
{
    if (!filename)
        return;
    auto r = (record*)handle->data;
    auto fn = fs::u8path(filename);
    if (r->mon->has_file(r->dir, fn) && r->callback)
        r->callback(r->dir / fn);
}

FileMonitor::FileMonitor()
{
    uv_loop_init(&loop);
    uv_run(&loop, UV_RUN_ONCE);
}

FileMonitor::~FileMonitor()
{
    // at first, stop all watchers
    for (auto &d : dir_files)
        d.second.r->stop();

    // join thread
    t.join();

    Strings errors;

    // and cleanup loop
    ::primitives::detail::uv_loop_close(loop, errors);
}

void FileMonitor::start()
{
    bool e = false;
    if (!started.compare_exchange_strong(e, true))
        return;

    t = std::move(std::thread([this]
    {
        uv_run(&loop, UV_RUN_DEFAULT);
    }));
}

void FileMonitor::addFile(const path &p, record::Callback cb)
{
    auto dir = p;
    if (fs::is_regular_file(p))
        dir = p.parent_path();

    boost::upgrade_lock lk(m);
    auto di = dir_files.find(dir);
    if (di != dir_files.end())
    {
        boost::unique_lock lk2(di->second.m);
        di->second.files.insert(p.filename());
        return;
    }
    boost::upgrade_to_unique_lock lk2(lk);
    auto &ref = dir_files[dir];
    ref.files.insert(p.filename());
    ref.r = std::make_shared<record>(this, dir, cb);
    start();
}

bool FileMonitor::has_file(const path &dir, const path &fn) const
{
    boost::upgrade_lock lk(m);
    auto di = dir_files.find(dir);
    if (di == dir_files.end())
        return false;
    boost::upgrade_lock lk2(di->second.m);
    return di->second.files.find(fn) != di->second.files.end();
}

}

}
