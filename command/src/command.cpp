#include <primitives/command.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/process.hpp>

namespace primitives
{

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

void Command::execute1(std::error_code *ec_in)
{
    // setup

    // try to use first arg as a program
    if (program.empty())
    {
        if (args.empty())
            throw std::runtime_error("No program was set");
        program = args[0];
        auto t = std::move(args);
        args.assign(t.begin() + 1, t.end());
    }

    // resolve exe
    auto p = resolve_executable(program);
    if (p.empty())
    {
        if (!ec_in)
            throw std::runtime_error("Cannot resolve executable: " + program.string());
        *ec_in = std::make_error_code(std::errc::no_such_file_or_directory);
        return;
    }
    program = p;

    if (working_directory.empty())
        working_directory = fs::current_path();

#ifndef _WIN32
    bp::environment env;
    if (use_parent_environment)
        env = boost::this_process::environment();
#endif

    std::unique_ptr<boost::asio::io_service> ios;
    boost::asio::io_service *ios_ptr;
    if (io_service)
        ios_ptr = io_service;
    else
    {
        ios = std::make_unique<boost::asio::io_service>();
        ios_ptr = ios.get();
    }

    bp::async_pipe p1(*ios_ptr);
    bp::async_pipe p2(*ios_ptr);

    std::function<void(int, const std::error_code &)> on_exit = [&p1, &p2, this, ec_in](int exit, const std::error_code &ec)
    {
        exit_code = exit;

        // return if ok
        if (exit_code == 0)
            return;

        // set ec, if passed
        if (ec_in)
        {
            if (ec)
                *ec_in = ec;
            else
                ec_in->assign(1, std::generic_category());
            return;
        }

        // throw now
        String s;
        s += "\"" + program.string() + "\" ";
        for (auto &a : args)
            s += "\"" + a + "\" ";
        s.resize(s.size() - 1);
        throw std::runtime_error("Last command failed: " + s + ", exit code = " + std::to_string(exit_code));
    };

    auto async_read = [](const boost::system::error_code &ec, std::size_t s, auto &out, auto &p, auto &out_buf, auto &&out_cb, auto &out_line)
    {
        if (s)
        {
            String str(out_buf.begin(), out_buf.begin() + s);
            out.text += str;
            if (out.action)
                out.action(str, ec);
        }
        if (!ec)
            boost::asio::async_read(p, boost::asio::buffer(out_buf), out_cb);
        else
            p.async_close();
    };

    // setup buffers also before child creation
    std::function<void(const boost::system::error_code &, std::size_t)> out_cb, err_cb;
    Buffer out_buf(8192), err_buf(8192);
    String out_line, err_line;
    out_cb = [this, &out_buf, &p1, &out_cb, &async_read, &out_line](const boost::system::error_code &ec, std::size_t s)
    {
        async_read(ec, s, out, p1, out_buf, out_cb, out_line);
    };
    err_cb = [this, &err_buf, &p2, &err_cb, &async_read, &err_line](const boost::system::error_code &ec, std::size_t s)
    {
        async_read(ec, s, err, p2, err_buf, err_cb, err_line);
    };
    boost::asio::async_read(p1, boost::asio::buffer(out_buf), out_cb);
    boost::asio::async_read(p2, boost::asio::buffer(err_buf), err_cb);

    // run
    std::unique_ptr<bp::child> c;
    if (ec_in)
    {
        c = std::make_unique<bp::child>(
            program
            , bp::args = args
            , bp::start_dir = working_directory
#ifndef _WIN32
            , bp::env = env
#endif
            , *ios_ptr
            , bp::std_in < stdin
            , bp::std_out > p1
            , bp::std_err > p2
            , bp::on_exit(on_exit)
            , *ec_in
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
            , *ios_ptr
            , bp::std_in < stdin
            , bp::std_out > p1
            , bp::std_err > p2
            , bp::on_exit(on_exit)
            );
    }

    // perform io only in case we didn't get external io_service
    if (ios)
        ios_ptr->run();
}

void Command::write(path p) const
{
    auto fn = p.filename().string();
    p = p.parent_path();
    write_file(p / (fn + "_out.txt"), out.text);
    write_file(p / (fn + "_err.txt"), err.text);
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
