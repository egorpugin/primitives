// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/asio/io_context.hpp>

#include <primitives/command.h>
#include <primitives/file_monitor.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/asio/executor_work_guard.hpp>
//#include <boost/nowide/convert.hpp>
#include <boost/process.hpp>

#include <uv.h>

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>

#ifndef _WIN32
extern char **environ;
#endif

auto &get_io_context()
{
    static boost::asio::io_context io_context;
    // protect context from being stopped
    static auto wg = boost::asio::make_work_guard(io_context);
    return io_context;
}

namespace primitives
{

namespace bp = boost::process;

struct CommandData
{
    using cb_func = std::function<void(const boost::system::error_code &, std::size_t)>;

    boost::asio::io_context &ios;
    boost::process::async_pipe pout, perr;

    cb_func out_cb, err_cb;

    std::vector<char> out_buf, err_buf;
    FILE *out_fs = nullptr;
    FILE *err_fs = nullptr;

    std::mutex m;
    std::condition_variable cv;
    std::atomic_int stopped{ 0 };

    // TODO: add pid and option to command to execute callbacks right in this thread

    CommandData(boost::asio::io_context &ios)
        : ios(ios), pout(ios), perr(ios)
    {
    }

    ~CommandData()
    {
        if (out_fs)
            fclose(out_fs);
        if (err_fs)
            fclose(err_fs);
    }
};

path resolve_executable(const path &p)
{
#ifdef _WIN32
    static const std::vector<String> exts = []
    {
        String s = getenv("PATHEXT");
        boost::to_lower(s);
        std::vector<String> exts;
        boost::split(exts, s, boost::is_any_of(";"));
        return exts;
    }();
    if (p.has_extension() &&
        std::find(exts.begin(), exts.end(), boost::to_lower_copy(p.extension().string())) != exts.end() &&
        fs::exists(p) &&
        fs::is_regular_file(p)
        )
        return p;
    // do not test direct file
    if (fs::exists(p) && fs::is_regular_file(p))
        return p;
    // failed, test new exts
    for (auto &e : exts)
    {
        auto p2 = p;
        p2 += e;
        if (fs::exists(p2) && fs::is_regular_file(p2))
            return p2;
    }
#else
    if (fs::exists(p) && fs::is_regular_file(p))
        return p;
#endif
    return bp::search_path(p.wstring()).wstring();
}

path resolve_executable(const std::vector<path> &paths)
{
    for (auto &p : paths)
    {
        auto e = resolve_executable(p);
        if (!e.empty())
            return e;
    }
    return path();
}

Command::Command()
{
}

Command::~Command()
{
}

String Command::print() const
{
    String s;
    s += "\"" + program.u8string() + "\" ";
    for (auto &a : args)
        s += "\"" + a + "\" ";
    s.resize(s.size() - 1);
    return s;
}

path Command::getProgram() const
{
    if (!program.empty())
        return program;
    if (args.empty())
        throw std::runtime_error("No program was set");
    return args[0];
}

void Command::execute1(std::error_code *ec_in)
{
    // setup

    // try to use first arg as a program
    if (program.empty())
    {
        if (args.empty())
            throw std::runtime_error("No program was set");
        program = args[0];
        auto t = std::move(args);
        args.assign(t.begin() + 1, t.end());
    }

    // resolve exe
    {
        auto p = resolve_executable(program);
        if (p.empty())
        {
            auto e = "Cannot resolve executable: " + program.u8string();
            if (!ec_in)
                throw std::runtime_error(e);
            *ec_in = std::make_error_code(std::errc::no_such_file_or_directory);
            err.text = e;
            return;
        }
        program = p;
    }

    if (working_directory.empty())
        working_directory = fs::current_path();

    // current loop
    uv_loop_t loop;
    uv_loop_init(&loop);

    // data holders, all strings are utf-8
    auto prog = program.u8string();
    auto wdir = working_directory.u8string();
    auto in_file_s = in.file.u8string();
    auto out_file_s = out.file.u8string();
    auto err_file_s = err.file.u8string();
    int r;

    // args
    std::vector<char*> uv_args;
    uv_args.push_back(prog.data()); // this name arg, win only?
    for (auto &a : args)
        uv_args.push_back(a.data());
    uv_args.push_back(nullptr); // last null

    // setup pipes
    uv_fs_t in_file_req, out_file_req, err_file_req;
    uv_pipe_t pin = { 0 }, pout = { 0 }, perr = { 0 };
    pin.data = &this->in;
    pout.data = &this->out;
    perr.data = &this->err;

    uv_stdio_container_t child_stdio[3];

    if (inherit)
        out.inherit = err.inherit = true;

    auto setup_pipe = [&loop, &child_stdio](auto &stream, int fd, auto &pipe, auto &file_req, auto &file, bool out = true)
    {
        if (stream.inherit)
        {
            child_stdio[fd].flags = UV_INHERIT_FD;
            child_stdio[fd].data.fd = fd;
        }
        else if (!stream.file.empty())
        {
            if (out)
            {
                uv_fs_open(&loop, &file_req, file.c_str(), O_CREAT | O_RDWR, 0644, NULL);
                uv_fs_req_cleanup(&file_req);
                child_stdio[fd].flags = UV_INHERIT_FD;
                child_stdio[fd].data.fd = file_req.result;
            }
            else
            {
                //uv_pipe_init(&loop, &pipe, 0);
                uv_fs_open(&loop, &file_req, file.c_str(), O_RDONLY | O_BINARY, 0644, NULL);
                uv_fs_req_cleanup(&file_req);
                child_stdio[fd].flags = UV_INHERIT_FD;//(uv_stdio_flags)(UV_CREATE_PIPE | UV_READABLE_PIPE);
                child_stdio[fd].data.fd = file_req.result;
            }
        }
        else
        {
            if (out)
            {
                uv_pipe_init(&loop, &pipe, 0);
                child_stdio[fd].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
                child_stdio[fd].data.stream = (uv_stream_t*)&pipe;
            }
            else
                child_stdio[0].flags = UV_IGNORE;
        }
    };
    setup_pipe(in, 0, pin, in_file_req, in_file_s, false);
    setup_pipe(out, 1, pout, out_file_req, out_file_s);
    setup_pipe(err, 2, perr, err_file_req, err_file_s);

    // exit callback
    auto on_exit = [](uv_process_t *req, int64_t exit_status, int term_signal)
    {
        ((Command*)(req->data))->exit_code = exit_status;
        uv_close((uv_handle_t*)req, 0);
    };

    // options
    uv_process_options_t options = { 0 };
    options.exit_cb = on_exit;
    options.file = prog.c_str();
    options.cwd = wdir.c_str();
    options.args = uv_args.data();
    options.stdio = child_stdio;
    options.stdio_count = 3;

    // set env
    std::vector<char *> env;
    std::vector<String> env_data;
    if (!environment.empty())
    {
#ifdef _WIN32
        auto lpvEnv = GetEnvironmentStrings();
        if (!lpvEnv)
            throw std::runtime_error("GetEnvironmentStrings failed: " + std::to_string(GetLastError()));

        auto lpszVariable = (LPTSTR)lpvEnv;
        while (*lpszVariable)
        {
            env_data.push_back(lpszVariable);
            lpszVariable += lstrlen(lpszVariable) + 1;
        }
        FreeEnvironmentStrings(lpvEnv);
#else
        for (int i = 0; environ[i]; i++)
            env_data.push_back(environ[i]);
#endif
    }
    for (auto &[k, v] : environment)
        env_data.push_back(k + "=" + v);
    for (auto &e : env_data)
        env.push_back(e.data());
    if (!env.empty() || !inherit_current_evironment)
    {
        env.push_back(0);
        options.env = env.data();
    }

    // child handle
    uv_process_t child_req = { 0 };
    child_req.data = this; // self pointer

    if (r = uv_spawn(&loop, &child_req, &options); r)
        errors.push_back("child not started: "s + uv_strerror(r));

    pid = child_req.pid;

    // capture callbacks
    auto on_alloc = [](uv_handle_t *s, size_t suggested_size, uv_buf_t *buf)
    {
        auto c = ((Command::Stream*)s->data);
        buf->base = c->buf;
        buf->len = sizeof(c->buf);
    };
    auto on_read = [](uv_stream_t *s, ssize_t nread, const uv_buf_t *rdbuf)
    {
        auto c = ((Command::Stream*)s->data);

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
    };

    // start capture
    auto start_capture = [&r, &on_alloc, &on_read, this](auto &stream, auto &pipe, auto &pipe_name)
    {
        if (!stream.inherit && stream.file.empty())
        {
            if (r = uv_read_start((uv_stream_t*)&pipe, on_alloc, on_read); r)
                errors.push_back("cannot start read from std"s + pipe_name + ": " + uv_strerror(r));
        }
    };
    start_capture(out, pout, "out");
    start_capture(err, perr, "err");

    // main loop
    if (r = uv_run(&loop, UV_RUN_DEFAULT); r)
        errors.push_back("something goes wrong in the loop: "s + uv_strerror(r));

    // cleanup
    auto close = [&loop](auto &stream, auto &pipe, auto &file_req)
    {
        if (!stream.inherit && stream.file.empty())
            uv_close((uv_handle_t*)&pipe, NULL);
        else if (!stream.file.empty())
            uv_fs_close(&loop, &file_req, file_req.result, 0);
    };
    if (child_stdio[0].flags != UV_IGNORE)
        close(in, pin, in_file_req);
    close(out, pout, out_file_req);
    close(err, perr, err_file_req);

    // pipes and process want endgames
    if (r = uv_run(&loop, UV_RUN_DEFAULT); r)
        errors.push_back("something goes wrong in the endgames loop: "s + uv_strerror(r));

    // cleanup loop
    r = ::primitives::detail::uv_loop_close(loop, errors);

    if (exit_code && exit_code.value() == 0)
        return;

    if (ec_in)
    {
        if (exit_code)
            ec_in->assign(exit_code.value(), std::generic_category());
        else // default error
            ec_in->assign(1, std::generic_category());
        return;
    }

    throw std::runtime_error(getError());
}

String Command::getError() const
{
    auto err = "command failed: " + print();
    if (exit_code)
        err += ", exit code = " + std::to_string(exit_code.value());
    for (auto &e : errors)
        err += ", " + e;
    return err;
}

void Command::write(path p) const
{
    auto fn = p.filename().string();
    p = p.parent_path();
    write_file(p / (fn + "_out.txt"), out.text);
    write_file(p / (fn + "_err.txt"), err.text);
}

void Command::execute1(const path &p, const Strings &args, std::error_code *ec)
{
    Command c;
    c.program = p;
    c.args = args;
    c.execute1(ec);
}

void Command::execute(const path &p)
{
    execute1(p);
}

void Command::execute(const path &p, std::error_code &ec)
{
    execute1(p, Strings(), &ec);
}

void Command::execute(const path &p, const Strings &args)
{
    execute1(p, args);
}

void Command::execute(const path &p, const Strings &args, std::error_code &ec)
{
    execute1(p, args, &ec);
}

void Command::execute(const Strings &args)
{
    Command c;
    c.args = args;
    c.execute();
}

void Command::execute(const Strings &args, std::error_code &ec)
{
    Command c;
    c.args = args;
    c.execute(ec);
}

void Command::execute(const std::initializer_list<String> &args)
{
    execute(Strings(args.begin(), args.end()));
}

void Command::execute(const std::initializer_list<String> &args, std::error_code &ec)
{
    execute(Strings(args.begin(), args.end()), ec);
}

}
