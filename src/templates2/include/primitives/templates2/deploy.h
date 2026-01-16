#pragma once

#include <primitives/templates.h>
#include <primitives/http.h>
#include <primitives/command.h>
#include <primitives/version.h>
#include <primitives/sw/main.h>
#include <primitives/templates2/win32_2.h>
#include <primitives/templates2/overload.h>
#include <primitives/sw/settings_program_name.h>

#include <fstream>
#include <source_location>
#include <variant>

#ifndef _WIN32
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <primitives/log.h>
DECLARE_STATIC_LOGGER(logger, "deploy");

// TODO:
// next item is proper paths, normalized for linuxes, for cygwin etc.
// use env from commands
// rsync args

unsigned int edit_distance(const std::string &s1, const std::string &s2) {
    const std::size_t len1 = s1.size(), len2 = s2.size();
    std::vector<std::vector<unsigned int>> d(len1 + 1, std::vector<unsigned int>(len2 + 1));

    d[0][0] = 0;
    for (unsigned int i = 1; i <= len1; ++i)
        d[i][0] = i;
    for (unsigned int i = 1; i <= len2; ++i)
        d[0][i] = i;

    for (unsigned int i = 1; i <= len1; ++i)
        for (unsigned int j = 1; j <= len2; ++j)
            // note that std::min({arg1, arg2, arg3}) works only in C++11,
            // for C++98 use std::min(std::min(arg1, arg2), arg3)
            d[i][j] = std::min({d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1)});
    return d[len1][len2];
}

namespace primitives::deploy {

using version_type = primitives::version::Version;

inline const version_type max_fedora_version = "43"; // get from internet?
inline const version_type max_ubuntu_version = "24.4";
inline const auto target_gcc_config = "build_gcc"sv;

inline std::mutex m_ssh_keygen;

#ifdef _WIN32
struct job_object_initializer {
    job_object_initializer() {
        win32::default_job_object();
    }
} job_object_initializer_;
#endif

decltype(auto) visit(auto &var, auto &&...f) {
    return ::std::visit(overload{FWD(f)...}, var);
}

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

auto current_time() {
    return std::chrono::zoned_time{std::chrono::current_zone(), std::chrono::system_clock::now()};
}
auto current_time_string() {
    return std::format("{:%F %H-%M-%S}", current_time());
}

struct thread_manager {
    std::thread::id main_tid{std::this_thread::get_id()};

    bool is_main_thread() const {
        return main_tid == std::this_thread::get_id();
    }
} thread_manager_;

thread_local std::string thread_name;
thread_local std::string log_name;
const path &log_dir1(const path &in = {}) {
    static auto dir = [] {
        path dir = ".sw/log";

        /*std::string time = __DATE__;
        time += " " __TIME__;

        std::istringstream ss{time};
        std::chrono::system_clock::time_point tp;
        ss >> std::chrono::parse("%b %d %Y %T", tp);*/

        std::string time;
        time = std::format("{:%F %H-%M-%OS}", current_time());
        time += " " + sw::getProgramName();

        path p = time;
        //path p = __DATE__ __TIME__; // PACKAGE_PATH;

        dir /= p.stem();
        fs::create_directories(normalize_path(dir));
        return fs::absolute(dir);
    }();
    if (!in.empty()) {
        dir = in;
    }
    return dir;
}
const path &log_dir(const path &in = {}) {
    const auto &d = log_dir1(fs::absolute(in));
    LOG_INFO(logger, "changing log dir to " << d);
    return d;
}

struct threaded_logger {
    void log(std::string s, std::source_location loc = std::source_location::current()) {
        boost::trim(s);
        boost::replace_all(s, "\r\n", "\n");
        auto make_string = [&]() {
            return std::format(
                //"[{}][{}:{}:{}] {}"
                "[{}] [{}:{}] {}"
                , current_time_string(), path{loc.file_name()}.filename().string(), loc.line()
                //, loc.function_name() // too long = full unmangled
                , s);
        };
        if (thread_manager_.is_main_thread()) {
            LOG_INFO(logger, s);
            static std::ofstream ofile{log_dir1() / "main.log"};
            ofile << make_string() << "\n";
            ofile.flush(); // ?
        } else {
            LOG_INFO(logger, std::format("[{}] {}", thread_name, s));
            thread_local std::ofstream ofile{log_dir1() / log_name};
            ofile << make_string() << "\n";
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
auto run_command_raw(primitives::Command &c) {
    std::string out,err;
    c.out.action = [&](const std::string &s, bool eof) {
        out += s;
        threaded_logger_.log(s);
    };
    c.err.action = [&](const std::string &s, bool eof) {
        err += s;
        threaded_logger_.log(s);
        };
    threaded_logger_.log(c.print());
    //
    std::string cmd;
    for (auto &&a : c.getArguments()) {
        auto as = a->toString();
        boost::replace_all(as, "\"", "\\\"");
        cmd += std::format("\"{}\" ", as);
    }
    threaded_logger_.log(std::format("quoted command:\n{}", cmd));
    //
    std::error_code ec;
    c.execute(ec);
    if (!c.exit_code) {
        throw std::runtime_error{c.err.text + ": " + ec.message()};
    }
    struct result {
        decltype(c.exit_code)::value_type exit_code;
        std::string out;
        std::string err;
        explicit operator bool() const {return exit_code == 0;}
    };
    return result{*c.exit_code, std::move(out), std::move(err)};
}
auto run_command(primitives::Command &c) {
    auto res = run_command_raw(c);
    if (res.exit_code) {
        throw std::runtime_error{"command failed: " + c.print()};
    }
    return res.out;
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

inline path cygwin_path(path p) {
    p = normalize_path(p);
    if (p.empty())
        return {};
    if (!p.is_absolute()) {
        return p;
    }
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

auto cygwin_raw(const std::vector<std::string> &args) {
#ifdef _WIN32
    primitives::Command c2;
    c2.working_directory = fs::current_path();
    c2.push_back("bashc");
    c2.push_back("-l");
    c2.push_back("-c");
    std::string s;
    s += std::format("cd \"{}\"; ", normalize_path(fs::current_path()).string());
    for (auto &&arg : args) {
        s += escape(arg) + " ";
    }
    s.resize(s.size() - 1);
    c2.push_back(s);
    return c2;
#else
    primitives::Command c;
    c.push_back(args);
    return c;
#endif
}
auto cygwin(auto &&...args) {
    std::vector<std::string> vargs;
    (vargs.push_back(args), ...);

    auto c = cygwin_raw(vargs);
    auto res = run_command_raw(c);
    if (res.exit_code) {
        throw std::runtime_error{"command failed: " + c.print()};
    }
    return res.out;
}

struct rsync_options {
    std::vector<std::string> arguments;
    std::vector<std::string> exclude;
    //fs::path from;
    //fs::path to;
};
auto default_rsync_options() {
    rsync_options opts{{
        "-czavP"s,
        //"--stats",
        "--no-owner"s,
        "--no-group"s,
    }};
    return opts;
}

auto rsync_raw(const rsync_options &opts) {
    std::vector<std::string> vargs;
    vargs.push_back("rsync");
    for (auto &&a : opts.arguments) {
        vargs.push_back(a);
    }
    for (auto &&a : opts.exclude) {
        vargs.push_back(std::format("--exclude={}", a));
    }
    return cygwin_raw(vargs);
}
auto rsync(const rsync_options &opts) {
    auto c = rsync_raw(opts);
    auto res = run_command_raw(c);
    if (res.exit_code) {
        if (res.err.contains("Sorry, try again.")) {
            if (res.err.contains("incorrect password")) {
            }
            throw std::runtime_error{ "command failed, probably ssh environment is not enabled: " + c.print() };
        }
        throw std::runtime_error{"command failed: " + c.print()};
    }
    return res.out;
}
auto make_rsync_local(const path &in) {
    return cygwin_path(in).string();
}

auto system(const std::string &s) {
    return ::system(s.c_str());
}

struct this_system {
    auto command(auto &&...args) {
        return primitives::deploy::command(args...);
    }
};

template <typename Serv>
struct git_command {
    Serv &serv;
    path dir{"."};

    auto operator()(auto &&...args) {
        return serv.command("git", "-C", dir, args...);
    }
    auto pull(auto &&...args) {
        return operator()("pull", args...);
    }
private:
    auto create_command() {
        //primitives::deploy::create_command()
    }
};

struct sw_command {
    auto operator()(auto &&...args) {
        return command(args...);
    }
    auto build(auto &&...args) {
        return command("build", args...);
    }
    auto run(auto &&...args) {
        return command("run", args...);
    }
    auto install(auto &&...args) {
        return command("install", args...);
    }
    auto build_raw(auto &&...args) {
        return command(args...);
    }
    auto build_for(auto serv, auto &&...args) {
        if constexpr (requires { serv.os; }) {
            SW_UNIMPLEMENTED;
            /*if (serv.os == "macos"s) {
                return build_for_macos(args...);
            }*/
        }
        return build_for_linux(args...);
    }
    auto build_for_linux(auto &&...args) {
        return build_raw(target_gcc_config, args...);
    }
    auto build_for_macos(auto &&...args) {
        return build_raw("build_macos", args...);
    }

    auto list_targets(auto &&...args) {
        auto c = create_command("build", "--list-targets", args...);
        auto res = run_command_raw(c);
        if (res.exit_code) {
            throw std::runtime_error{"command failed: " + c.print()};
        }
        return boost::trim_copy(res.out);
    }

private:
    primitives::Command create_command(auto &&...args) {
        auto c = primitives::deploy::create_command("sw");
        (c.arguments.push_back(args), ...);
        return c;
    }
    std::string command(auto &&...args) {
        auto c = create_command(args...);
        auto res = run_command_raw(c);
        if (res.exit_code) {
            throw std::runtime_error{"command failed: " + c.print()};
        }
        return res.err;
    }
};

// can be remote
struct sw_system {
    path storage_dir(this auto &&obj) {
        while (1) {
            auto s = boost::trim_copy(obj.command("sw", "get", "storage-dir"));
            if (s.starts_with("Downloading")) {
                continue;
            }
            return s;
        }
    }
    void remove_remote_storage(this auto &&obj) {
        obj.remove_all(obj.storage_dir() / "etc" / "sw" / "database" / "1" / "remote");
    }
};

struct raw_command_tag {};

template <typename Serv>
struct systemctl {
    struct simple_service {
        systemctl &ctl;
        std::string name;
        std::string progname;
        std::vector<std::string> args;
        std::string service_pre_script;
        std::string desc;
        std::string user;
        std::string wdir;
        std::map<std::string, std::string> environment;

        auto start() {
            return ctl("start", name);
        }
        auto restart() {
            return ctl("restart", name);
        }
        auto stop() {
            return ctl("stop", name);
        }
        auto enable() {
            return ctl("enable", name);
        }
        auto disable() {
            return ctl("disable", name);
        }
        auto status() {
            // https://refspecs.linuxbase.org/LSB_3.0.0/LSB-PDA/LSB-PDA/iniscrptact.html
            // 3 - program is not running
            auto r = ctl(raw_command_tag{}, "status", name, "--no-pager", "-l");
            return r;
        }
        auto service_name() const {
            auto n = name + ".service";
            return "/etc/systemd/system/" + n;
        }
        void remove_from_system() {
            ctl.serv.remove(service_name());
        }
        void add_to_system() {
            if (desc.empty()) {
                desc = name;
            }
            if (user.empty()) {
                user = name;
            }
            if (wdir.empty()) {
                wdir = "/home/" + user;
            }

            constexpr auto service_template =
                R"(
[Unit]
Description={} service
After=network-online.target

[Service]
Type=simple
User={}
{}
ExecStart={}
WorkingDirectory={}
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
)";
            std::string args_s;
            decltype(args) args2;
            args2.push_back(wdir + "/" + progname);
            args2.append_range(args);
            std::string s;
            for (auto &&a : args2) {
                s += escape(a) + " ";
            }
            s.resize(s.size() - 1);

            std::string pre;
            for (auto &&[k,v] : environment) {
                pre += std::format("Environment={}={}\n", k, v);
            }
            if (!service_pre_script.empty()) {
                pre = "ExecStartPre=/usr/bin/bash -c " + wdir + "/" + "pre.sh";
            }
            auto service = std::format(service_template, desc, user, pre, s, wdir);
            ctl.serv.sudo().write_file(service_name(), service);
        }
    };

    Serv &serv;

    auto operator()(auto &&...args) {
        auto c = serv.sudo().create_command("systemctl", args...);
        return run_command(c);
    }
    auto operator()(raw_command_tag, auto &&...args) {
        auto c = serv.create_command("systemctl", args...);
        return run_command_raw(c);
    }
    auto daemon_reload() {
        return operator()("daemon-reload");
    }
    auto service(const std::string &name) {
        return simple_service{*this,name};
    }
    auto new_simple_service() {
        return simple_service{*this};
    }
    void new_simple_service_auto(const std::string &name, const std::string &progname) {
        auto svc = new_simple_service();
        svc.name = name;
        svc.progname = progname;
        new_simple_service_auto(svc);
    }
    void new_simple_service_auto(simple_service &svc) {
        svc.add_to_system();
        daemon_reload();
        auto r = svc.status().exit_code;
        svc.enable();
        if (r == 0) {
            svc.restart();
        } else {
            svc.start();
        }
        r = svc.status().exit_code;
        if (r != 0) {
            svc.stop();
            throw std::runtime_error{"cannot start the server"};
        }
    }
    void delete_simple_service(const std::string &name) {
        auto svc = service(name);
        auto r = svc.status().exit_code;
        // 4 = not found
        if (r != 4) {
            svc.stop();
            svc.disable();
        }
        svc.remove_from_system();
        daemon_reload();
    }
};

template <typename Serv>
struct dnf {
    Serv &serv;
    auto operator()(auto &&...args) {
        auto s = serv.sudo();
        return s.command("dnf", args...);
    }
    auto upgrade(auto &&...args) {
        return operator()("upgrade", "-y", args...);
    }
    auto install(auto &&...args) {
        return operator()("install", "-y", args...);
    }
    auto system_upgrade(auto &&...args) {
        return operator()("system-upgrade", args...);
    }
};

template <typename Serv>
struct apt {
    Serv &serv;

    auto operator()(auto &&...args) {
        auto s = serv.sudo();
        //auto env = serv.scoped_environment("DEBIAN_FRONTEND", "noninteractive");
        auto c = s.create_command("apt", args...);
        c.environment["DEBIAN_FRONTEND"] = "noninteractive";
        s.run_command(c);
    }
    auto update(auto &&...args) {
        return operator()("update", args...);
    }
    auto upgrade(auto &&...args) {
        return operator()("upgrade", "-y", "-q", args...);
    }
    auto dist_upgrade(auto &&...args) {
        return operator()("dist-upgrade", "-y", "-q", args...);
    }
    auto install(auto &&...args) {
        return operator()("install", "-y", "-q", args...);
    }
};

template <typename Serv>
struct pacman {
    Serv &serv;

    auto operator()(auto &&...args) {
        auto s = serv.sudo();
        return s.command("pacman", args...);
    }
    auto update(auto &&...args) {
        return operator()("-y", args...);
    }
    auto upgrade(auto &&...args) {
        return operator()("-u", args...);
    }
    auto install(auto &&...args) {
        return operator()("-S", args...);
    }
};

struct system_base {
    void initial_setup(this auto &&obj) {
        auto &serv = obj.serv;
        auto su = serv.sudo();
        path ctor_file = "/root/.deploy/ctor";

        if (su.exists(ctor_file)) {
            return;
        }

        if (!obj.serv.current_user().is_root()) {
            su.setup_ssh();
        }
        obj.update_packages();
        // reboot?
        obj.package_manager().install("htop");
        obj.package_manager().install("rsync");
        obj.package_manager().install("certbot");
        //obj.package_manager().install("nginx"); // by default?
        //obj.package_manager().install("dnsmasq"); // by default?
        // dnsmasq requires the following setup for openvpn
        // listen-address=127.0.0.1
        // listen-address=10.8.0.1
        // bind-interfaces
        // gdb?
        if constexpr (requires {obj.initial_setup_packages();}) {
            obj.initial_setup_packages();
        }

        // for our servers Europe/Moscow
        su.command("timedatectl", "set-timezone", "UTC");

        // or ubuntu?
        // serv.create_user("egor");

        su.create_directories(ctor_file.parent_path());
        su.command("touch", ctor_file);
    }
};

template <typename Serv>
struct ubuntu : system_base {
    Serv &serv;
    version_type version;
    std::string name{"ubuntu"};

    ubuntu(Serv &serv, version_type version) : serv{serv}, version{version} {
        initial_setup();
    }
    void update_packages() {
        auto pm = package_manager();
        pm.update();
        pm.upgrade();
    }
    auto package_manager() {
        return apt{serv};
    }
    void upgrade_system() {
        if (version >= max_ubuntu_version) {
            threaded_logger_.log("system is already up to date"s);
            return;
        }
        auto su = serv.sudo();
        update_packages();
        //serv.reboot();

        apt a{su};
        a.install("ubuntu-release-upgrader-core");
        a.dist_upgrade();
        su.command("iptables", "-I", "INPUT", "-p", "tcp", "--dport", "1022", "-j", "ACCEPT");
        // ubuntu won't give us LTS option until xx.04.1 in August or so, so we use '-d'
        su.command("do-release-upgrade", "-d", "--frontend=DistUpgradeViewNonInteractive");
        su.reboot();
        threaded_logger_.log("system upgraded"s);
    }
    void install_postgres() {
        SW_UNIMPLEMENTED;
    }
};

template <typename Serv>
struct fedora : system_base {
    Serv &serv;
    version_type version;
    std::string name{"fedora"};

    fedora(Serv &serv, version_type version) : serv{serv}, version{version} {
        initial_setup();
    }
    void initial_setup_packages() {
        //package_manager().install("setools-console");
        package_manager().install("openssh-askpass");
        package_manager().install("policycoreutils-python-utils");
    }
    void update_packages() {
        auto pm = package_manager();
        pm.upgrade();
    }
    auto package_manager() {
        return dnf{serv};
    }
    void upgrade_system() {
        auto to_version = version_type{version.getMajor() + 1};
        if (to_version > max_fedora_version) {
            threaded_logger_.log("system is already up to date"s);
            return;
        }
        auto sudo = serv.sudo();
        sudo.upgrade_packages("--refresh");
        dnf d{sudo};
        d.install("dnf-plugin-system-upgrade");
        d.system_upgrade("download", "-y", std::format("--releasever={}", to_version.getMajor()));
        sudo.reboot([&] {
            if (to_version.getMajor() >= 42) {
                sudo.command("dnf5", "offline", "reboot", "-y");
            } else {
                d.system_upgrade("reboot");
            }
        });
        threaded_logger_.log("system upgraded"s);
    }
    void install_postgres() {
        package_manager().install("postgresql-server");
        // exts like citext
        package_manager().install("postgresql-contrib");
        auto su = serv.sudo();
        if (!su.exists("/var/lib/pgsql/data/pg_hba.conf")) {
            su.command("/usr/bin/postgresql-setup", "--initdb");
        }
        systemctl ctl{su};
        auto svc = ctl.service("postgresql");
        svc.enable();
        if (svc.status().exit_code != 0) {
            svc.start();
            if (svc.status().exit_code != 0) {
                throw std::runtime_error{"can't start pg service"};
            }
        }
    }
};

template <typename Serv>
struct arch : system_base {
    Serv &serv;
    std::string name{"arch"};

    arch(Serv &serv) : serv{serv} {
        //initial_setup();
    }
    void initial_setup_packages() {
    }
    void update_packages() {
        SW_UNIMPLEMENTED;
        /*auto scoped_sudo = serv.sudo();
        auto pm = package_manager();
        pm.upgrade();*/
    }
    auto package_manager() {
        return pacman{serv};
    }
    void upgrade_system() {
        SW_UNIMPLEMENTED;
    }
    void install_postgres() {
        SW_UNIMPLEMENTED;
    }
};

template <typename F, typename T>
struct lambda_wrapper {
    T &serv;

    void operator()(auto &&as_user, std::source_location loc = std::source_location::current()) {
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
            } else if (i > be.begin.line() && i < be.end.line()) {
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
        //pre += "threaded_logger_.log(\"before fs::current_path(home);\");\n\n";
        pre += "fs::current_path(\"" + normalize_path(as_user.home_dir()).string() + "\");\n\n";
        pre += "threaded_logger_.log(\"after fs::current_path(home);\");\n\n";
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
        auto c = create_command("sw", target_gcc_config, outfn, "-static", "-config-name", conf, "-sfc");
        c.working_directory = p;
        run_command(c);

        auto outbinfn = (p / ".sw" / "out" / conf / path{outfn}.stem()) += "-0.0.1";
        auto outservfn = path{"lambda"} / outbinfn.filename();
        {
            as_user.create_directories("lambda");
            as_user.rsync_to(outbinfn, as_user.absolute_path(outservfn));
            as_user.command("chmod", "755", normalize_path(as_user.absolute_path(outservfn)));
        }
        as_user.command(normalize_path(as_user.absolute_path(outservfn)));
    }
};

struct ssh_key {
};

struct ssh_options {
    bool pseudo_tty{};
    bool force_pseudo_tty{};
    // bool with_server{};
    bool password_auth_no{true};
};

template <typename Serv>
struct user {
    Serv &serv;
    std::string name;
    std::string password;
    bool ssh_access{};
    bool can_sudo{};
    bool sudo_nopasswd{};
    bool force_sudo_with_pass{};

private:
    // scoped settings
    bool sudo_mode{};
    /*const */user *main_user{};
    std::optional<ssh_options> custom_options;
    path cwd;

    bool sudo_mode_with_password() const {
        //auto r = !sudo_nopasswd && (is_root() || main_user && name != main_user->name);
        //return r;

        if (force_sudo_with_pass) {
            return true;
        }

        if (sudo_nopasswd) {
            return false;
        }

        // was
        //auto r = is_root() || !sudo_nopasswd || main_user && name != main_user->name;
        //return r;

        auto r = is_root() || main_user && name != main_user->name;
        return r;
    }

public:
    user(Serv &serv, const std::string &name, const std::string &password = {})
        : serv{serv}, name{name}, password{password} {
        serv.constructing_user = this;
        init();
        //cwd = home_dir(); //? this break things atm, because this home dir is not taken into account in several places and not respected
    }
    user(const user &) = default;
    user &operator=(const user &) = default;
    user(user &&) = default;
    user &operator=(user &&) = default;
    void init() {
        if (ssh_access) {
            return;
        }
        {
            auto ret = command_raw("test", "0");
            ssh_access = !ret.exit_code;
        }
        if (!ssh_access) {
            return;
        }
        {
            // we set up our options properly, so sudo will give an error if it requires a password
            decltype(custom_options) opts = ssh_options{};
            opts->password_auth_no = true;
            SwapAndRestore sr1{custom_options, opts};
            auto ret = command_raw("sudo", "-v");
            can_sudo = !ret.exit_code;
            if (!can_sudo && !password.empty()) {
                {
                SwapAndRestore sr{sudo_mode, true};
                SwapAndRestore sr1{force_sudo_with_pass, true};
                auto ret = command_raw("sudo", "-v");
                can_sudo = !ret.exit_code;
                }
                if (can_sudo) {
                    force_sudo_with_pass = true;
                }
                return;
            }
            if (can_sudo) {
                auto ret = command_raw("sudo", "test", "0");
                sudo_nopasswd = !ret.exit_code;
                if (!sudo_nopasswd && ret.err.contains("sudo: a password is required")) {
                    throw std::runtime_error{std::format("user {} cannot sudo. missing password?", name)};
                }
            }
        }
        /*if (!can_sudo && !password.empty()) {
            SwapAndRestore sr1{sudo_mode, true};
            auto ret = command_raw("test", "0");
            can_sudo = !ret.exit_code;
        }*/
    }
    bool is_root() const { return name == "root"; }
    bool operator==(std::string_view n) const { return name == n; }
    auto sudo() const {
        if (!can_sudo) {
            throw std::runtime_error{std::format("user {} cannot sudo", name)};
        }
        auto u = *this;
        u.sudo_mode = true;
        return u;
    }
    auto login_with_sudo(const std::string &name) {
        if (!can_sudo) {
            throw std::runtime_error{std::format("user {} cannot sudo", this->name)};
        }
        auto u = *this;
        u.name = name;
        u.password.clear();
        u.sudo_mode = true;
        u.main_user = this;
        return u;
    }

    auto create_command(auto &&...args) {
        ssh_options opts{};
        if (custom_options) {
            opts = *custom_options;
        } else {
            opts.password_auth_no = !sudo_mode || !sudo_mode_with_password();
            if (opts.password_auth_no) {
                opts.pseudo_tty = true;
                opts.force_pseudo_tty = true;
            }
        }

        primitives::Command c;
        c.push_back(serv.connection_args(opts));
        c.push_back(serv.server_string(main_user ? main_user->name : name));
        c.push_back("--"); // end of ssh args
        auto tmpdir = "/tmp"s;
        //auto tmpdir = "/tmp/sw"s;
        auto tmpfile = tmpdir + "/run.sh"s;
        if (!cwd.empty()) {
            //c.push_back("cd");
            //c.push_back(cwd);
            //c.push_back("&&");
            //c.environment["PWD"] = normalize_path(cwd).string();
            //
            //serv.create_directories(tmpdir);
            auto t = std::format("#!/bin/bash\n");
            t += std::format("cd \"{}\" && ", normalize_path(cwd).string());
            auto add = overload([&](const std::string &arg) {
                t += std::format("\"{}\" ", arg);
                }, /*[&]<auto N>(const char(&arg)[N]) {
                    t += std::format("\"{}\" ", arg);
                },*/ [&](const char *arg) {
                    t += std::format("\"{}\" ", arg);
                }, [&](const std::vector<std::string> &arg) {
                    for (auto &&e : arg) {
                        t += std::format("\"{}\" ", e);
                    }
                    }, [&](const path &arg) {
                        t += std::format("\"{}\" ", arg.string());
                        });
            (add(args), ...);
            threaded_logger_.log("making run.sh:\n"s + t);
            serv.write_file(tmpfile, t);
        }
        if (sudo_mode && !is_root()) {
            if (main_user) {
                c.push_back("sudo");
                c.push_back("-u");
                c.push_back(name);
            } else {
                c.push_back("sudo");
            }
            if (sudo_mode_with_password()) {
                c.push_back("-S");
            }
        }

        if (cwd.empty()) {
            (c.push_back(args), ...);
        } else {
            c.push_back(tmpfile);
        }

        if (sudo_mode && sudo_mode_with_password()) {
            auto &pass = main_user ? main_user->password : password;
            if (pass.empty()) {
                throw std::runtime_error{"password is required for sudo command"};
            }
            c.in.text = pass;
        }
        return c;
    }
    auto command(auto &&...args) {
        auto c = create_command(args...);
        return run_command(c);
    }
    auto command_raw(auto &&...args) {
        auto c = create_command(args...);
        return run_command_raw(c);
    }
    auto run_command(auto &&c) {
        auto res = run_command_raw(c);
        if (res.exit_code) {
            throw std::runtime_error{"command failed: " + c.print()};
        }
        return res.out;
    }
    auto run_command_raw(auto &&c) {
        auto e = ssh_environment_file();
        auto has_env = !c.environment.empty();
        auto old = serv.environment;
        if (has_env) {
            serv.environment = c.environment;
            serv.set_environment();
        }
        SCOPE_EXIT {
            if (has_env) {
                serv.environment = old;
                serv.set_environment();
            }
        };
        return primitives::deploy::run_command_raw(c);
    }

    void current_path(const path &p) {
        if (cwd.empty()) {
            cwd = home_dir();
        }
        cwd /= p;
        cwd = normalize_path(cwd);
    }
    auto current_path() {
        return cwd;
    }
    path home_dir() const {
        if (is_root()) {
            return "/root";
        }
        return normalize_path(serv.users_directory() / name);
    }
    auto create_directories(const path &p) {
        return command("mkdir", "-p", normalize_path(absolute_path(p)));
    }
    path absolute_path(const path &p) const {
        return p.is_absolute() ? p : (normalize_path(home_dir() / p));
    }
    bool exists(const path &p) {
        try {
            command("ls", normalize_path(absolute_path(p)));
            return true;
        } catch (std::exception &) {
            return false;
        }
    }
    path ssh_environment_file() {
        return home_dir() / ".ssh" / "environment";
    }
    path find_key() {
        auto fn = home_dir() / ".ssh" / "id_rsa";
        if (!exists(fn)) {
            fn = home_dir() / ".ssh" / "id_ed25519";
            if (!exists(fn)) {
                return {};
            }
        }
        return fn;
    }
    void append_file(const path &p, const std::string &data) {
        auto d = path{".sw/lambda/data"} += std::format("/{}", serv.id);
        auto fn = d / p.filename();
        if (exists(p)) {
            rsync_from(absolute_path(p), fn);
            ::write_file(fn, ::read_file(fn) + data);
        } else {
            ::write_file(fn, data);
        }
        rsync_to(fn, absolute_path(p));
    }

    auto git() {
        return git_command{*this, home_dir()};
    }

private:
    auto rsync_base(auto &&from, auto &&to, auto &&opts) {
        bool pwset{};
        auto pwdenv = "PASSWORD"s;
        auto &s = main_user ? main_user->serv : serv;
        // we need a tricky solution here
        if (sudo_mode || main_user && !is_root()) {
            auto u = " -u " + name;
            if (!main_user) {
                u.clear();
            }
            if (sudo_mode_with_password()) {
                auto &user_ = main_user ? *main_user : *this;
                auto dir = user_.home_dir() / ".sw" / "rsync";
                auto asker = "asker.sh"s;
                auto fn = normalize_path(dir / asker);
                auto su = s.sudo();
                su.create_directories(dir);
                auto contents = std::format("#!/bin/sh\necho ${}\n", pwdenv);
                //if (main_user) {
                    auto tmp = normalize_path(user_.home_dir() / ".sw" / "tmp");
                    user_.serv.create_directories(tmp);
                    user_.serv.write_file(normalize_path(tmp / asker), contents);
                    su.chmod(755, normalize_path(tmp / asker));
                    su.command("cp", normalize_path(tmp / asker), fn);
                    //} else {
                    //    throw std::runtime_error{ "empty pass" };
                    //    // we can loop here
                    //    su.write_file(fn, contents);
                    //}
                su.chmod(755, fn);
                opts.arguments.push_back("--rsync-path=SUDO_ASKPASS=" + normalize_path(fn).string() + " sudo -A" + u + " rsync");
                //s.environment["SUDO_ASKPASS"] = normalize_path(fn).string(); // also as env file
                // if the following does not work, sshd config does not use env file
                s.environment[pwdenv] = main_user ? main_user->password : password;
                if (s.environment[pwdenv].empty()) {
                    throw std::runtime_error{"empty pass"};
                }
                s.set_environment();
                pwset = true;
            } else {
                opts.arguments.push_back("--rsync-path=sudo" + u + " rsync");
            }
        }

        opts.arguments.push_back("-e");
        opts.arguments.push_back(serv.connection_string());
        opts.arguments.push_back(from);
        opts.arguments.push_back(to);
        /*if (!serv.cwd.empty()) {
            SW_UNIMPLEMENTED;
        }*/
        SCOPE_EXIT {
            if (pwset) {
                s.environment.erase(pwdenv);
                s.set_environment();
            }
        };
        return primitives::deploy::rsync(opts);
    }
public:
    auto rsync_from(const path &in_from, const path &in_to, rsync_options opts = default_rsync_options()) {
        return rsync_base(make_rsync_remote(in_from), make_rsync_local(in_to), opts);
    }
    auto rsync_to(const path &in_from, const path &in_to = "."s, rsync_options opts = default_rsync_options()) {
        return rsync_base(make_rsync_local(in_from), make_rsync_remote(in_to), opts);
    }

private:
    std::string make_rsync_remote(const path &in) {
        std::string s;
        s += serv.server_string(main_user ? main_user->name : name);
        s += ":";
        // is_absolute() checks on windows side, remote is usually linux with / root
        auto is_abs = in.is_absolute() || in.string().starts_with('/');
        if (!is_abs && !cwd.empty()) {
            s += normalize_path(cwd).string() + "/";
            if (in.empty()) {
                s += "."; // for safety?
            }
            s += normalize_path(in).string();
        } else if (!is_abs) {
            s += normalize_path(absolute_path(in)).string();
        } else {
            s += normalize_path(in).string();
        }
        return s;
    }
};

struct deploy_single_target_args {
    struct from_to_path {
        path from;
        path to{"."s};

        from_to_path(const path &p) : from{p} {
        }
        from_to_path(const char *p) : from{p} {
        }
        template <auto N>
        from_to_path(const char (&p)[N]) : from{p} {
        }
        from_to_path(auto &&from, auto &&to) : from{from}, to{to} {
        }

        /*from_to_path() = default;
        from_to_path(const path &p) : from{p} {
        }
        from_to_path(const path &from, const path &to) : from{from}, to{to} {
        }
        from_to_path(const from_to_path &) = default;
        from_to_path(from_to_path &&) = default;*/
    };

    path localdir;
    path build_file;
    std::string service_name;
    std::vector<std::string> service_args;
    std::string service_pre_script;
    std::string target_package_path;
    std::string settings_data;
    // from copied first to special data dir
    std::set<path> sync_files_from;
    // 'to' files copied from the other location to replace server files
    std::vector<from_to_path> sync_files_to;
    std::vector<std::string> sync_files_to_exclude;
    std::vector<from_to_path> deploy_files_once;
    //std::set<path> backup_files;
    std::set<std::string> fedora_packages;
    std::set<int> tcp_ports;
    bool postgres_db{};
    bool nginx{};
    std::function<void(void)> custom_build;
    path custom_exe_fn;
    std::map<std::string, std::string> environment;
    std::string prognamever;
    std::string cfgname{"r"};
    std::map<std::string, std::string> nginx_configs;
    bool restore_from_last_backup{};
    bool stop_and_disable_service_only{};

    auto cfgname_dir() {
        return "deploy_" + cfgname;
    }
    void backup_service_data(auto &&serv, const path &backup_root_dir) {
        auto u = serv.login(service_name);
        auto home = u.current_user().home_dir();

        auto backup_dir = backup_root_dir;
        fs::create_directories(backup_dir);
        for (auto &&f : sync_files_from) {
            auto src = home / f;
            serv.rsync_from(normalize_path(src).string(), backup_dir);
        }

        if (postgres_db) {
            auto src = normalize_path(home / "pg.sql").string();
            SCOPE_EXIT {
                serv.remove(src);
            };
            //auto postgres = serv.login("postgres");
            auto dump = u.command("pg_dump", "-d", service_name);
            write_file(backup_dir / "pg.sql", dump);
        }
    }
    static auto make_nginx_reverse_proxy_config(auto &&hostname, auto port, auto &&username) {
        auto s = R"(
server {
        listen 80;
        #listen [::]:80;

        server_name HOSTNAME;

        location / {
            return 301 https://$host$request_uri;
        }

        location /.well-known/ {
                root /usr/share/nginx/html/;
                autoindex on;
                autoindex_localtime on;
        }
}

server {
    listen       443 ssl;
    #listen       [::]:443 ssl;
    http2        on;
    server_name  HOSTNAME;
    #root         /home/USERNAME/www/; #? www or just home dir

    ssl_certificate /etc/letsencrypt/live/HOSTNAME/fullchain.pem; # managed by Certbot
    ssl_certificate_key /etc/letsencrypt/live/HOSTNAME/privkey.pem; # managed by Certbot
    ssl_session_cache shared:SSL:1m;
    ssl_session_timeout 10m;
    ssl_ciphers PROFILE=SYSTEM;
    ssl_prefer_server_ciphers on;

    location / {
        proxy_pass_header Cookie;
        proxy_pass http://127.0.0.1:LOCAL_PORT/;

        proxy_set_header        Host $host;
        proxy_set_header        X-Real-IP $remote_addr;
        proxy_set_header        X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header        X-Forwarded-Proto $scheme;
        proxy_set_header        Upgrade $http_upgrade;
        proxy_set_header        Connection "upgrade";
        proxy_redirect          off;
        proxy_http_version      1.1;
    }

    location /static/ {
        root /home/USERNAME/;
        autoindex on;
        expires 1h;
        add_header Cache-Control public;
    }
}
)"s;
        boost::replace_all(s, "HOSTNAME", hostname);
        boost::replace_all(s, "LOCAL_PORT", std::to_string(port));
        boost::replace_all(s, "USERNAME", username);
        return s;
    }
    auto add_nginx_reverse_proxy_config(auto &&hostname, auto port, auto &&username) {
        nginx_configs[hostname] = make_nginx_reverse_proxy_config(hostname, port, username);
    }
};

struct ssh_base1 {
    static inline int id0{};
    int id{};

    ssh_base1() {
        id = ++id0;
    }
};

template <typename RealType>
struct ssh_base : ssh_base1 {
    using user_type = user<RealType>;

    std::vector<user_type> users;
    user_type *constructing_user{};
    //path cwd;
    std::string ip_;
    std::map<std::string, std::string> environment;

    auto &current_user() {
        if (users.empty() && constructing_user) {
            return *constructing_user;
        }
        return users.at(0);
    }
    auto &set_user(this auto &&obj, auto &&...args) {
        return obj.users.emplace_back(obj, args...);
    }
    bool pseudo_tty() {
        // true except user with sudo and pass
        return false;
    }
    std::string connection_string(this auto &&obj, const ssh_options &connection_options = {}) {
        using T = std::decay_t<decltype(obj)>;

        std::string s;
        for (auto &&a : obj.connection_args(connection_options)) {
            s += a + " ";
        }
        s.resize(s.size() - 1);
        return s;
    }
    auto connection_args(this auto &&obj, const ssh_options &connection_options = {}) {
        using T = std::decay_t<decltype(obj)>;

        std::vector<std::string> args;
        args.push_back("ssh");
        if (connection_options.pseudo_tty) {
            args.push_back("-t"); // allocate pseudo tty
            if (connection_options.force_pseudo_tty) {
                args.push_back("-t"); // force!
            }
        }
        args.push_back("-o");
        args.push_back("StrictHostKeyChecking=accept-new");
        args.push_back("-o");
        args.push_back("ConnectionAttempts=10");
        args.push_back("-o");
        args.push_back("ServerAliveInterval=60");
        args.push_back("-o");
        args.push_back("ServerAliveCountMax=999");
        //if (obj.sudo_mode && obj.get_user() != "root") {
            //if (!obj.current_user().sudo_mode_with_password()) {
            if (connection_options.password_auth_no) {
                args.push_back("-o");
                args.push_back("PasswordAuthentication=no");
            }
        //}
        if constexpr (requires { T::port; }) {
            args.push_back("-p");
            args.push_back(std::format("{}", T::port));
        }
        args.push_back("-i");
        args.push_back(std::format("{}", normalize_path(obj.find_key()).string()));
        /*if (obj.connection_options.with_server) {
            args.push_back(obj.server_string());
        }*/
        return args;
    }
    std::string get_ip(this auto &&obj, int retries = 0) {
        using T = std::decay_t<decltype(obj)>;

        if (!obj.ip_.empty()) {
            return obj.ip_;
        }

        if constexpr (requires { obj.ip; }) {
            obj.ip_ = obj.ip;
        } else {
            if (obj.ip_.empty()) {
                if constexpr (requires { obj.check_ips; }) {
                    auto args = obj.connection_args();
                    for (auto &&i : obj.check_ips) {
                        threaded_logger_.log("checking ip: "s + i);

                        auto args2 = args;
                        args2.push_back(std::format("{}@{}", obj.current_user().name, i));

                        primitives::Command c;
                        c.push_back(args2);
                        c.push_back("hostname");
                        auto res = run_command_raw(c);
                        if (res.err.contains("WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!")) {
                            obj.remove_known_host(i);
                            // again
                            primitives::Command c;
                            c.push_back(args2);
                            c.push_back("hostname");
                            res = run_command_raw(c);
                        }
                        if (res.exit_code) {
                            continue;
                        }
                        if (boost::trim_copy(res.out) != obj.host) {
                            continue;
                        } else {
                            obj.ip_ = i;
                            break;
                        }
                    }
                    if (obj.ip_.empty()) {
                        // ??
                    }
                } else if constexpr (!requires { T::resolve_host; }) {
                    obj.ip_ = T::host;
                } else {
                    if (!T::resolve_host) {
                        obj.ip_ = T::host;
                    }
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
                        return obj.get_ip(retries + 1);
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
                        return obj.get_ip(retries + 1);
                    }
                }
            }
        }
        obj.remove_known_host(obj.ip_);
        return obj.ip_;
    }
    std::string server_string(this auto &&obj) {
        return obj.server_string(obj.current_user().name);
    }
    std::string server_string(this auto &&obj, auto &&username) {
        using T = std::decay_t<decltype(obj)>;

        std::string s;
        s += std::format("{}@{}", username, obj.get_ip());
        return s;
    }
    auto rsync_from(this auto &&obj, auto && ... args) {
        return obj.current_user().rsync_from(args...);
    }
    auto rsync_to(this auto &&obj, auto && ... args) {
        return obj.current_user().rsync_to(args...);
    }
    void set_environment(this auto &&obj) {
        std::string s;
        for (auto &&[k, v] : obj.environment) {
            s += std::format("{}={}\n", k, v);
        }
        obj.write_file(obj.current_user().ssh_environment_file(), s);
    }
    auto create_command(this auto &&obj, auto &&...args) {
        return obj.current_user().create_command(args...);

        primitives::Command c;
        /*SwapAndRestore sr{obj.connection_options.with_server, true};
        c.push_back(obj.connection_args());
        c.push_back("--"); // end of ssh args
        if (!obj.cwd.empty()) {
            c.push_back("cd");
            c.push_back(obj.cwd);
            c.push_back("&&");
        }*/
        SW_UNIMPLEMENTED;
        /*if (obj.sudo_mode && obj.get_user() != "root") {
            c.push_back("sudo");
            if (obj.current_user().sudo_mode_with_password()) {
                c.push_back("-S");
            }
        }*/
        /*if (obj.get_user() == "root" && !obj.sudo_user.empty()) {
            c.push_back("sudo");
            c.push_back("-u");
            c.push_back(obj.sudo_user);
        }*/
        (c.push_back(args), ...);
        return c;
    }
    auto command(this auto &&obj, auto &&...args) {
        return obj.current_user().command(args...);
    }
    auto run_command(this auto &&obj, auto &&c) {
        return obj.current_user().run_command(c);
        /*SW_UNIMPLEMENTED;
        //auto e = obj.user_dir() / ".ssh" / "environment";
        auto e = path{"x"} / ".ssh" / "environment";
        auto has_env = !obj.environment.empty();
        if (has_env) {
            std::string s;
            for (auto &&[k,v] : obj.environment) {
                s += std::format("{}={}\n", k, v);
            }
            obj.write_file(normalize_path(e), s);
        }
        SCOPE_EXIT {
            if (has_env) {
                auto env = obj.scoped_environment();
                obj.remove(normalize_path(e));
            }
        };
        return primitives::deploy::run_command(c);*/
    }
    auto sudo(this auto &&obj) {
        return obj.root();
    }
    auto create_directories(this auto &&obj, const path &p) {
        return obj.current_user().create_directories(p);
    }
    bool is_online(this auto &&obj) {
        try {
            auto c = obj.create_command("test", "0");
            auto ret = run_command_raw(c);
            return !ret.exit_code;
        } catch (std::exception &e) {
            return false;
        }
    }
    void wait_until_online(this auto &&obj) {
        while (!obj.is_online()) {
            if constexpr (requires { obj.host; }) {
                threaded_logger_.log(std::format("host {} is not online yet, waiting...", obj.host));
            } else {
                threaded_logger_.log(std::format("host {} is not online yet, waiting...", obj.ip_));
            }
            std::this_thread::sleep_for(5s);
            obj.ip_.clear(); // ip can change after reboot
        }

        if constexpr (requires { obj.host; }) {
            threaded_logger_.log(std::format("host {} is online now: {}", obj.host, obj.ip_));
        } else {
            threaded_logger_.log(std::format("host {} is online now", obj.ip_));
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
        int n = 3;
        while (n--) {
            int n2 = 10;
            while (n2--) {
                if (sendto(fd, message.c_str(), message.length(), 0, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
                    throw std::runtime_error("Failed to send packet");
                }
            }
            std::this_thread::sleep_for(1s);
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
    path users_directory(this auto &&obj) {
        if constexpr (requires {obj.os;}) {
            if (obj.os == "macos"s) {
                return "/Users";
            }
        }
        return "/home";
    }
    // windows
    path find_key(this auto &&obj) {
        using T = std::decay_t<decltype(obj)>;
        if constexpr (requires { T::key; }) {
            auto r = find_key(T::key);
            if (r.empty()) {
                throw SW_RUNTIME_ERROR("cannot find ssh key: "s + T::key);
            }
            return r;
        } else {
            auto r = find_key("id_rsa");
            if (r.empty()) {
                r = find_key("id_ed25519");
                if (r.empty()) {
                    throw SW_RUNTIME_ERROR("cannot find ssh key: id_rsa");
                }
            }
            return r;
        }
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
        return {};
    }
    std::string pubkey(this auto &&obj) {
        auto p = obj.find_key();
        if (fs::exists(path{p} += ".pub")) {
            return boost::trim_copy(::read_file(path{p} += ".pub"));
        }
        if (fs::exists(p.parent_path() / path{p.stem()} += ".pub")) {
            return boost::trim_copy(::read_file(p.parent_path() / path{p.stem()} += ".pub"));
        }
        throw SW_RUNTIME_ERROR("cannot find ssh public key"s);
    }
    auto directory(this auto &&obj, const path &dir) {
        SW_UNIMPLEMENTED;
        //return SwapAndRestore(obj.cwd, dir);
    }
    void remove_known_host(this auto &&obj, const std::string &what = {}) {
        using T = std::decay_t<decltype(obj)>;
        auto remove = [](auto &&what) {
            std::string s = what;
            if constexpr (requires { T::port; }) {
                s = std::format("[{}]:{}", s, T::port);
            }
            std::unique_lock lk{m_ssh_keygen}; // for safety
            primitives::deploy::command("ssh-keygen", "-R", s);
            cygwin("ssh-keygen", "-R", s);
        };
        if constexpr (requires { T::host; }) {
            remove(T::host);
        }
        if (!obj.ip_.empty()) {
            remove(obj.ip_);
        }
        if (!what.empty()) {
            remove(what);
        }
    }
    void remove_all(this auto &&obj, const path &dir) {
        obj.command("rm", "-rf", normalize_path(dir));
    }
    void remove(this auto &&obj, const path &dir) {
        obj.remove_all(dir);
    }
    void system(this auto &&obj, const std::string &s) {
        obj.command(s);
    }
    bool has_user(this auto &&obj, const std::string &n) {
        auto s = obj.sudo();
        auto passwd = s.command("cat", "/etc/passwd");
        return std::ranges::any_of(std::views::split(passwd, "\n"sv), [&](auto &&v) {
            std::string_view line{v.begin(), v.end()};
            return line.starts_with(n);
        });
    }
    void create_user(this auto &&obj, const std::string &n) {
        if (obj.has_user(n)) {
            return;
        }
        obj.command("useradd", n
            , "-m" // create home
        );
        // better add key to root?
        return;

        // add our ssh key
        /*auto new_login = obj.login(n);
        auto sshdir = new_login.user_dir() / ".ssh";
        auto ak = sshdir / "authorized_keys";
        obj.create_directories(sshdir);
        obj.chmod(700, sshdir);
        obj.chown(sshdir, n);
        obj.command("echo", obj.pubkey(), "|", "sudo", "tee", "-a", normalize_path(ak));
        obj.chmod(600, ak);*/
    }
    void delete_user(this auto &&obj, const std::string &n) {
        obj.command("userdel",
            "-r", // remove files
            n);
    }
    void chmod(this auto &&obj, int r, const path &p) {
        obj.command("chmod", std::to_string(r), normalize_path(p));
    }
    void chown(this auto &&obj, const path &p, const std::string &user) {
        obj.command("chown", "-R", std::format("{}:{}",user,user), normalize_path(p));
    }
    auto read_file(this auto &&obj, const path &p) {
        auto d = path{".sw/lambda/data"};
        obj.rsync_from(normalize_path(p).string(), d / "1.txt");
        return ::read_file(d / "1.txt");
    }
    void write_file(this auto &&obj, const path &p, const std::string &data) {
        auto d = path{".sw/lambda/data"} += std::format("/{}", obj.id);
        ::write_file(d / p.filename(), data);
        obj.rsync_to(d / p.filename(), p);
    }
    void append_file(this auto &&obj, const path &p, const std::string &data) {
        auto d = path{".sw/lambda/data"} += std::format("/{}", obj.id);
        auto fn = d / p.filename();
        if (obj.exists(p)) {
            obj.rsync_from(p, fn);
            ::write_file(fn, ::read_file(fn) + data);
        } else {
            ::write_file(fn, data);
        }
        obj.rsync_to(fn, p);
    }
    void reboot(this auto &&obj, auto &&f) {
        try {
            f();
            std::this_thread::sleep_for(3s);
            obj.wait_until_online();
        } catch (std::exception &) {
            // this will throw on reboot or no?
            std::this_thread::sleep_for(3s);
            obj.wait_until_online();
        }
    }
    void reboot(this auto &&obj) {
        auto s = obj.sudo();
        obj.reboot([&]{ obj.command("reboot"); });
    }
    void reboot_with_other_users_online(this auto &&obj) {
        auto s = obj.sudo();
        obj.reboot([&] { obj.command("systemctl", "reboot", "-i"); });
    }
    static auto split_string_map(const std::string &s, const std::string &kv_delim = "="s) {
        std::map<std::string, std::string> m;
        auto lines = split_lines(s);
        for (auto &&line : lines) {
            auto p = line.find(kv_delim);
            if (p == -1) {
                throw SW_RUNTIME_ERROR("value is missing");
            }
            auto k = line.substr(0, p);
            auto v = line.substr(p + kv_delim.size());
            boost::trim(v);
            if (!v.empty() && v.front() == '\"' && v.back() == '\"') {
                v = v.substr(1);
                if (!v.empty() && v.back() == '\"') {
                    v.resize(v.size() - 1);
                }
            }
            m[k] = v;
        }
        return m;
    }
    auto detect_os(this auto &&obj) {
        using T = std::decay_t<decltype(obj)>;
        // add windows, macos
        using os_type = std::variant<ubuntu<T>, fedora<T>, arch<T>>;

        /*
        sw_vers
        ProductName:    macOS
        ProductVersion:    14.4.1
        BuildVersion:    23E224
        */
        auto s = obj.command("cat", "/etc/os-release");
        auto m = split_string_map(s);
        struct distr {
            std::string distribution;
            version_type version;
        } d;
        d.distribution = boost::to_lower_copy(m.at("ID"));
        if (d.distribution == "fedora") {
            d.version = m.at("VERSION_ID");
            return os_type{fedora{obj, d.version}};
        }
        if (d.distribution == "ubuntu") {
            d.version = m.at("VERSION_ID");
            return os_type{ubuntu{obj, d.version}};
        }
        if (d.distribution == "arch") {
            return os_type{arch{obj}};
        }
        // unimplemented os
        SW_UNIMPLEMENTED;
        //return d;
    }
    auto detect_hostname(this auto &&obj) {
        return obj.command("hostname");
    }
    void upgrade_packages(this auto &&obj, auto &&...args) {
        auto sudo = obj.sudo();
        dnf d{sudo};
        d.upgrade(args...);
        // sleep and wait for depmod finishes?
    }
    void upgrade_system(this auto &&obj) {
        auto os = obj.detect_os();
        visit(os, [&](auto &&v) {v.upgrade_system();});
    }
    bool exists(this auto &&obj, const path &p) {
        try {
            obj.command("ls", normalize_path(p));
            return true;
        } catch (std::exception &) {
            return false;
        }
    }
    auto login(this auto &&obj, const std::string &u) {
        auto o = obj;
        o.users.clear();
        o.users.emplace_back(obj.current_user().login_with_sudo(u));
        return o;
    }
    auto root(this auto &&obj) {
        auto o = obj;
        o.users.clear();
        o.users.emplace_back(obj.current_user().sudo());
        return o;
    }
    void setup_ssh(this auto &&obj) {
        auto su = obj.sudo();

        auto sshd_conf = "/etc/ssh/sshd_config";
        // bad for fedora, so we don't set for ubuntu either
        // serv.command("sed", "-i", "-e", "'s/UsePAM yes/UsePAM no/g'", sshd_conf);
        su.command("sed", "-i", "-e", "'s/PermitRootLogin yes/PermitRootLogin without-password/g'", sshd_conf);
        // since ssh 7.0?
        su.command("sed", "-i", "-e", "'s/PermitRootLogin without-password/PermitRootLogin prohibit-password/g'", sshd_conf);
        su.command("sed", "-i", "-e", "'s/#PermitRootLogin prohibit-password/PermitRootLogin prohibit-password/g'", sshd_conf);

        su.command("sed", "-i", "-e", "'s/#PubkeyAuthentication yes/PubkeyAuthentication yes/g'", sshd_conf);

        su.command("sed", "-i", "-e", "'s/PasswordAuthentication yes/PasswordAuthentication no/g'", sshd_conf);
        su.command("sed", "-i", "-e", "'s/#PasswordAuthentication yes/PasswordAuthentication no/g'", sshd_conf);
        su.command("sed", "-i", "-e", "'s/#PasswordAuthentication no/PasswordAuthentication no/g'", sshd_conf);
        su.command("sed", "-i", "-e", "'s/#PerminEmptyPasswords no/PerminEmptyPasswords no/g'", sshd_conf);

        su.command("sed", "-i", "-e", "'s/#PermitUserEnvironment no/PermitUserEnvironment yes/g'", sshd_conf);
        su.command("systemctl", "daemon-reload");
        su.command("systemctl", "restart", "sshd");
    }

    auto detect_targetline(this auto &&obj, auto &&args) {
        std::string targetline;
        if (args.custom_build) {
            return args.service_name;
        }
        sw_command s;
        auto list_output = args.build_file.empty() ? s.list_targets() : s.list_targets(args.build_file);
        auto lines = split_lines(list_output);
        if (lines.empty()) {
            throw std::runtime_error{"empty targets"s};
        } else if (args.target_package_path.empty() && lines.size() == 1) {
            targetline = lines[0];
        } else if (args.target_package_path.empty() && lines.size() != 1) {
            if (!args.service_name.empty()) {
                for (auto &&line : lines) {
                    if (line.starts_with(args.service_name + "-")) {
                        targetline = line;
                        break;
                    }
                }
            }
            if (targetline.empty()) {
                auto it = std::ranges::min_element(lines, {}, [&](auto &&el){return edit_distance(el, args.service_name);});
                if (edit_distance(*it, args.service_name) > args.service_name.size() / 2 + 6) {
                    throw std::runtime_error{"cant detect targets from sw output (empty or more than one): "s +
                                             list_output};
                }
                targetline = *it;
            }
        } else {
            for (auto &&line : lines) {
                if (line.starts_with(args.target_package_path)) {
                    targetline = line;
                }
            }
        }
        if (targetline.empty()) {
            throw std::runtime_error{"cant detect target"s};
        }
        return targetline;
    }
    void deploy_single_target(this auto &&obj, auto &&args) {
        using T = std::decay_t<decltype(obj)>;

        ScopedCurrentPath scp{args.localdir};

        auto targetline = obj.detect_targetline(args);

        auto pkg = split_string(targetline, "-");
        const auto progname = pkg.at(0);
        args.prognamever = targetline;

        if (args.service_name.empty()) {
            args.service_name = progname;
        }

        auto os = obj.detect_os();
        std::string build_command;
        visit(os
            , [&](fedora<T> &v) {
                build_command = target_gcc_config;
                auto gcc_version = v.version.getMajor() / 2 - 6;
                build_command += std::format("{}", gcc_version);
            }
            , [&](auto &v) {}
        );

        sw_command s;
        auto build_cmd = [&](auto &&...args) {
            if (build_command.empty()) {
                s.build_for(obj, args...);
            } else {
                s.build_raw(build_command, args...);
            }
        };
        if (args.custom_build) {
            args.custom_build();
        } else if (args.build_file.empty()) {
            build_cmd("-static", "-config", args.cfgname, "-config-name", args.cfgname_dir(), "--target", args.prognamever);
        } else {
            build_cmd("-static", "-config", args.cfgname, "-config-name", args.cfgname_dir(), args.build_file, "--target", args.prognamever);
        }

        auto username = args.service_name;
        auto service_name = args.service_name;
        auto root = obj.root();
        auto first_time = !root.has_user(username);
        root.create_user(username);
        systemctl ctl{root};

        if (args.nginx) {
            args.fedora_packages.insert("nginx");
            args.fedora_packages.insert("certbot");
            args.tcp_ports.insert(80);
            args.tcp_ports.insert(443);
            // for nginx
            root.command("setsebool", "-P", "httpd_can_network_connect", "1");

            // on the first run we must init certbot somehow (accept agreement or so)
        }
        if (!args.nginx_configs.empty()) {
            // get nginx root
            auto ng = root.read_file("/etc/nginx/nginx.conf");
            auto p = ng.find("root");
            if (p == -1) {
                throw std::runtime_error{"cant find nginx root"};
            }
            auto e = ng.find(";", p);
            ng = ng.substr(p+4, e-(p+4));
            boost::trim(ng);

            for (auto &&[h,s] : args.nginx_configs) {
                auto c = root.create_command("certbot", "certonly", "-d", h, "--webroot", "--renew-by-default");
                c.in.text = ng;
                root.run_command(c);
                root.write_file("/etc/nginx/conf.d/" + h + ".conf", s);
            }
        }
        if (args.nginx) {
            auto svc = ctl.service("nginx");
            if (svc.status().exit_code != 0) {
                svc.enable();
                svc.start();
            } else {
                // or just nginx -s reload?
                svc.restart();
            }
        }

        std::string pkgs;
        for (auto &&p : args.fedora_packages) {
            // FIXME: match os <-> packages
            pkgs += p + " ";
        }
        if (!pkgs.empty()) {
            visit(os, [&](auto &v){v.package_manager().install(pkgs);});
        }
        // make init_openvpn_server() method
        // easyrsa --nopass init-pki
        // easyrsa --nopass build-ca
        // easyrsa --nopass gen-dh
        // easyrsa --nopass build-server-full server
        // easyrsa --nopass build-client-full client1
        // firewall-cmd add port ........
        // semanage port -a -t openvpn_port_t -p tcp 1198
        for (auto &&p : args.tcp_ports) {
            root.command("firewall-cmd", "--permanent", "--add-port", std::to_string(p) + "/tcp");
        }
        root.command("firewall-cmd", "--reload");

        auto svc = ctl.service(service_name);
        if (svc.status().exit_code == 0) {
            svc.stop();
        }
        if (svc.status().exit_code == 4) {
            first_time = true;
        }

        // maybe put our public key into 'authorized_keys'
        // and sign in instead of root?

        auto fn = path{".sw"} / "out" / args.cfgname_dir() / args.prognamever;
        if (args.custom_build && !args.custom_exe_fn.empty()) {
            fn = args.custom_exe_fn;
        }
        auto u = root.login(username);
        auto home = u.current_user().home_dir();
        u.rsync_to(fn, home);
        auto prog = normalize_path(home / fn.filename());
        root.chmod(755, prog);
        root.chown(prog, username);
        // selinux
        root.command("chcon", "-t", "bin_t", normalize_path(prog));

        if constexpr (requires {args.custom_steps(obj);}) {
            args.custom_steps(obj);
        }

        if (!args.service_pre_script.empty()) {
            root.write_file(normalize_path(home / "pre.sh"), "#!/bin/bash\n\n" + args.service_pre_script + "\n");
            root.chmod(755, normalize_path(home / "pre.sh"));
            root.chown(normalize_path(home / "pre.sh"), username);
        }

        if (!args.settings_data.empty()) {
            auto settings = home / progname += ".settings";
            root.write_file(settings, args.settings_data);
            root.chown(settings, username);
        }
        // backup first
        if (!first_time) {
            args.backup_service_data(root, obj.backup_dir(args.service_name));
        }
        if (args.restore_from_last_backup) {
            auto backup_dir = obj.latest_backup_dir(args.service_name);
            for (auto &&f : args.sync_files_from) {
                auto src = backup_dir / f;
                auto dst = home / f;
                if (!fs::exists(src)) {
                    threaded_logger_.log(std::format("sync_files_to: {} does not exists", src.string()));
                    continue;
                }
                // cant use 'a' flag because of -user and -owner flags
                rsync_options opts;
                opts.arguments.clear();
                opts.arguments.push_back("-crvP");
                u.rsync_to(src, dst, opts);
                //root.chmod(755, dst); // for dirs
                //root.chmod(644, dst); // for files
                // --chmod=Du=rwx,Dg=rx,Do=rx,Fu=rw,Fg=r,Fo=r
                root.chown(dst, username);
            }
        }
        // then to
        for (auto &&[from,to] : args.sync_files_to) {
            auto dst = home / to;
            // cant use 'a' flag because of -user and -owner flags
            rsync_options opts;
            opts.arguments.clear();
            opts.arguments.push_back("-crvP");
            opts.exclude = args.sync_files_to_exclude;
            u.rsync_to(from, dst, opts);
            //root.chmod(755, dst); // for dirs
            //root.chmod(644, dst); // for files
            // --chmod=Du=rwx,Dg=rx,Do=rx,Fu=rw,Fg=r,Fo=r
            root.chown(dst, username);
        }
        if (first_time) {
            for (auto &&[from, to] : args.deploy_files_once) {
                auto dst = home / to;
                // cant use 'a' flag because of -user and -owner flags
                rsync_options opts;
                opts.arguments.clear();
                opts.arguments.push_back("-crvP");
                u.rsync_to(from, dst, opts);
                // root.chmod(755, dst); // for dirs
                // root.chmod(644, dst); // for files
                //  --chmod=Du=rwx,Dg=rx,Do=rx,Fu=rw,Fg=r,Fo=r
                root.chown(dst, username);
            }
        }

        if (args.postgres_db) {
            visit(os, [&](auto &v) {
                v.install_postgres();
            });
            auto postgres = root.login("postgres");
            // user, not role; users can login
            auto x = postgres.command("psql", "-t", "-c", "'select count(*) from pg_user where usename = $$" + username + "$$;'");
            boost::trim(x);
            if (x == "0") {
                auto run_and_dont_fail_on_exists = [&](auto && ... args2) {
                    auto c = postgres.create_command(args2...);
                    auto r = run_command_raw(c);
                    if (!r) {
                        if (!(c.out.text.contains("already exists") || c.err.text.contains("already exists"))) {
                            throw std::runtime_error{"cant create postgres object"};
                        }
                    }
                };
                run_and_dont_fail_on_exists("psql", "-c", "'create user " + args.service_name + " password $$" + args.service_name + "$$;'");
                run_and_dont_fail_on_exists("psql", "-c", "'create database " + args.service_name + " owner " + args.service_name + ";'");
            }
        }

        if (first_time) {
            if constexpr (requires { args.run_once(root, prog); }) {
                args.run_once(root, prog);
            }
        }

        {
            auto svc = ctl.new_simple_service();
            svc.name = service_name;
            svc.progname = fn.filename().string();
            svc.args = args.service_args;
            svc.service_pre_script = args.service_pre_script;
            svc.environment = args.environment;
            ctl.new_simple_service_auto(svc);
        }
    }
    void discard_single_target(this auto &&obj, auto &&args) {
        ScopedCurrentPath scp{args.localdir};

        auto targetline = obj.detect_targetline(args);

        auto pkg = split_string(targetline, "-");
        const auto progname = pkg.at(0);
        args.prognamever = targetline;

        if (args.service_name.empty()) {
            args.service_name = progname;
        }

        auto service_name = args.service_name;
        auto username = args.service_name;

        auto root = obj.root();
        if (!root.has_user(username)) {
            return;
        }

        if (args.stop_and_disable_service_only) {
            systemctl ctl{ root };
            auto svc = ctl.service(service_name);
            svc.stop();
            svc.disable();
            args.backup_service_data(root, obj.backup_dir(args.service_name));
            return;
        }

        auto os = obj.detect_os();
        auto osname = visit(os, [](auto &&v){return v.name;});
        if (osname != "arch") {
            for (auto &&p : args.tcp_ports) {
                root.command("firewall-cmd", "--permanent", "--remove-port", std::to_string(p) + "/tcp");
            }
            root.command("firewall-cmd", "--reload");
        }

        systemctl ctl{root};
        ctl.delete_simple_service(service_name);

        args.backup_service_data(root, obj.backup_dir(args.service_name));
        if (args.postgres_db) {
            root.command("psql", "-U", "postgres", "-c", "'drop database " + args.service_name + ";'");
            root.command("psql", "-U", "postgres", "-c", "'drop user " + args.service_name + ";'");
        }

        root.delete_user(username);

        if constexpr (requires { args.cleanup(root); }) {
            args.cleanup(root);
        }
    }
};

} // namespace primitives::deploy
