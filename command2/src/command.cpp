#include <primitives/command.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/process.hpp>

#include <primitives/log.h>
DECLARE_STATIC_LOGGER(logger, "command");

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

/*
namespace command
{

class stream_reader
{
public:
    stream_reader(boost::process::pistream &in, std::ostream &out, std::string &buffer, const Options::Stream &opts)
        : in(in), out(out), buffer(buffer), opts(opts)
    {
        t = std::move(std::thread([&t = *this] { t(); }));
    }

    ~stream_reader()
    {
        t.join();
        in.close();
    }

private:
    std::thread t;
    boost::process::pistream &in;
    std::ostream &out;
    std::string &buffer;
    const Options::Stream &opts;

    void operator()()
    {
        std::string s;
        while (std::getline(in, s))
        {
            boost::trim(s);

            // before newline
            if (opts.action)
                opts.action(s);

            s += "\n";
            if (opts.capture)
                buffer += s;
            if (opts.inherit)
                out << s;
        }
    }
};

Result execute(const Args &args, const Options &opts)
{
    using namespace boost::process;

    auto args_fixed = args;

	if (!fs::is_regular_file(args_fixed[0]) || args_fixed[0].find_first_of("\\/") == std::string::npos)
	{
		auto e = resolve_executable(args_fixed[0]);
		if (!e)
			throw std::runtime_error("Program '" + args_fixed[0] + "' not found");
		args_fixed[0] = e.get().string();
	}

#ifdef _WIN32
    boost::algorithm::replace_all(args_fixed[0], "/", "\\");
#endif

    // log
    {
        std::string s;
        for (auto &a : args_fixed)
            s += a + " ";
        s.resize(s.size() - 1);
        LOG_TRACE(logger, "executing command: " << s);
    }

    context ctx;
    ctx.stdin_behavior = inherit_stream();

    auto set_behavior = [](auto &stream_behavior, auto &opts)
    {
        if (opts.capture)
            stream_behavior = capture_stream();
        else if (opts.inherit)
            stream_behavior = inherit_stream();
        else
            stream_behavior = silence_stream();
    };

    set_behavior(ctx.stdout_behavior, opts.out);
    set_behavior(ctx.stderr_behavior, opts.err);

    // copy env
#if !defined(_WIN32)
    auto env = environ;
    while (*env)
    {
        std::string s = *env;
        auto p = s.find("=");
        if (p != s.npos)
            ctx.environment[s.substr(0, p)] = s.substr(p + 1);
        env++;
    }
#endif

    Result r;

    try
    {
        child c = boost::process::launch(args_fixed[0], args_fixed, ctx);

        std::unique_ptr<stream_reader> rdout, rderr;

        // run only if captured
        // inherit will be automatically done by boost::process
#define RUN_READER(x) \
        if (opts.x.capture) \
            rd ## x = std::make_unique<stream_reader>(c.get_std ## x(), std::c ## x, r.x, opts.x)

        RUN_READER(out);
        RUN_READER(err);

        r.rc = c.wait().exit_status();

        if (r.rc)
            LOG_WARN(logger, "Command exited with non zero status: " << args_fixed[0] << ", rc = " << r.rc);
    }
    catch (std::exception &e)
    {
        LOG_FATAL(logger, "Command failed: " << args_fixed[0] << ": " << e.what());
        throw;
    }

    return r;
}

Result execute_and_capture(const Args &args, const Options &options)
{
    auto opts = options;
    opts.out.capture = true;
    opts.err.capture = true;
    return execute(args, opts);
}

Result execute_with_output(const Args &args, const Options &options)
{
    auto opts = options;
    opts.out.inherit = true;
    opts.err.inherit = true;
    return execute(args, opts);
}

} // namespace command
*/
