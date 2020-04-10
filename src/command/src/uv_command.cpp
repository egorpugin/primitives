// Copyright (C) 2020 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "uv_command.h"

#include <primitives/exceptions.h>

#include <string.h> // memset()

#ifndef _WIN32
extern char **environ;
#endif

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace primitives::command::detail
{

io_data::io_data(UvCommand &c)
    : c(c)
{
    clear(T_EMPTY);
}

void io_data::set_ignore(int fd)
{
    clear(T_IGNORE);
    this->fd = fd;
}

uv_pipe_t *io_data::get_redirect_pipe(bool out)
{
    clear(T_REDIRECT_BETWEEN_PROCESSES);

    this->out = out;
    this->p_provided = &p;
    return p_provided;
}

void io_data::set_redirect_pipe(uv_pipe_t *p_provided, bool out)
{
    clear(T_REDIRECT_BETWEEN_PROCESSES);

    this->out = out;
    this->p_provided = p_provided;
}

void io_data::set_capture(int fd, primitives::Command::Stream &dst, bool out)
{
    if (!out && dst.text.empty())
        return;

    clear(T_CAPTURE);

    this->out = out;
    this->fd = fd;
    p.data = &dst;
    uv_pipe_init(c.loop, &p, 0);
}

void io_data::set_redirect(const path &file, bool out, bool append)
{
    if (file.empty())
        return;

    clear(T_REDIRECT_TO_FILE);

    this->out = out;
    auto f = file.u8string();

    int flags = 0;
#ifdef _WIN32
    flags = O_BINARY;
#endif
    if (out)
        flags |= (append ? O_APPEND : (O_CREAT | O_TRUNC)) | O_WRONLY;
    else
        flags |= O_RDONLY;

    uv_fs_open(c.loop, &file_req, f.c_str(), flags, 0644, NULL);
    uv_fs_req_cleanup(&file_req);

    fd = (int)file_req.result;
}
// add set redirect for fd r/w

void io_data::set_inherit(int fd)
{
    clear(T_INHERIT);
    this->fd = fd;
}

void io_data::set_type(int t)
{
    if (t < type)
        throw SW_LOGIC_ERROR("cannot downgrade stdio type from " + std::to_string(type) + " to " + std::to_string(t));
    type = t;
}

void io_data::clear(int t)
{
    set_type(t);

    out = true;
    fd = -1;
}

void io_data::clean() noexcept
{
    if (c.c.detached || c.c.create_new_console)
        return;

    switch (type)
    {
    case T_CAPTURE:
        if (out)
            uv_close((uv_handle_t *)&p, NULL);
        // it is closed in on_write()
        break;

    case T_REDIRECT_TO_FILE:
#if UV_VERSION_MAJOR > 1
        uv_fs_close(c.loop, &file_req, uv_get_osfhandle(file_req.result), 0);
#else
        uv_fs_close(c.loop, &file_req, (uv_file)file_req.result, 0);
#endif
        break;

    default:
        break;
    }
}

uv_stdio_container_t io_data::get_child_stdio() const
{
    uv_stdio_container_t s;
    memset(&s, 0, sizeof(s));

    if (c.c.detached || c.c.create_new_console)
        return s;

    switch (type)
    {
    case T_INHERIT:
    case T_REDIRECT_TO_FILE:
        s.flags = UV_INHERIT_FD;
#if UV_VERSION_MAJOR > 1
        s.data.file = uv_get_osfhandle(fd);
#else
        s.data.fd = fd;
#endif
        break;

    case T_REDIRECT_BETWEEN_PROCESSES:
        if (out)
            s.flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
        else
            s.flags = UV_INHERIT_STREAM;
        s.data.stream = (uv_stream_t*)p_provided;
        break;

    case T_CAPTURE:
        if (out)
            s.flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
        else
            s.flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_READABLE_PIPE);
        s.data.stream = (uv_stream_t*)&p;
        break;

    case T_EMPTY:
    case T_IGNORE:
        s.flags = UV_IGNORE;
        break;

    default:
        throw SW_LOGIC_ERROR("unknown type");
    }

    return s;
}

void io_data::start_capture(Strings &errors)
{
    if (c.c.detached || c.c.create_new_console)
        return;
    if (type != T_CAPTURE)
        return;
    if (out)
    {
        if (auto r = uv_read_start((uv_stream_t *)&p, on_alloc, on_read); r)
            errors.push_back("cannot start read from fd" + std::to_string(fd) + ": " + uv_strerror(r));
    }
    else
    {
        auto s = ((primitives::Command::Stream *)p.data);
        wbuf.base = s->text.data();
        wbuf.len = (decltype(wbuf.len))s->text.size();
        wreq.data = this;
        if (auto r = uv_write(&wreq, (uv_stream_t *)&p, &wbuf, 1, on_write); r)
            errors.push_back("cannot start write from string to fd" + std::to_string(fd) + ": " + uv_strerror(r));
    }
}

void io_data::on_alloc(uv_handle_t *s, size_t suggested_size, uv_buf_t *buf)
{
    auto c = ((primitives::Command::Stream*)s->data);
    buf->base = c->buf;
    buf->len = sizeof(c->buf);
}

void io_data::on_read(uv_stream_t *s, ssize_t nread, const uv_buf_t *rdbuf)
{
    auto c = ((primitives::Command::Stream*)s->data);

    if (nread == UV_EOF)
    {
        if (c->action)
            c->action({}, true);
        return;
    }
    if (nread <= 0)
        return; // something really wrong - throw?

    c->text.append(rdbuf->base, rdbuf->base + nread);

    if (c->action)
        c->action({ rdbuf->base, rdbuf->base + nread }, false);
}

void io_data::on_write(uv_write_t *req, int status)
{
    auto d = (io_data *)req->data;
    if (status == 0)
    {
        uv_close((uv_handle_t*)&d->p, 0);
        return;
    }
}

std::vector<uv_stdio_container_t> io_datas::get_child_stdio() const
{
    std::vector<uv_stdio_container_t> c;
    for (auto &i : *this)
        c.push_back(i->get_child_stdio());
    return c;
}

UvCommand::UvCommand(uv_loop_t *loop, primitives::Command &c)
    : loop(loop), c(c)
{
    auto &arguments = c.getArguments();

    prog = arguments[0]->toString();
    wdir = c.working_directory.u8string();

    // args
    for (const auto &a : c.getArguments())
        args.push_back(a->toString());
    for (const auto &a : args)
        uv_args.push_back((char *)a.data());
    uv_args.push_back(nullptr); // last null

    // setup default fds
    {
        int n = 0;
        auto in = std::make_unique<io_data>(*this);
        in->set_ignore(n); // before capture
        in->set_capture(n, c.in, false);
        in->set_redirect(c.in.file, false);
        if (c.inherit || c.in.inherit)
            in->set_inherit(n);
        streams.push_back(std::move(in));

        n++;
        auto out = std::make_unique<io_data>(*this);
        out->set_capture(n, c.out);
        out->set_redirect(c.out.file, true, c.out.append);
        if (c.inherit || c.out.inherit)
            out->set_inherit(n);
        streams.push_back(std::move(out));

        n++;
        auto err = std::make_unique<io_data>(*this);
        err->set_capture(n, c.err);
        err->set_redirect(c.err.file, true, c.err.append);
        if (c.inherit || c.err.inherit)
            err->set_inherit(n);
        streams.push_back(std::move(err));
    }

    // options
    {
        options.file = prog.c_str();
        options.cwd = wdir.c_str();
        options.args = uv_args.data();
        //#if UV_VERSION_MAJOR > 1
#ifdef _WIN32
        options.attribute_list = c.attribute_list;
#endif
        //#endif
        if (c.detached)
            options.flags |= UV_PROCESS_DETACHED;
#ifdef _WIN32
        if (c.create_new_console)
            options.flags |= UV_PROCESS_WINDOWS_ALLOC_CONSOLE;
        else
            options.flags |= UV_PROCESS_WINDOWS_HIDE;
#endif
        //if (!detached)
        options.exit_cb = on_exit;
    }

    // set env
    {
#ifdef _WIN32
        // must be case insensitive on windows
        struct CaseInsensitiveComparator
        {
            bool operator()(const std::string &a, const std::string &b) const noexcept
            {
                return ::_stricmp(a.c_str(), b.c_str()) < 0;
            }
        };
        std::map<std::string, std::string, CaseInsensitiveComparator> env_map;
#else
        StringMap<String> env_map;
#endif
        if (!c.environment.empty())
        {
#ifdef _WIN32
            // TODO: switch to GetEnvironmentStringsW
            auto lpvEnv = GetEnvironmentStringsA();
            if (!lpvEnv)
                throw SW_RUNTIME_ERROR("GetEnvironmentStrings failed: " + std::to_string(GetLastError()));

            auto lpszVariable = (LPTSTR)lpvEnv;
            while (*lpszVariable)
            {
                String s(lpszVariable);
                env_map[s.substr(0, s.find('='))] = s.substr(s.find('=') + 1);
                lpszVariable += lstrlenA(lpszVariable) + 1;
            }
            FreeEnvironmentStringsA(lpvEnv);
#else
            for (int i = 0; environ[i]; i++)
            {
                String s(environ[i]);
                env_map[s.substr(0, s.find('='))] = s.substr(s.find('=') + 1);
            }
#endif
        }

        // fill combined map
        for (auto &[k, v] : c.environment)
            env_map[k] = v;

        // fill env data
        for (auto &[k, v] : env_map)
            env_data.push_back(k + "=" + v);
        for (auto &e : env_data)
            env.push_back(e.data());
        if (!env.empty() || !c.inherit_current_evironment)
        {
            env.push_back(0); // end of env block
            options.env = env.data();
        }
    }

    // self pointer
    child_req.data = this;
}

// actually might throw in case of oom during errors.push_back, but let's ignore it for now
void UvCommand::execute(Strings &errors) noexcept
{
    child_stdio = streams.get_child_stdio();
    options.stdio = child_stdio.data();
    options.stdio_count = (int)child_stdio.size();

    c.onBeforeRun();

    // spawn
    if (auto r = uv_spawn(loop, &child_req, &options); r)
        errors.push_back("child not started: "s + uv_strerror(r));

    c.pid = child_req.pid;

    // start capture
    for (auto &s : streams)
        s->start_capture(errors);

    if (c.detached)
    {
        uv_unref((uv_handle_t*)&child_req);
        //uv_close((uv_handle_t*)&child_req);
    }
}

void UvCommand::clean() noexcept
{
    for (auto &s : streams)
        s->clean();
}

void UvCommand::on_exit(uv_process_t *child_req, int64_t exit_status, int term_signal)
{
    auto c = (UvCommand*)(child_req->data);
    c->c.exit_code = exit_status;
    c->c.onEnd();
    uv_close((uv_handle_t*)child_req, 0);
}

} // namespace primitives::command::detail
