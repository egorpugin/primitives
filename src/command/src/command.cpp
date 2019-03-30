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
#include <boost/process/search_path.hpp>

#include <uv.h>

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>

#ifndef _WIN32
extern char **environ;
#endif

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
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

namespace
{

struct UvCommand
{
    // all strings are utf-8

    class io_data
    {
        enum
        {
            // with priority

            T_EMPTY,

            //
            T_IGNORE,

            // default action is to capture
            T_CAPTURE,

            // redirect to real file
            T_REDIRECT_TO_FILE,

            // sometimes we want to inherit things, highest priority
            // equals to redirect to fd
            T_INHERIT,

            T_REDIRECT_BETWEEN_PROCESSES,
        };

        UvCommand &c;
        int type = T_EMPTY;
        uv_fs_t file_req; // redirect
        uv_pipe_t p; // capture
        uv_pipe_t *p_provided = nullptr; // capture other's stream
        int fd; // inherit
        bool out = true; // direction, rename to read?

        // write (stdin) operations
        uv_buf_t wbuf;
        uv_write_t wreq;

    public:
        io_data(UvCommand &c)
            : c(c)
        {
            clear(T_EMPTY);
        }

        void set_ignore(int fd)
        {
            clear(T_IGNORE);
            this->fd = fd;
        }

        uv_pipe_t *get_redirect_pipe(bool out = true)
        {
            clear(T_REDIRECT_BETWEEN_PROCESSES);

            this->out = out;
            this->p_provided = &p;
            return p_provided;
        }

        void set_redirect_pipe(uv_pipe_t *p_provided, bool out = true)
        {
            clear(T_REDIRECT_BETWEEN_PROCESSES);

            this->out = out;
            this->p_provided = p_provided;
        }

        void set_capture(int fd, primitives::Command::Stream &dst, bool out = true)
        {
            if (!out && dst.text.empty())
                return;

            clear(T_CAPTURE);

            this->out = out;
            this->fd = fd;
            p.data = &dst;
            uv_pipe_init(c.loop, &p, 0);
        }

        void set_redirect(const path &file, bool out = true, bool append = false)
        {
            if (file.empty())
                return;

            clear(T_REDIRECT_TO_FILE);

            this->out = out;
            auto f = file.u8string();

            int flags = O_BINARY;
            if (out)
                flags |= (append ? O_APPEND : O_CREAT) | O_RDWR;
            else
                flags |= O_RDONLY;

            uv_fs_open(c.loop, &file_req, f.c_str(), flags, 0644, NULL);
            uv_fs_req_cleanup(&file_req);

            fd = file_req.result;
        }
        // add set redirect for fd r/w

        void set_inherit(int fd)
        {
            clear(T_INHERIT);
            this->fd = fd;
        }

        void set_type(int t)
        {
            if (t < type)
                throw SW_LOGIC_ERROR("cannot downgrade stdio type from " + std::to_string(type) + " to " + std::to_string(t));
            type = t;
        }

        void clear(int t)
        {
            auto oldt = type;
            set_type(t);

            switch (oldt)
            {
            default:
                break;
            }

            out = true;
            fd = -1;
        }

        void clean() noexcept
        {
            if (c.c.detached)
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
                uv_fs_close(c.loop, &file_req, file_req.result, 0);
#endif
                break;

            default:
                break;
            }
        }

        uv_stdio_container_t get_child_stdio() const
        {
            uv_stdio_container_t s;
            memset(&s, 0, sizeof(s));

            if (c.c.detached)
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

        void start_capture(Strings &errors)
        {
            if (c.c.detached)
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
                wbuf.len = s->text.size();
                wreq.data = this;
                if (auto r = uv_write(&wreq, (uv_stream_t *)&p, &wbuf, 1, on_write); r)
                    errors.push_back("cannot start write from string to fd" + std::to_string(fd) + ": " + uv_strerror(r));
            }
        }

        static void on_alloc(uv_handle_t *s, size_t suggested_size, uv_buf_t *buf)
        {
            auto c = ((primitives::Command::Stream*)s->data);
            buf->base = c->buf;
            buf->len = sizeof(c->buf);
        }

        static void on_read(uv_stream_t *s, ssize_t nread, const uv_buf_t *rdbuf)
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

        static void on_write(uv_write_t *req, int status)
        {
            auto d = (io_data *)req->data;
            if (status == 0)
            {
                uv_close((uv_handle_t*)&d->p, 0);
                return;
            }
        }
    };

    struct io_datas : std::vector<std::unique_ptr<io_data>>
    {
        std::vector<uv_stdio_container_t> get_child_stdio() const
        {
            std::vector<uv_stdio_container_t> c;
            for (auto &i : *this)
                c.push_back(i->get_child_stdio());
            return c;
        }
    };

    uv_loop_t *loop;
    primitives::Command &c;

    // data holders, all strings are utf-8
    String prog;
    String wdir;

    std::vector<char*> uv_args;

    // i/o
    io_datas streams;
    std::vector<uv_stdio_container_t> child_stdio;

    // options
    uv_process_options_t options = { 0 };

    // env
    std::vector<char *> env;
    std::vector<String> env_data;

    // child handle
    uv_process_t child_req = { 0 };

public:
    UvCommand(uv_loop_t *loop, primitives::Command &c)
        : loop(loop), c(c)
    {
        prog = c.program.u8string();
        wdir = c.working_directory.u8string();

        // args
        uv_args.push_back(prog.data()); // this name arg, win only?
        for (const auto &a : c.getArgs())
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
#endif
            //if (!detached)
            options.exit_cb = on_exit;
        }

        // set env
        {
            StringMap<String> env_map;
            if (!c.environment.empty())
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
            for (auto &[k, v] : c.environment)
                env_map[k] = v;
            for (auto &[k, v] : env_map)
                env_data.push_back(k + "=" + v);
            for (auto &e : env_data)
                env.push_back(e.data());
            if (!env.empty() || !c.inherit_current_evironment)
            {
                env.push_back(0);
                options.env = env.data();
            }
        }

        // self pointer
        child_req.data = this;
    }

    // actually might throw in case of oom during errors.push_back, but let's ignore it for now
    void execute(Strings &errors) noexcept
    {
        child_stdio = streams.get_child_stdio();
        options.stdio = child_stdio.data();
        options.stdio_count = child_stdio.size();

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

    void clean() noexcept
    {
        for (auto &s : streams)
            s->clean();
    }

    static void on_exit(uv_process_t *child_req, int64_t exit_status, int term_signal)
    {
        auto c = (UvCommand*)(child_req->data);
        c->c.exit_code = exit_status;
        c->c.onEnd();
        uv_close((uv_handle_t*)child_req, 0);
    }
};

}

namespace primitives
{

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
    return boost::process::search_path(p.wstring()).wstring();
}

path resolve_executable(const FilesOrdered &paths)
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

path Command::resolveProgram(const path &in) const
{
    return resolve_executable(in);
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

void Command::preExecute(std::error_code *ec_in)
{
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
        // remove this? underlying libuv could resolve itself
        auto p = resolveProgram(program);
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

    if (inherit)
        out.inherit = err.inherit = true;
}

std::vector<Command*> Command::execute2(std::error_code *ec_in)
{
    uv_loop_t loop;
    uv_loop_init(&loop);

    std::vector<Command *> cmds_big;
    std::vector<std::unique_ptr<UvCommand>> cmds;

    auto pre_exec = [&ec_in, &cmds, &cmds_big, &loop](auto &c)
    {
        c.preExecute(ec_in);
        if (ec_in && *ec_in)
            return false;
        cmds.push_back(std::make_unique<UvCommand>(&loop, c));
        cmds_big.push_back(&c);
        return true;
    };

    if (!pre_exec(*this))
        return cmds_big;

    auto p = next;
    while (p)
    {
        if (!pre_exec(*p))
            return cmds_big;

        p = p->next;

        auto &c1 = *(cmds.end() - 2);
        auto &c2 = cmds.back();

        auto pipe = c1->streams[1]->get_redirect_pipe();
        c2->streams[0]->set_redirect_pipe(pipe, false);
    }

    for (auto &c : cmds)
        c->execute(errors);

    // main loop
    if (auto r = uv_run(&loop, UV_RUN_DEFAULT); r)
        errors.push_back("something goes wrong in the loop: "s + uv_strerror(r));

    // cleanup
    for (auto &c : cmds)
        c->clean();

    // endgames before dtors
    // pipes and processes want endgames
    if (auto r = uv_run(&loop, UV_RUN_DEFAULT); r)
        errors.push_back("something goes wrong in the endgames loop: "s + uv_strerror(r));

    // cleanup loop, before dtors!
    if (auto r = ::primitives::detail::uv_loop_close(loop, errors); r)
        errors.push_back("error in cmd: loop was not closed");

    return cmds_big;
}

void Command::execute1(std::error_code *ec_in)
{
    if (prev)
        return prev->execute1(ec_in);
        //throw SW_RUNTIME_ERROR("Do not run piped commands manually");

    auto cmds = execute2(ec_in);
    if (ec_in && *ec_in)
        return;

    // TODO: use sync ostream from C++20
    for (auto &e : errors)
    {
        static std::mutex m;
        std::unique_lock lk(m);
        std::cerr << "error in cmd: " << e << std::endl;
    }

    // check all errors in chain
    if (std::all_of(cmds.begin(), cmds.end(), [](auto &c)
        {
            return c->exit_code && *c->exit_code == 0;
        }))
    {
        return;
    }
    // now do detached check only for the first (current) command
    else if (detached)
        return;

    if (ec_in)
    {
        for (auto &c : cmds)
        {
            if (c->exit_code)
            {
                if (c->exit_code.value() == 0)
                    continue;
                ec_in->assign((int)c->exit_code.value(), std::generic_category());
            }
            else // default error
                ec_in->assign(1, std::generic_category());
            return;
        }
    }

    for (auto &c : cmds)
    {
        if (c->exit_code && *c->exit_code == 0)
            continue;
        throw SW_RUNTIME_ERROR(getError());
    }
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

/*void Command::setInteractive(bool i)
{
    // check if we have win32 or console app?
    create_new_console = true;
}*/

void Command::appendPipeCommand(Command &c2)
{
    next = &c2;
    c2.prev = this;
}

Command &Command::operator|(Command &c2)
{
    appendPipeCommand(c2);
    return *this;
}

Command &Command::operator|=(Command &c2)
{
    return *this | c2;
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
