#pragma once

#include <primitives/filesystem.h>

struct Command;
using Commands = std::unordered_set<Command*>;

struct Command
{
    using Buffer = std::vector<char>;

    struct Stream
    {
        String text;
        bool capture = true;
        bool inherit = false;
        //std::function<void(const Buffer &)> action;
    };

    // input
    path program;
    Strings args;
    StringMap environment;
    path working_directory;

    // output
    int exit_code = -1;
    //Stream input
    Stream out;
    Stream err;
    // more streams

    // properties
    //int32_t pid = -1;
    bool use_parent_environment = true;

    bool execute() { return execute1(); }
    bool execute(std::error_code &ec) { return execute1(&ec); }
    void write(path p) const;

private:
    bool execute1(std::error_code *ec = nullptr);
};

path resolve_executable(const path &p);
path resolve_executable(const std::vector<path> &paths);
