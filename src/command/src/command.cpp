// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#endif

#include <primitives/command.h>

#include "uv_command.h"

#include <primitives/file_monitor.h>
#include <primitives/exceptions.h>
#include <primitives/templates.h>

#include <boost/algorithm/string.hpp>
#include <boost/process/search_path.hpp>

#include <uv.h>

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>

#ifdef _WIN32
// Returns length of resulting string, excluding null-terminator.
// Use LocalFree() to free the buffer when it is no longer needed.
// Returns 0 upon failure, use GetLastError() to get error details.
static String FormatNtStatus(NTSTATUS nsCode)
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

path resolve_executable(const path &p)
{
    // fast path for existing files
    if (fs::exists(p) && fs::is_regular_file(p))
    {
        if (p.is_absolute())
            return p;
        return fs::absolute(p);
    }
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

void Command::push_back(Arguments::Element in)
{
    arguments.push_back(std::move(in));
}

void Command::push_back(const char *in)
{
    arguments.push_back(in);
}

void Command::push_back(const String &in)
{
    arguments.push_back(in);
}

void Command::push_back(const path &in)
{
    arguments.push_back(in);
}

void Command::push_back(const Arguments &in)
{
    arguments.push_back(in);
}

void Command::setProgram(const path &p)
{
    auto np = normalize_path(p);
    if (program_set)
    {
        getArguments()[0] = std::make_unique<command::SimpleArgument>(np);
        return;
    }
    getArguments().push_front(np);
    program_set = true;
}

String Command::getProgram() const
{
    if (!program_set && getArguments().empty())
        throw SW_RUNTIME_ERROR("program is not set");
    return getArguments()[0]->toString();
}

Command::Arguments &Command::getArguments()
{
    return arguments;
}

const Command::Arguments &Command::getArguments() const
{
    return arguments;
}

void Command::setArguments(const Arguments &args)
{
    arguments = args;
}

path Command::resolveProgram(const path &in) const
{
    return resolve_executable(in);
}

String Command::print() const
{
    if (prev)
        return prev->print();

    auto prnt = [](auto &c)
    {
        String s;
        for (auto &a : c.getArguments())
        {
            s += a->quote();
            s += " ";
        }
        if (!s.empty())
            s.resize(s.size() - 1);
        return s;
    };

    auto s = prnt(*this);
    auto p = next;
    while (p)
    {
        s += " | " + prnt(*p);
        p = p->next;
    }
    return s;
}

void Command::preExecute(std::error_code *ec_in)
{
    // resolve exe
    {
        // remove this? underlying libuv could resolve itself
        auto p = resolveProgram(fs::u8path(getProgram()));
        if (p.empty())
        {
            auto e = "Cannot resolve executable: " + getProgram();
            if (!ec_in)
                throw SW_RUNTIME_ERROR(e);
            *ec_in = std::make_error_code(std::errc::no_such_file_or_directory);
            err.text = e;
            return;
        }
        program_set = true;
        setProgram(p);
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
    std::vector<std::unique_ptr<::primitives::command::detail::UvCommand>> cmds;

    auto pre_exec = [&ec_in, &cmds, &cmds_big, &loop](auto &c)
    {
        c.preExecute(ec_in);
        if (ec_in && *ec_in)
            return false;
        cmds.push_back(std::make_unique<::primitives::command::detail::UvCommand>(&loop, c));
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

    // after execute
    internal_command = cmds[0].get();

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

    // cleanup ptr
    internal_command = nullptr;

    return cmds_big;
}

void Command::interrupt()
{
    if (prev)
        return prev->interrupt();
    if (pid == -1)
        throw SW_RUNTIME_ERROR("Command is not running (pid)");
    if (!internal_command)
        throw SW_RUNTIME_ERROR("Command is not running (internal command was not set)");

    ((::primitives::command::detail::UvCommand*)internal_command)->interrupt();
}

static String getStringFromErrorCode(int exit_code)
{
    String err = "exit code = " + std::to_string(exit_code);
#ifdef _WIN32
    if (exit_code > 256 || exit_code < 0)
    {
        std::ostringstream stream;
        stream << "0x" << std::hex << std::uppercase << exit_code;
        err += " (" + stream.str() + ")";
        auto e = FormatNtStatus((NTSTATUS)exit_code);
        if (!e.empty())
            err += ": " + e;
    }
#endif
    return err;
}

void Command::execute1(std::error_code *ec_in)
{
    if (prev)
        return prev->execute1(ec_in);
        //throw SW_RUNTIME_ERROR("Do not run piped commands manually");

    auto cmds = execute2(ec_in);
    if (ec_in && *ec_in)
        return;

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
    {
        return;
    }

    if (ec_in)
    {
        for (auto &c : cmds)
        {
            if (c->exit_code)
            {
                if (c->exit_code.value() == 0)
                    continue;
                ec_in->assign((int)c->exit_code.value(), std::generic_category());

                // prepend exit code
                errors.insert(errors.begin(), getStringFromErrorCode((int)c->exit_code.value()));
            }
            else // default error
                ec_in->assign(1, std::generic_category());
            return;
        }
    }

    // prepend exit code
    if (exit_code)
        errors.insert(errors.begin(), getStringFromErrorCode((int)exit_code.value()));

    throw SW_RUNTIME_ERROR(getError());
}

String Command::getError() const
{
    auto err = "command failed: " + print();
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

Command *Command::getFirstCommand() const
{
    auto p = prev;
    while (p && p->prev)
        p = p->prev;
    return p;
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

static String getSystemRoot()
{
    static const String sr = []()
    {
        auto e = getenv("SystemRoot");
        if (!e)
            throw SW_RUNTIME_ERROR("getenv() failed");
        return e;
    }();
    return sr;
}

static String getSystemPath()
{
    // explicit! Path may be changed by IDE, other sources, so we keep it very small and very system
    static const String sp = []()
    {
        auto r = getSystemRoot();
        // let users provide other paths than System32 themselves?
        //                                        remove?                   remove?
        return r + "\\System32;" + r + ";" + r + "\\System32\\Wbem;" + r + "\\System32\\WindowsPowerShell\\v1.0\\";
    }();
    return sp;
}

void Command::addPathDirectory(const path &p)
{
    // Windows uses 'Path' not 'PATH', but we handle it in underlying primitives::Command.
    static const auto env = "PATH";
#ifdef _WIN32
    auto norm = [](const auto &p) { return normalize_path_windows(p); };
#else
    auto norm = [](const auto &p) { return p.u8string(); };
#endif

    if (environment[env].empty())
    {
#ifdef _WIN32
        environment[env] = getSystemPath();
#else
        auto e = getenv(env);
        if (!e)
            throw SW_RUNTIME_ERROR("getenv() failed");
        environment[env] = e;
#endif
    }

    // now add without duplication
    appendEnvironmentArrayValue(env, to_string(norm(p)), true);
}

void Command::appendEnvironmentArrayValue(const String &key, const String &value, bool unique_values)
{
    static const auto delim =
#ifdef _WIN32
        ";"
#else
        ":"
#endif
        ;

    auto add = [](auto &e, auto &value)
    {
        if (!e.empty())
            e += delim;
        e += value;
    };

    auto &e = environment[key];
    if (!unique_values)
        return add(e, value);

    auto values = split_string(e, delim);
    auto in_values = split_string(value, delim);
    for (auto &v : in_values)
    {
        if (std::find(values.begin(), values.end(), v) == values.end())
            add(e, v);
    }
}

void Command::execute1(const path &p, const Arguments &args, std::error_code *ec)
{
    Command c;
    c.setProgram(p);
    for (auto &a : args)
        c.push_back(a->clone());
    c.execute1(ec);
}

void Command::execute(const path &p)
{
    execute1(p);
}

void Command::execute(const path &p, std::error_code &ec)
{
    execute1(p, {}, &ec);
}

void Command::execute(const path &p, const Arguments &args)
{
    execute1(p, args);
}

void Command::execute(const path &p, const Arguments &args, std::error_code &ec)
{
    execute1(p, args, &ec);
}

void Command::execute(const Arguments &args)
{
    Command c;
    c.arguments = args;
    c.execute();
}

void Command::execute(const Arguments &args, std::error_code &ec)
{
    Command c;
    c.arguments = args;
    c.execute(ec);
}

void Command::execute(const std::initializer_list<String> &args)
{
    execute(Arguments(args));
}

void Command::execute(const std::initializer_list<String> &args, std::error_code &ec)
{
    execute(Arguments(args), ec);
}

}
