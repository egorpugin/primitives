// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/asio/io_context.hpp>

#include <primitives/command.h>
#include <primitives/exceptions.h>
#include <primitives/file_monitor.h>
#include <primitives/templates.h>

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

#ifdef _WIN32
#include <fcntl.h>
#endif

#ifdef _WIN32
// Returns length of resulting string, excluding null-terminator.
// Use LocalFree() to free the buffer when it is no longer needed.
// Returns 0 upon failure, use GetLastError() to get error details.
String FormatNtStatus(NTSTATUS nsCode)
{
    // Get handle to ntdll.dll.
    HMODULE hNtDll = LoadLibrary("NTDLL.DLL");

    // Check for fail, user may use GetLastError() for details.
    if (hNtDll == NULL)
        return {};

    SCOPE_EXIT
    {
        // Free loaded dll module and decrease its reference count.
        FreeLibrary(hNtDll);
    };

    TCHAR *ppszMessage;

    typedef LONG NTSTATUS;
    using RtlNtStatusToDosErrorType = ULONG(*)(NTSTATUS Status);

    auto RtlNtStatusToDosError = (RtlNtStatusToDosErrorType)GetProcAddress(hNtDll, "RtlNtStatusToDosError");

    // Call FormatMessage(), note use of RtlNtStatusToDosError().
    DWORD dwRes = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
        hNtDll, RtlNtStatusToDosError(nsCode), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&ppszMessage, 0, NULL);

    if (!dwRes)
        return {};

    if (!ppszMessage)
        return {};

    String s = ppszMessage;
    LocalFree(ppszMessage);

    return s;
}
#endif

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
    for (auto &a : getArgs())
    {
        s += "\"" + a + "\"";
        s += " ";
    }
    s.resize(s.size() - 1);
    return s;
}

path Command::getProgram() const
{
    if (!program.empty())
        return program;
    if (getArgs().empty())
        throw SW_RUNTIME_ERROR("No program was set");
    return getArgs()[0];
}

void Command::execute1(std::error_code *ec_in)
{
    // setup

    // try to use first arg as a program
    if (program.empty())
    {
        if (getArgs().empty())
            throw SW_RUNTIME_ERROR("No program was set");
        program = getArgs()[0];
        auto t = std::move(getArgs());
        getArgs().assign(t.begin() + 1, t.end());
    }

    // resolve exe
    {
        auto p = resolve_executable(program);
        if (p.empty())
        {
            auto e = "Cannot resolve executable: " + program.u8string();
            if (!ec_in)
                throw SW_RUNTIME_ERROR(e);
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
    for (const auto &a : getArgs())
        uv_args.push_back((char *)a.data());
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
#if UV_VERSION_MAJOR > 1
        // FIXME:
        throw std::logic_error("FIXME: " __FILE__);
        child_stdio[fd].data.file = uv_get_osfhandle(fd);
#endif
        if (stream.inherit)
        {
            child_stdio[fd].flags = UV_INHERIT_FD;
#if UV_VERSION_MAJOR > 1
            child_stdio[fd].data.file = uv_get_osfhandle(fd);
#else
            child_stdio[fd].data.fd = fd;
#endif
        }
        else if (!stream.file.empty())
        {
            if (out)
            {
                uv_fs_open(&loop, &file_req, file.c_str(), O_CREAT | O_RDWR, 0644, NULL);
                uv_fs_req_cleanup(&file_req);
                child_stdio[fd].flags = UV_INHERIT_FD;
#if UV_VERSION_MAJOR > 1
                child_stdio[fd].data.file = uv_get_osfhandle(file_req.file);
#else
                child_stdio[fd].data.fd = file_req.result;
#endif
            }
            else
            {
                //uv_pipe_init(&loop, &pipe, 0);
                uv_fs_open(&loop, &file_req, file.c_str(), O_RDONLY
#ifdef _WIN32
                           | O_BINARY
#endif
                           , 0644, NULL);
                uv_fs_req_cleanup(&file_req);
                child_stdio[fd].flags = UV_INHERIT_FD;//(uv_stdio_flags)(UV_CREATE_PIPE | UV_READABLE_PIPE);
#if UV_VERSION_MAJOR > 1
                child_stdio[fd].data.file = uv_get_osfhandle(file_req.result);
#else
                child_stdio[fd].data.fd = file_req.result;
#endif
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
    auto on_exit = [](uv_process_t *child_req, int64_t exit_status, int term_signal)
    {
        auto c = (Command*)(child_req->data);
        c->exit_code = exit_status;
        c->onEnd();
        uv_close((uv_handle_t*)child_req, 0);
    };

    // options
    uv_process_options_t options = { 0 };
    options.exit_cb = on_exit;
    options.file = prog.c_str();
    options.cwd = wdir.c_str();
    options.args = uv_args.data();
    options.stdio = child_stdio;
    options.stdio_count = 3;
//#if UV_VERSION_MAJOR > 1
    options.attribute_list = attribute_list;
//#endif
    if (detached)
        options.flags |= UV_PROCESS_DETACHED;

    // set env
    std::vector<char *> env;
    std::vector<String> env_data;
    StringMap<String> env_map;
    if (!environment.empty())
    {
#ifdef _WIN32
        auto lpvEnv = GetEnvironmentStrings();
        if (!lpvEnv)
            throw SW_RUNTIME_ERROR("GetEnvironmentStrings failed: " + std::to_string(GetLastError()));

        auto lpszVariable = (LPTSTR)lpvEnv;
        while (*lpszVariable)
        {
            String s(lpszVariable);
            env_map[s.substr(0, s.find('='))] = s.substr(s.find('=') + 1);
            lpszVariable += lstrlen(lpszVariable) + 1;
        }
        FreeEnvironmentStrings(lpvEnv);
#else
        for (int i = 0; environ[i]; i++)
        {
            String s(environ[i]);
            env_map[s.substr(0, s.find('='))] = s.substr(s.find('=') + 1);
        }
#endif
    }
    for (auto &[k, v] : environment)
        env_map[k] = v;
    for (auto &[k, v] : env_map)
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

    onBeforeRun();

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

    if (detached)
        uv_unref((uv_handle_t*)&child_req);

    // main loop
    if (r = uv_run(&loop, UV_RUN_DEFAULT); r)
        errors.push_back("something goes wrong in the loop: "s + uv_strerror(r));

    // cleanup
    auto close = [&loop](auto &stream, auto &pipe, auto &file_req)
    {
        if (!stream.inherit && stream.file.empty())
            uv_close((uv_handle_t*)&pipe, NULL);
        else if (!stream.file.empty())
#if UV_VERSION_MAJOR > 1
            uv_fs_close(&loop, &file_req, uv_get_osfhandle(file_req.result), 0);
#else
            uv_fs_close(&loop, &file_req, file_req.result, 0);
#endif
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
    for (auto &e : errors)
        std::cerr << "error in cmd: " << e << std::endl;
    if (r)
        std::cerr << "error in cmd: loop was not closed" << std::endl;

    if (exit_code && exit_code.value() == 0)
        return;

    if (ec_in)
    {
        if (exit_code)
            ec_in->assign((int)exit_code.value(), std::generic_category());
        else // default error
            ec_in->assign(1, std::generic_category());
        return;
    }

    throw SW_RUNTIME_ERROR(getError());
}

String Command::getError() const
{
    auto err = "command failed: " + print();
    if (exit_code)
        err += ", exit code = " + std::to_string(exit_code.value());
#ifdef _WIN32
    if (exit_code > 256)
    {
        std::ostringstream stream;
        stream << "0x" << std::hex << std::uppercase << exit_code.value();
        err += " (" + stream.str() + ")";
        auto e = FormatNtStatus((NTSTATUS)exit_code.value());
        if (!e.empty())
            err += ": " + e;
    }
#endif
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
