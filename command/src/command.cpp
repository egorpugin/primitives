#include <primitives/command.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/process.hpp>

namespace bp = boost::process;

path resolve_executable(const path &p)
{
    return bp::search_path(p);
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

bool Command::execute1(std::error_code *ec)
{
    // setup
    auto p = resolve_executable(program);
    if (p.empty())
    {
        if (!ec)
            throw std::runtime_error("Cannot resolve executable: " + program.string());
        // set error code
        return false;
    }
    program = p;

    if (working_directory.empty())
        working_directory = fs::current_path();

#ifndef _WIN32
    bp::environment env;
    if (use_parent_environment)
        env = boost::this_process::environment();
#endif

    boost::asio::io_service io_service;
    bp::async_pipe p1(io_service);
    bp::async_pipe p2(io_service);

    std::function<void(int, const std::error_code &)> on_exit = [&p1, &p2, this](int exit, const std::error_code &ec)
    {
        exit_code = exit;
        p1.async_close();
        p2.async_close();
    };

    std::unique_ptr<bp::child> c;
    if (ec)
    {
        c = std::make_unique<bp::child>(
            program
            , bp::args = args
            , bp::start_dir = working_directory
#ifndef _WIN32
            , bp::env = env
#endif
            , io_service
            , bp::std_in < stdin
            , bp::std_out > p1
            , bp::std_err > p2
            , bp::on_exit(on_exit)
            , *ec
        );
    }
    else
    {
        c = std::make_unique<bp::child>(
            program
            , bp::args = args
            , bp::start_dir = working_directory
#ifndef _WIN32
            , bp::env = env
#endif
            , io_service
            , bp::std_in < stdin
            , bp::std_out > p1
            , bp::std_err > p2
            , bp::on_exit(on_exit)
            );
    }

    std::function<void(const boost::system::error_code &, std::size_t)> out_cb, err_cb;
    Buffer out_buf(8192), err_buf(8192);
    out_cb = [this, &out_buf, &p1, &out_cb](const boost::system::error_code &ec, std::size_t s)
    {
        if (s)
            out.text.insert(out.text.end(), out_buf.begin(), out_buf.begin() + s);
        if (!ec)
            boost::asio::async_read(p1, boost::asio::buffer(out_buf), out_cb);
    };
    err_cb = [this, &err_buf, &p2, &err_cb](const boost::system::error_code &ec, std::size_t s)
    {
        if (s)
            err.text.insert(err.text.end(), err_buf.begin(), err_buf.begin() + s);
        if (!ec)
            boost::asio::async_read(p2, boost::asio::buffer(err_buf), err_cb);
    };
    boost::asio::async_read(p1, boost::asio::buffer(out_buf), out_cb);
    boost::asio::async_read(p2, boost::asio::buffer(err_buf), err_cb);

    io_service.run();

    return (ec && !*ec || !ec) && exit_code == 0;
}

void Command::write(path p) const
{
    auto fn = p.filename().string();
    p = p.parent_path();
    write_file(p / (fn + "_out.txt"), out.text);
    write_file(p / (fn + "_err.txt"), err.text);
}

bool Command::execute1(const path &p, const Strings &args, std::error_code *ec)
{
    Command c;
    c.program = p;
    c.args = args;
    return c.execute1(ec);
}

bool Command::execute(const path &p)
{
    return execute1(p);
}

bool Command::execute(const path &p, std::error_code &ec)
{
    return execute1(p, Strings(), &ec);
}

bool Command::execute(const path &p, const Strings &args)
{
    return execute1(p, args);
}

bool Command::execute(const path &p, const Strings &args, std::error_code &ec)
{
    return execute1(p, args, &ec);
}

bool Command::execute(const Strings &args)
{
    if (args.empty())
        throw std::runtime_error("Empty program");
    return execute1(args[0], Strings(args.begin() + 1, args.end()));
}

bool Command::execute(const Strings &args, std::error_code &ec)
{
    if (args.empty())
        throw std::runtime_error("Empty program");
    return execute1(args[0], Strings(args.begin() + 1, args.end()), &ec);
}
