#include <boost/asio/io_service.hpp>

#include <primitives/command.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/nowide/convert.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/process.hpp>

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>

#ifdef _WIN32
static auto in_mode = 0x01;
static auto out_mode = 0x02;
static auto app_mode = 0x08;
static auto binary_mode = 0x20;
#else
static auto in_mode = std::ios::in;
static auto out_mode = std::ios::out;
static auto app_mode = std::ios::app;
static auto binary_mode = std::ios::binary;
#endif

auto &get_io_service()
{
    static boost::asio::io_service ios;
    return ios;
}

namespace primitives
{

namespace bp = boost::process;

struct CommandData
{
    using cb_func = std::function<void(const boost::system::error_code &, std::size_t)>;

    boost::asio::io_service &ios;
    boost::process::async_pipe pout, perr;

    cb_func out_cb, err_cb;

    std::vector<char> out_buf, err_buf;
    FILE *out_fs = nullptr;
    FILE *err_fs = nullptr;

    std::mutex m;
    std::condition_variable cv;

    CommandData(boost::asio::io_service &ios)
        : ios(ios), pout(ios), perr(ios)
    {
    }

    ~CommandData()
    {
        if (out_fs)
            fclose(out_fs);
        if (err_fs)
            fclose(err_fs);
    }
};

path resolve_executable(const path &p)
{
#ifdef _WIN32
    static const std::vector<String> exts = []
    {
        String s = getenv("PATHEXT");
        boost::to_lower(s);
        std::vector<String> exts;
        boost::split(exts, s, boost::is_any_of(";"));
        return exts;
    }();
    if (p.has_extension() &&
        std::find(exts.begin(), exts.end(), boost::to_lower_copy(p.extension().string())) != exts.end() &&
        fs::exists(p) &&
        fs::is_regular_file(p)
        )
        return p;
    // not test direct file
    if (fs::exists(p) && fs::is_regular_file(p))
        return p;
    // failed, test new exts
    for (auto &e : exts)
    {
        auto p2 = p;
        p2 += e;
        if (fs::exists(p2) && fs::is_regular_file(p2))
            return p2;
    }
#else
    if (fs::exists(p) && fs::is_regular_file(p))
        return p;
#endif
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

String Command::print() const
{
    String s;
    s += "\"" + program.string() + "\" ";
    for (auto &a : args)
        s += "\"" + a + "\" ";
    s.resize(s.size() - 1);
    return s;
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

    // local scope, so we automatically close and destroy everything on exit
    CommandData d(get_io_service());

    // setup buffers
    d.out_buf.resize(buf_size);
    d.err_buf.resize(buf_size);

    if (!out.file.empty())
        d.out_fs = primitives::filesystem::fopen(out.file, "wb");
    if (!err.file.empty() && out.file != err.file)
        d.err_fs = primitives::filesystem::fopen(err.file, "wb");

    auto async_read = [this, &d](const boost::system::error_code &ec, std::size_t s, auto &out, auto &p, auto &out_buf, auto &&out_cb, auto &out_fs, auto &stream)
    {
        if (s)
        {
            String str(out_buf.begin(), out_buf.begin() + s);
            out.text += str;
            if (inherit || out.inherit)
            {
                // ???
                if (!stream)
                    stream.clear();
                stream << str;
            }
            if (out.action)
                out.action(str, ec);
            if (!out.file.empty())
                fwrite(&str[0], str.size(), 1, out_fs);
        }
        if (!ec)
            p.async_read_some(boost::asio::buffer(d.out_buf), out_cb);
        else
        {
            // win32: ec = 109, pipe is ended
            if (p.is_open())
            {
                if (s)
                {
                    //p.async_close();
                    p.close();
                    assert(false && "non zero message with closed pipe");
                    throw std::runtime_error("primitives.command (" + std::to_string(__LINE__) + "): non zero message with closed pipe");
                }
                else
                    p.close();
            }
            // if we somehow hit this w/o p.is_open(),
            // we still consider pipe as closed and notice users
            d.cv.notify_all();
        }
    };

    d.out_cb = [this, &d, &async_read](const boost::system::error_code &ec, std::size_t s)
    {
        async_read(ec, s, out, d.pout, d.out_buf, d.out_cb,
            d.out_fs,
            std::cout);
    };
    d.err_cb = [this, &d, &async_read](const boost::system::error_code &ec, std::size_t s)
    {
        async_read(ec, s, err, d.perr, d.err_buf, d.err_cb,
            out.file != err.file ? d.err_fs : d.out_fs,
            std::cerr);
    };

    d.pout.async_read_some(boost::asio::buffer(d.out_buf), d.out_cb);
    d.perr.async_read_some(boost::asio::buffer(d.err_buf), d.err_cb);

    // run
#ifdef _WIN32
    // widen args
    std::vector<std::wstring> wargs;
    for (auto &a : args)
        wargs.push_back(boost::nowide::widen(a));
#else
    const Strings &wargs = args;
#endif

    auto on_exit = [this, &d](int exit, const std::error_code& ec_in)
    {
        // must be empty, pipes are closed in async_read_some() funcs
        //d.pout.async_close();
        //d.perr.async_close();
    };

    std::error_code ec;
    bp::child c(
        program
        , bp::args = wargs
        , bp::start_dir = working_directory

        , bp::std_in < stdin
        , bp::std_out > d.pout
        , bp::std_err > d.perr

        , ec // always use ec

        , d.ios
        // Without this line boost::process does not work well on linux
        // in current setup.
        // It cannot detect empty pipes.
        , bp::on_exit(on_exit)
    );

    // some critical error during process creation
    if (!c || ec)
    {
        if (ec_in)
        {
            *ec_in = ec;
            return;
        }
        throw std::runtime_error("Last command failed: " + print() + ", ec = " + ec.message());
    }

    using namespace std::literals::chrono_literals;

    auto delay = 100ms;
    const auto max = 1s;
    while (c.running())
    {
        // in case we're done or two pipes were closed, we do not wait in cv anymore
        if (d.ios.poll_one())
        {
            delay = 100ms;
            continue;
        }
        // no jobs available, do a small sleep waiting for process
        delay = delay > 1s ? 1s : delay;
        c.wait_for(delay);
        delay += 100ms;
    }

    //int repeats = 5;
    while (d.pout.is_open() || d.perr.is_open())
    {
        // in case we're done or two pipes were closed, we do not wait in cv anymore
        if (d.ios.poll_one())
        {
            delay = 100ms;
            continue;
        }
        // no jobs available, do a small sleep waiting for pipes closed
        std::unique_lock<std::mutex> lk(d.m);
        delay = delay > 1s ? 1s : delay;
        d.cv.wait_for(lk, delay, [&d] {return !d.pout.is_open() && !d.perr.is_open(); });
        delay += 100ms;

        /*if (repeats-- == 0)
        {
            std::cerr << "looks like a deadlock in primitives.command (boost::process)" << std::endl;
            if (d.pout.is_open())
            {
                auto sz = d.pout.read_some(boost::asio::buffer(d.out_buf));
                int a = 5;
                a++;
            }
            if (d.perr.is_open())
            {
                auto sz = d.perr.read_some(boost::asio::buffer(d.err_buf));
                int a = 5;
                a++;
            }
        }*/
    }

    exit_code = c.exit_code();

    // return if ok
    if (exit_code.value() == 0)
        return;
    else if (ec_in)
    {
        ec_in->assign(exit_code.value(), std::generic_category());
        return;
    }

    throw std::runtime_error("Last command failed: " + print() + ", exit code = " + std::to_string(exit_code.value()));
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
