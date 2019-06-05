// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>
#include <optional>

#include <functional>

namespace primitives
{

struct Command;
using Commands = std::unordered_set<Command*>;

namespace detail
{

struct PRIMITIVES_COMMAND_API Args : Strings
{
    using Strings::Strings;
    using Strings::push_back;
    using Strings::operator=;

    void push_back(const path &p);
};

}

// return value:
//   1. throw if exit_code != 0
//   2. no throw when ec is provided
struct PRIMITIVES_COMMAND_API Command
{
    using ActionType = void(const String &, bool /* eof */);
    using Args = detail::Args;

    struct Stream
    {
        String text;
        path file;
        //bool capture = true; // by default - we can make redirect to null by default instead
        bool inherit = false;
        bool append = false;
        std::function<ActionType> action;
        char buf[8192];
    };

    // input
    path program;
    detail::Args args;
    bool inherit_current_evironment = true;
    StringMap<String> environment;
    path working_directory;

    // output
    std::optional<int64_t> exit_code;
    Stream in;
    Stream out;
    Stream err;
    // more streams
    // generalize to custom fds?

    // properties
    int32_t pid = -1;
    size_t buf_size = 8192;
    bool detached = false;
    bool create_new_console = false;
    void *attribute_list = nullptr; // win32 STARTUPINFOEX
    //bool protect_args_with_quotes = true;
    //bool capture = true; // by default - we can make redirect to null by default instead
    bool inherit = false; // inherit everything (out+err)

    // chaining
    Command *prev = nullptr;
    Command *next = nullptr;

public:
    Command();
    virtual ~Command();

    virtual Args &getArgs() { return args; }
    virtual const Args &getArgs() const { return const_cast<Command*>(this)->getArgs(); }

    virtual void execute() { execute1(); }
    virtual void execute(std::error_code &ec) { execute1(&ec); }

    virtual void onBeforeRun() noexcept {}
    virtual void onEnd() noexcept {}

    virtual path resolveProgram(const path &p) const;

    void write(path p) const;
    String print() const;
    String getError() const;

    virtual path getProgram() const;

    //void setInteractive(bool i);

    // hide? no, but add control over chaining
    // e.g. redirect stderr etc.
    void appendPipeCommand(Command &);

    Command &operator|(Command &);
    Command &operator|=(Command &);

public:
    static void execute(const path &p);
    static void execute(const path &p, std::error_code &ec);
    static void execute(const path &p, const Args &args);
    static void execute(const path &p, const Args &args, std::error_code &ec);
    static void execute(const Args &args);
    static void execute(const Args &args, std::error_code &ec);
    static void execute(const std::initializer_list<String> &args);
    static void execute(const std::initializer_list<String> &args, std::error_code &ec);

private:
    Strings errors;

    void preExecute(std::error_code *ec);
    void execute1(std::error_code *ec = nullptr);
    std::vector<Command*> execute2(std::error_code *ec);

    static void execute1(const path &p, const Args &args = {}, std::error_code *ec = nullptr);
};

/// return empty when file not found
PRIMITIVES_COMMAND_API
path resolve_executable(const path &p);

PRIMITIVES_COMMAND_API
path resolve_executable(const FilesOrdered &paths);

}
