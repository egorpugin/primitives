#include <primitives/command.h>

#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>

#include <iostream>
#include <thread>

#include <primitives/log.h>
DECLARE_STATIC_LOGGER(logger, "command");

#ifndef _WIN32
#if defined(__APPLE__) && defined(__DYNAMIC__)
extern "C" { extern char ***_NSGetEnviron(void); }
#   define environ (*_NSGetEnviron())
#else
#   include <unistd.h>
#endif
#endif

optional<path> resolve_executable(const path &p)
try
{
    return boost::process::find_executable_in_path(p.string());
}
catch (fs::filesystem_error &)
{
    return boost::none;
}

optional<path> resolve_executable(const std::vector<path> &paths)
{
    for (auto &p : paths)
    {
        if (auto e = resolve_executable(p))
            return e;
    }
    return boost::none;
}

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

    if (!fs::exists(args_fixed[0]) || args_fixed[0].find_first_of("\\/") == std::string::npos)
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
        LOG_DEBUG(logger, "executing command: " << s);
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

void Result::write(path p) const
{
    auto fn = p.filename().string();
    p = p.parent_path();
    write_file(p / (fn + "_out.txt"), out);
    write_file(p / (fn + "_err.txt"), err);
}

} // namespace command
