#pragma once

#include <primitives/filesystem.h>
#include <boost/process.hpp>

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
    using ActionType = void(const String &, bool);

    struct Stream
    {
        String text;
        //bool capture = true;
        bool inherit = false;
        std::function<ActionType> action;
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
    size_t buf_size = 8192;
    bool use_parent_environment = true;
    //bool capture = true;
    bool inherit = false;
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
    std::unique_ptr<boost::asio::io_service> ios;
    std::unique_ptr<boost::process::async_pipe> pout, perr;
    std::unique_ptr<boost::process::child> c;
    using cb_func = std::function<void(const boost::system::error_code &, std::size_t)>;
    cb_func out_cb, err_cb;
    std::function<void(int, const std::error_code &)> on_exit;
    std::function<void(const boost::system::error_code &ec, std::size_t s, Stream &out, boost::process::async_pipe &p, Buffer &out_buf, cb_func &out_cb, std::ostream &stream)> async_read;
    Buffer out_buf, err_buf;

    void execute1(std::error_code *ec = nullptr);

    static void execute1(const path &p, const Strings &args = Strings(), std::error_code *ec = nullptr);
};

path resolve_executable(const path &p);
path resolve_executable(const std::vector<path> &paths);

}
