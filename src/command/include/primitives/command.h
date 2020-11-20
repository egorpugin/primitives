// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "argument.h"

#include <primitives/filesystem.h>

#include <functional>
#include <optional>

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
    using Argument = command::Argument;
    using Arguments = command::Arguments;

    struct Stream
    {
        String text;
        path file;
        //bool capture = true; // by default - we can make redirect to null by default instead
        bool inherit = false;
        bool append = false;
        std::function<ActionType> action;
        char buf[8192];

    private:
#ifdef BOOST_SERIALIZATION_ACCESS_HPP
        friend class boost::serialization::access;
        template <class Ar>
        void serialize(Ar &ar, unsigned)
        {
            ar & text;
            ar & file;
            ar & append;
        }
#endif
    };

    Arguments arguments; // hide?
    // input
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
    //bool capture = true; // by default - we can make redirect to null by default instead
    bool inherit = false; // inherit everything (out+err)

    // chaining
    Command *prev = nullptr;
    Command *next = nullptr;

public:
    Command();
    virtual ~Command();

    bool isProgramSet() const { return program_set; }
    void setProgram(const path &);
    path getProgram() const;
    virtual Arguments &getArguments();
    virtual const Arguments &getArguments() const;
    void setArguments(const Arguments &);

    virtual void execute() { execute1(); }
    virtual void execute(std::error_code &ec) { execute1(&ec); }

    // immediately stop running command
    void interrupt();

    virtual void onBeforeRun() noexcept {}
    virtual void onEnd() noexcept {}

    virtual path resolveProgram(const path &p) const;

    void write(path p) const;
    String print() const;
    String getError() const;
    Strings getErrors() const { return errors; }

    void push_back(Arguments::Element);
    void push_back(const char *);
    void push_back(const String &);
    void push_back(const path &);
    void push_back(const Arguments &);

    //void setInteractive(bool i);

    // hide? no, but add control over chaining
    // e.g. redirect stderr etc.
    void appendPipeCommand(Command &);
    Command *getFirstCommand() const;

    Command &operator|(Command &);
    Command &operator|=(Command &);

    // env commands
    void addPathDirectory(const path &p);
    // not only PATH may be added with delimiters ([DY]LD_LIBRARY_PATH etc.)
    // value may be an array of values (proper delims must be set for target platform already)
    void appendEnvironmentArrayValue(const String &key, const String &value, bool unique_values = false);

public:
    static void execute(const path &p);
    static void execute(const path &p, std::error_code &ec);
    static void execute(const path &p, const Arguments &);
    static void execute(const path &p, const Arguments &, std::error_code &ec);
    static void execute(const Arguments &);
    static void execute(const Arguments &, std::error_code &ec);
    static void execute(const std::initializer_list<String> &args);
    static void execute(const std::initializer_list<String> &args, std::error_code &ec);

private:
    mutable void *internal_command = nullptr;
    bool program_set = false;
    Strings errors;

    void preExecute(std::error_code *ec);
    void execute1(std::error_code *ec = nullptr);
    std::vector<Command*> execute2(std::error_code *ec);

    static void execute1(const path &p, const Arguments & = {}, std::error_code *ec = nullptr);

#ifdef BOOST_SERIALIZATION_ACCESS_HPP
    friend class boost::serialization::access;
    template <class Ar>
    void serialize(Ar &ar, unsigned)
    {
        ar & working_directory;
        ar & environment;

        ar & in;
        ar & out;
        ar & err;

        //ar & prev;

        ar & arguments;
    }
#endif
};

/// return empty when file not found
PRIMITIVES_COMMAND_API
path resolve_executable(const path &p);

PRIMITIVES_COMMAND_API
path resolve_executable(const FilesOrdered &paths);

}
