#include <primitives/command.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/nowide/convert.hpp>

#include <iostream>

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
    {
        auto p = resolve_executable(program);
        if (p.empty())
        {
            if (!ec_in)
                throw std::runtime_error("Cannot resolve executable: " + program.string());
            *ec_in = std::make_error_code(std::errc::no_such_file_or_directory);
            return;
        }
        program = p;
    }

    if (working_directory.empty())
        working_directory = fs::current_path();

    boost::asio::io_service *ios_ptr;
    if (io_service)
        ios_ptr = io_service;
    else
    {
        ios = std::make_unique<boost::asio::io_service>();
        ios_ptr = ios.get();
    }

    pout = std::make_unique<bp::async_pipe>(*ios_ptr);
    perr = std::make_unique<bp::async_pipe>(*ios_ptr);

    on_exit = [this, ec_in](int exit, const std::error_code &ec)
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

    async_read = [this](const boost::system::error_code &ec, std::size_t s, auto &out, auto &p, auto &out_buf, auto &&out_cb, auto &stream)
    {
        if (s)
        {
            String str(out_buf.begin(), out_buf.begin() + s);
            out.text += str;
            if (inherit || out.inherit)
                stream << str;
            if (out.action)
                out.action(str, ec);
        }
        if (!ec)
            boost::asio::async_read(p, boost::asio::buffer(out_buf), out_cb);
        else
            p.async_close();
    };

    // setup buffers also before child creation
    out_buf.resize(buf_size);
    err_buf.resize(buf_size);
    out_cb = [this](const boost::system::error_code &ec, std::size_t s)
    {
        async_read(ec, s, out, *pout.get(), out_buf, out_cb, std::cout);
    };
    err_cb = [this](const boost::system::error_code &ec, std::size_t s)
    {
        async_read(ec, s, err, *perr.get(), err_buf, err_cb, std::cerr);
    };
    boost::asio::async_read(*pout.get(), boost::asio::buffer(out_buf), out_cb);
    boost::asio::async_read(*perr.get(), boost::asio::buffer(err_buf), err_cb);

#ifdef _WIN32
    // widen args
    std::vector<std::wstring> wargs;
    for (auto &a : args)
        wargs.push_back(boost::nowide::widen(a));
#else
    const Strings &wargs = args;
#endif

    // run
    if (ec_in)
    {
        c = std::make_unique<bp::child>(
            program
            , bp::args = wargs
            , bp::start_dir = working_directory
            , bp::std_in < stdin
            , bp::std_out > *pout.get()
            , bp::std_err > *perr.get()
            , bp::on_exit(on_exit)
            , *ios_ptr
            , *ec_in
            );
    }
    else
    {
        c = std::make_unique<bp::child>(
            program
            , bp::args = wargs
            , bp::start_dir = working_directory
            , bp::std_in < stdin
            , bp::std_out > *pout.get()
            , bp::std_err > *perr.get()
            , bp::on_exit(on_exit)
            , *ios_ptr
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
