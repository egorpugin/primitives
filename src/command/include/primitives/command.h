// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>
#include <primitives/stdcompat/optional.h>

#include <functional>

namespace primitives
{

struct Command;
using Commands = std::unordered_set<Command*>;

// return value:
//   1. throw if exit_code != 0
//   2. no throw when ec is provided
struct PRIMITIVES_COMMAND_API Command
{
    using ActionType = void(const String &, bool /* eof */);

    struct Stream
    {
        String text;
        path file;
        //bool capture = true; // by default - we can make redirect to null by default instead
        bool inherit = false;
        std::function<ActionType> action;
        char buf[8192];
    };

    // input
    path program;
    Strings args;
    bool inherit_current_evironment = true;
    StringMap<String> environment;
    path working_directory;

    // output
    optional<int64_t> exit_code;
    Stream in;
    Stream out;
    Stream err;
    // more streams

    // properties
    int32_t pid = -1;
    size_t buf_size = 8192;
    bool use_parent_environment = true;
    bool detached = false;
    void *attribute_list = nullptr; // win32 STARTUPINFOEX
    //bool protect_args_with_quotes = true;
    //bool capture = true; // by default - we can make redirect to null by default instead
    bool inherit = false;

    Command();
    virtual ~Command();

    virtual void execute() { execute1(); }
    virtual void execute(std::error_code &ec) { execute1(&ec); }
    void write(path p) const;
    String print() const;
    String getError() const;

    virtual path getProgram() const;

    static void execute(const path &p);
    static void execute(const path &p, std::error_code &ec);
    static void execute(const path &p, const Strings &args);
    static void execute(const path &p, const Strings &args, std::error_code &ec);
    static void execute(const Strings &args);
    static void execute(const Strings &args, std::error_code &ec);
    static void execute(const std::initializer_list<String> &args);
    static void execute(const std::initializer_list<String> &args, std::error_code &ec);

private:
    Strings errors;

    void execute1(std::error_code *ec = nullptr);

    static void execute1(const path &p, const Strings &args = Strings(), std::error_code *ec = nullptr);
};

PRIMITIVES_COMMAND_API
path resolve_executable(const path &p);

PRIMITIVES_COMMAND_API
path resolve_executable(const std::vector<path> &paths);

}
