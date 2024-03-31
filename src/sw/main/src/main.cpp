// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#define SW_MAIN_IMPL
#include <primitives/sw/main.h>

#include <primitives/sw/settings.h>
#include <primitives/sw/cl.h>
#include <primitives/sw/settings_program_name.h>
#include <primitives/main.h>

#include <boost/dll.hpp>

#include <random>
#include <thread>

#define CRASH_SERVER_OPTION "internal-with-crash-server"
#define CRASH_SERVER_OPTION_START "internal-with-crash-server-start-server"

static cl::opt<int> with_crash_server(CRASH_SERVER_OPTION,
    cl::desc("Start crash handler server on startup. Use this option to record a crash dump."),
    cl::Hidden, cl::ValueOptional);

static cl::opt<String> crash_server_pipe_name(CRASH_SERVER_OPTION_START,
    cl::desc("Start crash handler server."),
    cl::ReallyHidden);

static ::cl::opt<bool, true> bt("bt", ::cl::desc("Print backtrace"), ::cl::location(sw::gUseStackTrace), ::cl::Hidden, ::cl::sub(::cl::getAllSubCommands()));
static ::cl::opt<bool, true> doe("doe", ::cl::desc("Debug on Exception"), ::cl::location(sw::gDebugOnException), ::cl::Hidden, ::cl::sub(::cl::getAllSubCommands()));
static ::cl::alias bt2("st", ::cl::desc("Alias for -bt"), ::cl::aliasopt(bt));

static ::cl::opt<std::string, true> symbol_path("symbol-path", ::cl::desc("Set symbol path for backtrace"), ::cl::location(sw::gSymbolPath), ::cl::Hidden, ::cl::sub(::cl::getAllSubCommands()));

static ::cl::opt<int> sleep_seconds("sleep", ::cl::desc("Sleep on startup"), ::cl::Hidden, ::cl::sub(::cl::getAllSubCommands()));
extern bool gPauseOnError;
static ::cl::opt<bool, true> pause_on_error("pause-on-error", ::cl::desc("Pause on program error"), ::cl::Hidden, ::cl::location(gPauseOnError), ::cl::sub(::cl::getAllSubCommands()));

static path temp_directory_path(const path &subdir)
{
    return fs::temp_directory_path() / "sw" / subdir;
}

static path get_crash_dir()
{
    auto p = temp_directory_path(::sw::getProgramNameSilent()) / "dump";
    fs::create_directories(p);
    return p;
}

static std::string get_cl_option_base()
{
    return "-" PACKAGE_NAME_CLEAN ".internal.start-minidump-server";
}

#if defined(_WIN32) && !defined(__MINGW32__)
#include <exception_handler.h>
#include <crash_generation_server.h>
#include <common/windows/guid_string.h>

#include <Windows.h>

static google_breakpad::CrashGenerationServer* crash_server;
static std::atomic_bool client_connected;

static google_breakpad::ExceptionHandler* handler;
static std::string client_pipe_name;
static std::wstring client_pipe_name_prepared;
static const wchar_t *client_pipe_name_prepared_c_str;
static std::string filter_message;
static std::string after_minidump_write_message;

static std::string get_object_prefix()
{
    return "sw_main_";
}

static std::string get_event_name_base()
{
    return "Local\\" + get_object_prefix();
}

static std::string get_pipe_name_base()
{
    return "\\\\.\\pipe\\" + get_object_prefix();
}

static int StartCrashGenerationServer(const std::string &win_pipe_id)
{
    auto dump_path = get_crash_dir().wstring();

    auto on_client_connect = [](void* context, const google_breakpad::ClientInfo* client_info)
    {
        client_connected = true;
    };

    auto on_client_disconnect = [](void* context, const google_breakpad::ClientInfo* client_info)
    {
        exit(0);
    };

    crash_server = new google_breakpad::CrashGenerationServer(to_wstring(get_pipe_name_base() + win_pipe_id), 0,
        on_client_connect, 0,
        0, 0,
        on_client_disconnect, 0,
        0, 0,
        true, &dump_path
    );

    auto r = crash_server->Start();
    if (r)
    {
        auto ev = OpenEvent(EVENT_MODIFY_STATE, 0, to_wstring(get_event_name_base() + win_pipe_id).c_str());
        if (!ev)
        {
            printf("OpenEvent failed: %d\n", GetLastError());
            return 1;
        }
        if (!SetEvent(ev))
        {
            printf("SetEvent failed: %d\n", GetLastError());
            return 1;
        }
        Sleep(10000);
        if (client_connected)
            Sleep(INFINITE);
        else
            r = false;
    }
    return !r;
}

static int LaunchCrashServerHelper()
{
    auto prog = boost::dll::program_location().wstring();
    auto cmd = L'\"' + prog + L'\"';
    cmd += to_wstring(" " + get_cl_option_base() + " " + client_pipe_name);

    auto pipe_name = to_wstring(get_pipe_name_base() + client_pipe_name);
    auto event_name = to_wstring(get_event_name_base() + client_pipe_name);

    STARTUPINFO si{ 0 };
    PROCESS_INFORMATION pi{ 0 };

    auto ev = CreateEvent(0, 0, 0, event_name.c_str());
    if (!ev)
    {
        printf("CreateEvent failed: %d\n", GetLastError());
        return false;
    }
    if (!CreateProcess(prog.c_str(), cmd.data(), 0, 0, 0, 0, 0, 0, &si, &pi))
    {
        printf("CreateProcess failed: %d\n", GetLastError());
        return false;
    }
    if (WaitForSingleObject(ev, 5000))
    {
        printf("Cannot connect to crash server\n");
        return false;
    }

    return true;
}

static void print_string_safe(const std::string &s)
{
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), s.data(), s.size(), 0, 0);
}

static bool filter_cb(void *context, EXCEPTION_POINTERS *exinfo, MDRawAssertionInfo *assertion)
{
    print_string_safe(filter_message);
    return true;
}

static bool after_minidump_write_cb(const wchar_t *dump_path,
    const wchar_t *minidump_id,
    void *context,
    EXCEPTION_POINTERS *exinfo,
    MDRawAssertionInfo *assertion,
    bool succeeded)
{
    print_string_safe(after_minidump_write_message);
    return succeeded;
}
#endif

void sw_append_symbol_path(const path &in)
{
    // bt is not available at this time
    //if (!bt)
        //return;

    // not working atm
    return;

#if defined(_WIN32) && !defined(__MINGW32__)
    const auto nt_symbol_path = L"_NT_SYMBOL_PATH";
    const auto delim = L";";
    const auto max_env_var_size = 32'767;

    WCHAR buf[max_env_var_size];
    if (!GetEnvironmentVariableW(nt_symbol_path, buf, max_env_var_size))
    {
        auto e = GetLastError();
        if (e != ERROR_ENVVAR_NOT_FOUND)
        {
            printf("GetEnvironmentVariableW failed: %d", e);
            return;
        }
    }

    std::wstring p = buf;
    p += delim + in.wstring();
    SetEnvironmentVariableW(nt_symbol_path, p.c_str());
#endif
}

static int startup(int argc, char *argv[])
{
    for (int i = 1; i < argc - 1; i++)
    {
        if (argv[i] == "-sleep"s || argv[i] == "--sleep"s)
        {
            std::this_thread::sleep_for(std::chrono::seconds(std::stoi(argv[i + 1])));
            break;
        }
    }

#if defined(_WIN32) && !defined(__MINGW32__)
    // prepare strings
    filter_message = "Unhandled exception.\nWriting minidump to "s + to_string(get_crash_dir()) + "\n";
    after_minidump_write_message = "Minidump saved.\n";

    //
    auto option_name = get_cl_option_base();
    if (argc > 1 && argv[1] == option_name)
    {
        // out-of-process server
        cl::opt<std::string> id(cl::arg{ option_name.substr(1) }, cl::Required);
        cl::ParseCommandLineOptions(argc, argv);

        return StartCrashGenerationServer(id);
    }
    else if (argc > 1 && String(argv[1]).find(CRASH_SERVER_OPTION) != String::npos)
    {
        // with early out-of-process server
        int t = MiniDumpNormal;
        auto p = String(argv[1]).find('=');
        if (p != String::npos)
            t = std::stoi(String(argv[1]).substr(p + 1));
        // start server and handler
        sw_enable_crash_server(t);
    }
    else if (!IsDebuggerPresent())
    {
        // in-process handler
        handler = new google_breakpad::ExceptionHandler(get_crash_dir(),
            filter_cb, after_minidump_write_cb, nullptr, google_breakpad::ExceptionHandler::HANDLER_ALL);
    }
#endif

#if defined(_WIN32) && !defined(__MINGW32__)
    //SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    // fix current path:
    //  make it canonical (big drive letter on Windows)
    //  make it lexically normal
    //  remove trailing slashes

    // we also use fs::u8path() here, because normalize_path() converts it to utf-8

    auto cp = fs::current_path();
    auto s = to_path_string(normalize_path(cp.lexically_normal()));
    while (!s.empty() && s.back() == '/')
        s.resize(s.size() - 1);
#if defined(_WIN32) && !defined(__MINGW32__)
    // windows does not change case of the disk letter if other parts unchanged
    cp = s;
    cp = normalize_path_windows(cp);
    // so we take parent dir first, set cd there
    cp = cp.parent_path();
    fs::current_path(cp);
    // then we restore our cwd but with our changes
#endif
    cp = s;
    fs::current_path(cp);

    //
#if defined(_WIN32) && !defined(__MINGW32__)
    sw_append_symbol_path(boost::dll::program_location().parent_path().wstring());
#else
    sw_append_symbol_path(boost::dll::program_location().parent_path().string());
#endif
    sw_append_symbol_path(fs::current_path());

    // init settings early
    // e.g. before parsing command line
    sw::getSettingStorage();

    return 0;
}

// main or backup main
int main(int argc, char *argv[])
{
    return sw_main_internal(argc, argv);
}

int sw_main_internal(int argc, char *argv[])
{
    if (int r = startup(argc, argv))
        return r;
    return SW_MAIN(argc, argv);
}

void sw_enable_crash_server(int level)
{
#if defined(_WIN32) && !defined(__MINGW32__)
    delete handler; // delete previous always

    if (level < 0)
        return;

    // with early explicit server
    client_pipe_name = std::to_string(std::random_device()());
    client_pipe_name_prepared = to_wstring(get_pipe_name_base() + client_pipe_name);
    client_pipe_name_prepared_c_str = client_pipe_name_prepared.c_str();
    if (!LaunchCrashServerHelper())
    {
        printf("Cannot start crash server.\n");
        exit(1); // exit immediately? yes for now
        return;
    }
    handler = new google_breakpad::ExceptionHandler(get_crash_dir(),
        filter_cb, after_minidump_write_cb, nullptr, google_breakpad::ExceptionHandler::HANDLER_ALL,
        (MINIDUMP_TYPE)level, client_pipe_name_prepared_c_str, nullptr);
#endif
}
