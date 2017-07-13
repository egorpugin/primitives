#pragma once

#include <primitives/filesystem.h>

namespace boost
{
namespace asio
{
class io_service;
}
}

namespace primitives
{

struct Command;
using Commands = std::unordered_set<Command*>;

// return value:
//   1. throw if exit_code != 0
//   2. no throw when ec is provided
struct Command
{
    using Buffer = std::vector<char>;

    struct Stream
    {
        String text;
        //bool capture = true;
        //bool inherit = false;
        std::function<void(const String &)> action;
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
    // to use parent's io service
    boost::asio::io_service *io_service = nullptr;

    virtual ~Command() = default;

    virtual void execute() { execute1(); }
    virtual void execute(std::error_code &ec) { execute1(&ec); }
    void write(path p) const;

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

path resolve_executable(const path &p);
path resolve_executable(const std::vector<path> &paths);

}
