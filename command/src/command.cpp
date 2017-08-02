#include <primitives/command.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/nowide/convert.hpp>
#include <boost/process.hpp>

#include <iostream>

namespace primitives
{

namespace bp = boost::process;

struct CommandData
{
    std::unique_ptr<boost::asio::io_service> ios;
    std::unique_ptr<boost::process::async_pipe> pout, perr;
    std::unique_ptr<boost::process::child> c;
    using cb_func = std::function<void(const boost::system::error_code &, std::size_t)>;
    cb_func out_cb, err_cb;
    std::function<void(int, const std::error_code &)> on_exit;
    std::function<void(void)> on_error;
    std::function<void(const boost::system::error_code &ec, std::size_t s, Command::Stream &out, boost::process::async_pipe &p, Command::Buffer &out_buf, cb_func &out_cb, std::ostream &stream)> async_read;
    std::atomic_int pipes_closed{ 0 };
    Command::Buffer out_buf, err_buf;
};

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

Command::Command()
{
}

Command::~Command()
{
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

    d = std::make_unique<CommandData>();

    boost::asio::io_service *ios_ptr;
    if (io_service)
        ios_ptr = io_service;
    else
    {
        d->ios = std::make_unique<boost::asio::io_service>();
        ios_ptr = d->ios.get();
    }

    d->pout = std::make_unique<bp::async_pipe>(*ios_ptr);
    d->perr = std::make_unique<bp::async_pipe>(*ios_ptr);

    d->on_error = [this, &ios_ptr]()
    {
        if (d->pipes_closed != 2)
        {
            ios_ptr->post([this] {d->on_error(); });
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

    d->on_exit = [this, ec_in](int exit, const std::error_code &ec)
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

        d->on_error();
    };

    d->async_read = [this](const boost::system::error_code &ec, std::size_t s, auto &out, auto &p, auto &out_buf, auto &&out_cb, auto &stream)
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
        {
            p.async_close();
            d->pipes_closed++;
        }
    };

    // setup buffers also before child creation
    d->out_buf.resize(buf_size);
    d->err_buf.resize(buf_size);
    d->out_cb = [this](const boost::system::error_code &ec, std::size_t s)
    {
        d->async_read(ec, s, out, *d->pout.get(), d->out_buf, d->out_cb, std::cout);
    };
    d->err_cb = [this](const boost::system::error_code &ec, std::size_t s)
    {
        d->async_read(ec, s, err, *d->perr.get(), d->err_buf, d->err_cb, std::cerr);
    };
    boost::asio::async_read(*d->pout.get(), boost::asio::buffer(d->out_buf), d->out_cb);
    boost::asio::async_read(*d->perr.get(), boost::asio::buffer(d->err_buf), d->err_cb);

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
        d->c = std::make_unique<bp::child>(
            program
            , bp::args = wargs
            , bp::start_dir = working_directory
            , bp::std_in < stdin
            , bp::std_out > *d->pout.get()
            , bp::std_err > *d->perr.get()
            , bp::on_exit(d->on_exit)
            , *ios_ptr
            , *ec_in
            );
    }
    else
    {
        d->c = std::make_unique<bp::child>(
            program
            , bp::args = wargs
            , bp::start_dir = working_directory
            , bp::std_in < stdin
            , bp::std_out > *d->pout.get()
            , bp::std_err > *d->perr.get()
            , bp::on_exit(d->on_exit)
            , *ios_ptr
            );
    }

    // perform io only in case we didn't get external io_service
    if (d->ios)
    {
        ios_ptr->run();
    }
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
