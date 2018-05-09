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
    using ActionType = void(const String &, bool);

    struct Stream
    {
        String text;
        //bool capture = true;
        bool inherit = false;
        std::function<ActionType> action;
        path file;
    };

    // input
    path program;
    Strings args;
    StringMap environment;
    path working_directory;

    // output
    optional<int> exit_code;
    //Stream input
    Stream out;
    Stream err;
    // more streams

    // properties
    //int32_t pid = -1;
    size_t buf_size = 8192;
    bool use_parent_environment = true;
    //bool capture = true;
    bool inherit = false;

    Command() = default;
    virtual ~Command() = default;

    virtual void execute() { execute1(); }
    virtual void execute(std::error_code &ec) { execute1(&ec); }
    void write(path p) const;
    String print() const;

    static void execute(const path &p);
    static void execute(const path &p, std::error_code &ec);
    static void execute(const path &p, const Strings &args);
    static void execute(const path &p, const Strings &args, std::error_code &ec);
    static void execute(const Strings &args);
    static void execute(const Strings &args, std::error_code &ec);
    static void execute(const std::initializer_list<String> &args);
    static void execute(const std::initializer_list<String> &args, std::error_code &ec);

private:
    void execute1(std::error_code *ec = nullptr);

    static void execute1(const path &p, const Strings &args = Strings(), std::error_code *ec = nullptr);
};

PRIMITIVES_COMMAND_API
path resolve_executable(const path &p);

PRIMITIVES_COMMAND_API
path resolve_executable(const std::vector<path> &paths);

}