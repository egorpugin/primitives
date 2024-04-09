#pragma once

#include <primitives/command.h>
#include <primitives/sw/main.h>
#include <primitives/templates2/win32_2.h>

#ifndef _WIN32
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

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
        std::cout << s;
    };
    c.err.inherit = true;
    std::cout << c.print() << "\n";
    c.execute();
    return out;
}

struct ssh_command {
    auto send_command(auto &&server, auto &&remote_args) {
        using T = std::decay_t<decltype(server)>;

        primitives::Command c;
        c.push_back("ssh");
        c.push_back("-i");
        c.push_back(find_key(T::key));
        c.push_back("-o");
        c.push_back("StrictHostKeyChecking=no");
        c.push_back(std::format("{}@{}", T::user, T::host));
        if constexpr (requires { T::port; }) {
            c.push_back("-p");
            c.push_back(std::format("{}", T::port));
        }
        std::string s;
        for (auto &c : remote_args) {
            s += c + " ";
        }
        if (!s.empty()) {
            s.resize(s.size() - 1);
        }
        c.push_back(s);
        return run_command(c);
    }
    auto send_command(auto &&server, auto &&...args) {
        std::vector<std::string> c;
        ((c.push_back(args)), ...);
        return send_command(server, c);
    }
    static path find_key(auto &&name) {
        path p;
        if (auto u = getenv("USERPROFILE")) {
            p = u;
            p /= ".ssh";
            p /= name;
            if (fs::exists(p)) {
                return p;
            }
        }
        if (auto u = getenv("USER")) {
            p = "/mnt/c/Users";
            p /= u;
            p /= ".ssh";
            if (fs::exists(p / name)) {
                return p / name;
            }
            p = "/cygdrive/c/Users";
            p /= u;
            p /= ".ssh";
            if (fs::exists(p / name)) {
                return p / name;
            }
        }
        throw SW_RUNTIME_ERROR("cannot find ssh key: "s + name);
    }
};

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

} // namespace primitives::deploy
