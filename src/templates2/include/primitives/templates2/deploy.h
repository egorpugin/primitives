#pragma once

#include <primitives/command.h>
//#include <primitives/executor.h>
#include <primitives/sw/main.h>
#include <primitives/templates2/win32_2.h>

#include <fstream>

#ifndef _WIN32
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#include <primitives/log.h>
DECLARE_STATIC_LOGGER(logger, "deploy");

namespace primitives::deploy {

#ifdef _WIN32
struct job_object_initializer {
    job_object_initializer() {
        win32::default_job_object();
    }
} job_object_initializer_;
#endif

template <typename F>
struct appender {
    F f;
    auto operator,(auto &&v) {
        f(v);
        return std::move(*this);
    }
};

template <typename T>
struct lambda_registrar {
    struct wrapper {
        std::source_location begin, end;
    };
    static inline wrapper value;
    static void begin(std::source_location loc) { value.begin = loc; }
    static wrapper &end(std::source_location loc) { value.end = loc; return value; }
};

struct thread_manager {
    std::thread::id main_tid{std::this_thread::get_id()};

    bool is_main_thread() const {
        return main_tid == std::this_thread::get_id();
    }
} thread_manager_;

thread_local std::string thread_name;
thread_local std::string log_name;
path log_dir;

struct threaded_logger {
    void log(std::string s) {
        boost::trim(s);
        boost::replace_all(s, "\r\n", "\n");
        if (thread_manager_.is_main_thread()) {
            LOG_INFO(logger, s);
            //std::cout << s;
        } else {
            LOG_INFO(logger, std::format("[{}] {}", thread_name, s));
            thread_local std::ofstream ofile{log_dir / log_name};
            ofile << s << "\n";
            ofile.flush(); // ?
        }
    }
} threaded_logger_;

struct scoped_task {
    std::jthread t;
    bool err{};
    std::string name;
    scoped_task(const std::string &n, auto &&f) {
        name = n;
        threaded_logger_.log(std::format("starting job: {}", name));
        t = std::jthread{[&] {
            try {
                thread_name = name;
                log_name = name + ".log";
                f();
            } catch (std::exception &e) {
                err = true;
                threaded_logger_.log(e.what());
            } catch (...) {
                err = true;
                threaded_logger_.log("unknown exception");
            }
        }};
    }
    ~scoped_task() noexcept(false) {
        t.join();
        if (err) {
            auto s = "error in child job: "s + name;
            threaded_logger_.log(s);
            if (std::uncaught_exceptions() == 0) {
                throw std::runtime_error{s};
            }
        }
    }
};
auto scoped_runner(auto && ... args) {
    return scoped_task{args...};
}

/*struct command {
    primitives::Command c;

    void add(const char *p) {
        c.push_back(p);
    }
    auto operator+=(auto &&arg) {
        add(arg);
        return appender{[&](auto &&v) {
            add(v);
        }};
    }
};*/

auto create_command(auto &&...args) {
    primitives::Command c;
    (c.arguments.push_back(args), ...);
    return c;
}
auto run_command(primitives::Command &c) {
    std::string out;
    c.out.action = [&](const std::string &s, bool eof) {
        out += s;
        threaded_logger_.log(s);
    };
    if (thread_manager_.is_main_thread()) {
        c.err.inherit = true;
    } else {
        c.err.action = [](const std::string &s, bool eof) {
            threaded_logger_.log(s);
        };
    }
    threaded_logger_.log(c.print());
    c.execute();
    return out;
}

/*auto run_command(auto &&...args) {
    primitives::Command c;
    (c.push_back(args), ...);
    return run_command(c);
}*/
auto run_command(std::vector<std::string> args) {
    primitives::Command c;
    for (auto &&a : args) {
        c.push_back(a);
    }
    return run_command(c);
}
auto run_command_string(const std::string &s) {
    auto args = split_string(s, " ");
    return run_command(args);
}

auto command(const std::string &cmd, auto &&...args) {
    return run_command(cmd, args...);
}
/*auto command(auto &&ssh_, const std::string &cmd, auto &&...args) {
    ssh_command s;
    return s.send_command(ssh_, cmd, args...);
}*/

inline path cygwin_path(const path &p) {
    if (p.empty())
        return {};
    auto s = p.string();
    s[0] = tolower(s[0]);
    if (s.size() > 2) {
        s = s.substr(0,1) + s.substr(2);
    } else if (s.size() > 1) {
        s = s.substr(0, 1);
    }
    s = "/cygdrive/"s + s;
    return s;
}

} // namespace primitives::deploy
