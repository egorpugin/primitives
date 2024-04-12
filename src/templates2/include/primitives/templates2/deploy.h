#pragma once

#include <primitives/templates.h>
#include <primitives/command.h>
#include <primitives/sw/main.h>
#include <primitives/templates2/win32_2.h>

#include <fstream>

#ifndef _WIN32
#include <arpa/inet.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
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
auto command(auto &&...args) {
    primitives::Command c;
    (c.arguments.push_back(args), ...);
    return run_command(c);
}

inline path cygwin_path(const path &p) {
    if (p.empty())
        return {};
    auto s = p.string();
    s[0] = tolower(s[0]);
    if (s.size() > 2) {
        s = s.substr(0, 1) + s.substr(2);
    } else if (s.size() > 1) {
        s = s.substr(0, 1);
    }
    s = "/cygdrive/"s + s;
    return s;
}

auto escape(std::string s) {
    boost::replace_all(s, "\"", "\\\"");
    if (s.contains(" ")) {
        s = std::format("\"{}\"", s);
    }
    return s;
}
auto escape(const char *p) {
    return escape(std::string{p});
}
template <auto N>
auto escape(const char (&s)[N]) {
    return escape(std::string{s});
}
auto escape(const path &p) {
    return escape(p.string());
}

auto cygwin(auto &&...args) {
#ifdef _WIN32
    primitives::Command c2;
    c2.working_directory = fs::current_path();
    c2.push_back("bashc");
    c2.push_back("-l");
    c2.push_back("-c");
    std::string s;
    s += std::format("cd \"{}\"; ", normalize_path(fs::current_path()).string());
    auto f = [&](auto &&arg) {
        s += escape(arg) + " ";
    };
    (f(args), ...);
    s.resize(s.size() - 1);
    c2.push_back(s);
    return run_command(c2);
#else
    return command(args...);
#endif
}
auto rsync(auto &&...args) {
    return cygwin("rsync", args...);
}

template <typename F, typename T>
struct lambda_wrapper {
    T &serv;

    void operator()(std::source_location loc = std::source_location::current()) {
        auto &be = lambda_registrar<F>::end(loc);

        auto fn = loc.file_name();
        auto f = read_file(fn);
        std::string lambda;
        int main_start{};
        for (int i = 1; auto &&v : std::views::split(f, "\n"sv)) {
            std::string_view line{v.begin(), v.end()};
            if (line == "#define MAIN_LABEL"sv && lambda.empty()) {
                main_start = i + 1;
            }
            if (i == be.begin.line()) {
                lambda = line.substr(be.begin.column() - 1);
                lambda = lambda.substr(lambda.find('['));
                lambda += "\n";
            } else if (i > be.begin.line() && i < be.end.line() - 1) {
                lambda += line;
                lambda += "\n";
            }
            ++i;
        }
        lambda.resize(lambda.rfind("});"));
        lambda += "};\n";
        std::string pre;
        for (int i = 1; auto &&v : std::views::split(f, "\n"sv)) {
            if (i > main_start) {
                break;
            }
            std::string_view line{v.begin(), v.end()};
            pre += line;
            pre += "\n";
            ++i;
        }
        pre += "auto f = ";
        pre += lambda;
        pre += R"(
// make template (instantiation) context
auto f2 = [&](auto) {
    if constexpr (std::same_as<decltype(f()), int>) {
        return f();
    }
    else {
        f();
        return 0;
    }
};
return f2(0);
)";
        pre += "}\n";

        auto p = path{".sw/lambda"};
        auto outfn = std::format("{}_{}.cpp", path{be.begin.file_name()}.stem().string(), be.begin.line());
        write_file_if_different(p / outfn, pre);
        auto conf = "static_r";
        auto c = create_command("sw", "build_gcc", outfn, "-static", "-config-name", conf);
        c.working_directory = p;
        run_command(c);

        auto outbinfn = (p / ".sw" / "out" / conf / path{outfn}.stem()) += "-0.0.1";
        auto outservfn = path{"lambda"} / outbinfn.filename();
        {
            auto scope = serv.sudo(false);
            serv.create_directories("lambda");
            serv.rsync("-cavP", normalize_path(outbinfn), serv.server_string() + ":" + normalize_path(outservfn).string());
            serv.command("chmod", "755", normalize_path(outservfn));
        }
        serv.command(normalize_path(outservfn));
    }
};

struct ssh_base {
    bool sudo_mode{};
    path cwd;
    std::string ip_;

    std::string connection_string(this auto &&obj, bool with_server = false, bool with_t = true) {
        using T = std::decay_t<decltype(obj)>;

        std::string s;
        for (auto &&a : obj.connection_args(with_server)) {
            if (!with_t && a == "-t") {
                continue;
            }
            s += a + " ";
        }
        s.resize(s.size() - 1);
        return s;
    }
    auto connection_args(this auto &&obj, bool with_server = false) {
        using T = std::decay_t<decltype(obj)>;

        std::vector<std::string> args;
        args.push_back("ssh");
        args.push_back("-t"); // allocate pseudo tty
        args.push_back("-t"); // 2nd tty?
        args.push_back("-o");
        args.push_back("StrictHostKeyChecking=accept-new");
        args.push_back("-o");
        args.push_back("ConnectionAttempts=10");
        args.push_back("-o");
        args.push_back("ServerAliveInterval=5");
        args.push_back("-o");
        args.push_back("ServerAliveCountMax=1");
        if (with_server) {
            args.push_back(obj.server_string());
        }
        if constexpr (requires { T::port; }) {
            args.push_back("-p");
            args.push_back(std::format("{}", T::port));
        }
        if constexpr (requires { T::key; }) {
            args.push_back("-i");
            args.push_back(std::format("{}", normalize_path(find_key(T::key)).string()));
        }
        return args;
    }
    std::string ip(this auto &&obj, int retries = 0) {
        using T = std::decay_t<decltype(obj)>;

        if constexpr (!requires {T::resolve_host;}) {
            obj.ip_ = T::host;
        } else {
            if (!T::resolve_host) {
                obj.ip_ = T::host;
            }
        }
        if (obj.ip_.empty()) {
            struct addrinfo *result = NULL;
            auto r = getaddrinfo(T::host, 0, 0, &result);
            if (r) {
                threaded_logger_.log("cannot resolve: "s + T::host);
                if (retries > 10) {
                    throw std::runtime_error{"cannot resolve: "s + T::host};
                } else {
                    return obj.ip(retries + 1);
                }
            }
            for (auto ptr = result; ptr != NULL; ptr = ptr->ai_next) {
                switch (ptr->ai_family) {
                case AF_INET:
                    obj.ip_ = inet_ntoa(((struct sockaddr_in *)ptr->ai_addr)->sin_addr);
                    break;
                default:
                    break;
                }
            }
            freeaddrinfo(result);
            if (obj.ip_.empty()) {
                threaded_logger_.log("cannot resolve ipv4: "s + T::host);
                if (retries > 10) {
                    throw std::runtime_error{"cannot resolve ipv4: "s + T::host};
                } else {
                    return obj.ip(retries + 1);
                }
            }
        }
        return obj.ip_;
    }
    std::string server_string(this auto &&obj) {
        using T = std::decay_t<decltype(obj)>;

        std::string s;
        s += std::format("{}@{}", T::user, obj.ip());
        return s;
    }
    path home(this auto &&obj) {
        using T = std::decay_t<decltype(obj)>;
        return path{} / "home" / T::user;
    }
    auto rsync(this auto &&obj, auto &&...args) {
        return ::rsync("-e", obj.connection_string(false, false), args...);
    }
    auto rsync_from(this auto &&obj, const std::string &from, auto &&to, const std::string &flags = "-cavP"s) {
        auto to2 = normalize_path(to);
        to2 = cygwin_path(to2);
        return obj.rsync(flags, obj.server_string() + ":" + (obj.cwd.empty() ? "" : (obj.cwd.string() + "/")) + from, to2);
    }
    /*auto rsync_to(this auto &&obj, const std::string &from, auto &&to, const std::string &flags = "-cavP"s) {
        return obj.rsync(flags, obj.server_string() + ":" + (obj.cwd.empty() ? "" : (obj.cwd.string() + "/")) + from, to);
    }*/
    auto create_command(this auto &&obj, auto &&...args) {
        primitives::Command c;
        c.push_back(obj.connection_args(true));
        if (!obj.cwd.empty()) {
            c.push_back("cd");
            c.push_back(obj.cwd);
            c.push_back("&&");
        }
        if (obj.sudo_mode) {
            c.push_back("sudo");
        }
        (c.push_back(args), ...);
        return c;
    }
    auto command(this auto &&obj, auto &&...args) {
        auto c = obj.create_command(args...);
        return run_command(c);
    }
    auto sudo(this auto &&obj, bool sudo_mode = true) {
        return SwapAndRestore(obj.sudo_mode, sudo_mode);
    }
    auto sudo(this auto &&obj, auto &&prog, auto &&...args) {
        SwapAndRestore sr(obj.sudo_mode, true);
        return obj.command(prog, args...);
    }
    auto create_directories(this auto &&obj, const path &p) {
        return obj.command("mkdir", "-p", normalize_path(p));
    }
    bool is_online(this auto &&obj) {
        try {
            auto c = obj.create_command("exit");
            std::error_code ec;
            c.execute(ec);
            return !ec;
        } catch (std::exception &e) {
            return false;
        }
    }
    void wait_until_online(this auto &&obj) {
        while (!obj.is_online()) {
            threaded_logger_.log(std::format("host {} is not online yet, waiting...", obj.host));
            std::this_thread::sleep_for(5s);
        }
    }
    void send_wake_on_lan_magic(this auto &&obj) {
        // https://wiki.archlinux.org/title/Wake-on-LAN#NetworkManager
        // see https://github.com/gumb0/wol.cpp
        using T = std::decay_t<decltype(obj)>;

        auto get_hex_from_string = [](std::string s) {
            boost::to_lower(s);
            unsigned hex{0};
            for (size_t i = 0; i < s.length(); ++i) {
                hex <<= 4;
                if (isdigit(s[i])) {
                    hex |= s[i] - '0';
                } else if (s[i] >= 'a' && s[i] <= 'f') {
                    hex |= s[i] - 'a' + 10;
                } else {
                    throw std::runtime_error("Failed to parse hexadecimal " + s);
                }
            }
            return hex;
        };
        auto get_ether = [&](const std::string &hardware_addr) {
            std::string ether_addr;
            for (size_t i = 0; i < hardware_addr.length();) {
                // Parse two characters at a time.
                unsigned hex = get_hex_from_string(hardware_addr.substr(i, 2));
                i += 2;
                ether_addr += static_cast<char>(hex & 0xFF);
                // We might get a colon here, but it is not required.
                if (hardware_addr[i] == ':')
                    ++i;
            }
            if (ether_addr.length() != 6)
                throw std::runtime_error(hardware_addr + " not a valid ether address");
            return ether_addr;
        };

        unsigned short port = 60000;
        unsigned long bcast{0xFFFFFFFF};
        const std::string ether_addr{get_ether(T::mac)};
        auto fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (fd <= 0) {
            // wsa startup on win?
            throw std::runtime_error{"can't create a socket"};
        }
        SCOPE_EXIT {
#ifdef _WIN32
            closesocket(fd);
#else
            close(fd);
#endif
        };
        // Build the message to send.
        //   (6 * 0XFF followed by 16 * destination address.)
        std::string message(6, 0xFF);
        for (size_t i = 0; i < 16; ++i) {
            message += ether_addr;
        }
        // Set socket options.
        const int optval{1};
        if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char *)&optval, sizeof(optval)) < 0) {
            throw std::runtime_error("Failed to set socket options");
        }
        // Set up address
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = bcast;
        addr.sin_port = htons(port);
        // Send the packet out.
        if (sendto(fd, message.c_str(), message.length(), 0, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
            throw std::runtime_error("Failed to send packet");
        }
    }
    void wake_on_lan(this auto &&obj) {
        obj.send_wake_on_lan_magic();
        while (!obj.is_online()) {
            threaded_logger_.log(std::format("host {} is not online yet, waiting...", obj.host));
            std::this_thread::sleep_for(5s);
            obj.send_wake_on_lan_magic();
        }
    }
    auto create_lambda(this auto &&obj, auto f, std::source_location loc = std::source_location::current()) {
        using T = std::decay_t<decltype(obj)>;
        lambda_registrar<decltype(f)>::begin(loc);
        return lambda_wrapper<decltype(f), T>(obj);
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
    auto directory(this auto &&obj, const path &dir) {
        return SwapAndRestore(obj.cwd, dir);
    }
    path home_dir(this auto &&obj) {
        if constexpr (requires {obj.os;}) {
            if (obj.os == "macos"s) {
                return "/Users";
            }
        }
        return "/home";
    }
    path user_dir(this auto &&obj) {
        return obj.home_dir() / obj.user;
    }
    void remove_known_host(this auto &&obj) {
        using T = std::decay_t<decltype(obj)>;
        std::string s = T::host;
        if constexpr (requires { T::port; }) {
            s = std::format("[{}]:{}", T::host, T::port);
        }
        ::command("ssh-keygen", "-R", s);
    }
    void remove_all(this auto &&obj, const path &dir) {
        obj.command("rm", "-rf", normalize_path(dir));
    }
};

struct sw_system {
    path storage_dir(this auto &&obj) {
        return boost::trim_copy(obj.command("sw", "get", "storage-dir"));
    }
    void remove_remote_storage(this auto &&obj) {
        obj.remove_all(obj.storage_dir() / "etc" / "sw" / "database" / "1" / "remote");
    }
};

} // namespace primitives::deploy