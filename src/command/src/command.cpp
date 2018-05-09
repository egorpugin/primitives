// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/asio/io_service.hpp>

#include <primitives/command.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/nowide/convert.hpp>
#include <boost/process.hpp>

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>

auto &get_io_context()
{
    static boost::asio::io_context io_context;
    // protect context from being stopped
    static auto wg = boost::asio::make_work_guard(io_context);
    return io_context;
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

    // TODO: add pid and option to command to execute callbacks right in this thread

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
    // do not test direct file
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
    return bp::search_path(p.wstring()).wstring();
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

String Command::print() const
{
    String s;
    s += "\"" + program.u8string() + "\" ";
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
            auto e = "Cannot resolve executable: " + program.u8string();
            if (!ec_in)
                throw std::runtime_error(e);
            *ec_in = std::make_error_code(std::errc::no_such_file_or_directory);
            err.text = e;
            return;
        }
        program = p;
    }

    if (working_directory.empty())
        working_directory = current_thread_path();

#ifdef _WIN32
    // widen args
    std::vector<std::wstring> wargs;
    for (auto &a : args)
        wargs.push_back(boost::nowide::widen(a));
#else
    const Strings &wargs = args;
#endif

    // local scope, so we automatically close and destroy everything on exit
    CommandData d(get_io_context());

    auto async_read = [this, &cv = d.cv, &d](const boost::system::error_code &ec, std::size_t s,
        auto &out, auto &p, auto &out_buf, auto &&out_cb, auto &out_fs, auto &stream)
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
                out.action(str, !!ec);
            if (!out.file.empty())
                fwrite(&str[0], str.size(), 1, out_fs);
        }

        if (!ec)
        {
            p.async_read_some(boost::asio::buffer(out_buf), out_cb);
        }
        else
        {
            // win32: ec = 109, pipe is ended
            if (p.is_open())
            {
                if (s)
                {
                    p.async_close();
                    p.close();
                    assert(false && "non zero message with closed pipe");
                    throw std::runtime_error("primitives.command (" + std::to_string(__LINE__) + "): non zero message with closed pipe");
                }
                else
                    p.close();
            }
            // if we somehow hit this w/o p.is_open(),
            // we still consider pipe as closed and notice users
            cv.notify_all();
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

    // setup buffers
    d.out_buf.resize(buf_size);
    d.err_buf.resize(buf_size);

    // move this after process start?
    // make a copy there below?
    d.pout.async_read_some(boost::asio::buffer(d.out_buf), d.out_cb);
    d.perr.async_read_some(boost::asio::buffer(d.err_buf), d.err_cb);

    // run
    auto on_exit = [](int exit, const std::error_code& ec_in)
    {
        // must be empty, pipes are closed in async_read_some() funcs
        //d.pout.async_close();
        //d.perr.async_close();
    };

    std::error_code ec;
    auto error = [this, &ec_in, &ec]()
    {
        if (ec_in)
        {
            *ec_in = ec;
            return;
        }
        throw std::runtime_error("Last command failed: " + print() + ", ec = " + ec.message());
    };

    bp::child c;
    if (!out.file.empty() || !err.file.empty())
    {
        if (!out.file.empty() && !err.file.empty())
        {
            c = bp::child(
                program.wstring()
                , bp::args = wargs
                , bp::start_dir = working_directory.wstring()

                , bp::std_in < stdin
                , bp::std_out > out.file
                , bp::std_err > err.file

                , ec // always use ec
            );
            if (ec)
                error();
            c.wait();
            return;
        }
        else if (!out.file.empty())
        {
            c = bp::child(
                program.wstring()
                , bp::args = wargs
                , bp::start_dir = working_directory.wstring()

                , bp::std_in < stdin
                , bp::std_out > out.file
                , bp::std_err > d.perr

                , ec // always use ec

                , d.ios
                // Without this line boost::process does not work well on linux
                // in current setup.
                // It cannot detect empty pipes.
                , bp::on_exit(on_exit)
            );
            d.pout.close();
        }
        else if (!err.file.empty())
        {
            c = bp::child(
                program.wstring()
                , bp::args = wargs
                , bp::start_dir = working_directory.wstring()

                , bp::std_in < stdin
                , bp::std_out > d.pout
                , bp::std_err > err.file

                , ec // always use ec

                , d.ios
                // Without this line boost::process does not work well on linux
                // in current setup.
                // It cannot detect empty pipes.
                , bp::on_exit(on_exit)
            );
            d.perr.close();
        }
    }
    else
    {
        c = bp::child(
            program.wstring()
            , bp::args = wargs
            , bp::start_dir = working_directory.wstring()

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
    }

    // some critical error during process creation
    if (/*!c || */ec)
    {
        while (c.running())
            d.ios.poll_one();
        while (d.pout.is_open() || d.perr.is_open())
            d.ios.poll_one();

        error();
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
        delay = delay > max ? max : delay;
        c.wait_for(delay);
        delay += 100ms;
    }

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
        delay = delay > max ? max : delay;
        d.cv.wait_for(lk, delay, [&d] {return !d.pout.is_open() && !d.perr.is_open(); });
        delay += 100ms;
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