#define SW_MAIN_IMPL
#include <primitives/sw/main.h>

#include <primitives/sw/settings.h>
#include <primitives/sw/cl.h>
#include <primitives/sw/settings_program_name.h>
#include <primitives/main.h>
#include <primitives/filesystem.h>

#include <boost/dll.hpp>

#include <random>

#define CRASH_SERVER_OPTION "internal-with-crash-server"
#define CRASH_SERVER_OPTION_START "internal-with-crash-server-start-server"

static cl::opt<int> with_crash_server(CRASH_SERVER_OPTION,
    cl::desc("Start crash handler server on startup. Use this option to record a crash dump."),
    cl::Hidden, cl::ValueOptional);

static cl::opt<String> crash_server_pipe_name(CRASH_SERVER_OPTION_START,
    cl::desc("Start crash handler server."),
    cl::ReallyHidden);

extern bool gUseStackTrace;
static ::cl::opt<bool, true> bt("bt", ::cl::desc("Print backtrace"), ::cl::location(gUseStackTrace));
static ::cl::alias bt2("st", ::cl::desc("Alias for -bt"), ::cl::aliasopt(bt));

static path temp_directory_path(const path &subdir)
{
    return fs::temp_directory_path() / "sw" / subdir;
}

static path get_crash_dir()
{
    auto p = temp_directory_path(::sw::getProgramName()) / "dump";
    fs::create_directories(p);
    return p;
}

static std::string get_cl_option_base()
{
    return "-" PACKAGE_NAME_CLEAN ".internal.start-minidump-server";
}

#ifdef _WIN32
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
static std::atomic_bool handled_on_server;

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

static int CrashServerStart(const std::string &id)
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

    crash_server = new google_breakpad::CrashGenerationServer(to_wstring(get_pipe_name_base() + id), 0,
        on_client_connect, 0,
        0, 0,
        on_client_disconnect, 0,
        0, 0,
        true, &dump_path
    );

    auto r = crash_server->Start();
    if (r)
    {
        auto ev = OpenEvent(EVENT_MODIFY_STATE, 0, to_wstring(get_event_name_base() + id).c_str());
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

static int CrashServerStart2()
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

static bool filter_cb(void* context, EXCEPTION_POINTERS* exinfo, MDRawAssertionInfo* assertion)
{
    if (client_pipe_name_prepared_c_str)
    {
        // server-side handling

        /*google_breakpad::CrashGenerationClient c(client_pipe_name_prepared_c_str, MINIDUMP_TYPE::MiniDumpNormal, nullptr);
        if (!c.Register())
        {
            printf("CrashGenerationClient::Register failed\n");
            return true;
        }
        if (!c.RequestDump(exinfo, assertion))
        {
            printf("CrashGenerationClient::RequestDump failed\n");
            return true;
        }

        printf("Unhandled exception.\nWriting minidump to %s\n", get_crash_dir().u8string().c_str());
        printf("Crash server was used.\n");*/

        // return to crash handler client to handle on server
        return true;
    }

    // actually we can't safely start process here, make allocation etc.
    // we can get loader lock and (or) other issues

    // right approach is to setup crash handler server or a watchdog process
    // before exception occurs

    printf("Unhandled exception.\nWriting minidump to %s\n", get_crash_dir().u8string().c_str());

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
        return true;
    }
    if (!CreateProcess(prog.c_str(), cmd.data(), 0, 0, 0, 0, 0, 0, &si, &pi))
    {
        printf("CreateProcess failed: %d\n", GetLastError());
        return true;
    }
    if (WaitForSingleObject(ev, 5000))
    {
        printf("Cannot connect to crash server\n");
        return true;
    }

    google_breakpad::CrashGenerationClient c(pipe_name.c_str(), MINIDUMP_TYPE::MiniDumpNormal, nullptr);
    if (!c.Register())
    {
        printf("CrashGenerationClient::Register failed\n");
        return true;
    }
    if (!c.RequestDump(exinfo, assertion))
    {
        printf("CrashGenerationClient::RequestDump failed\n");
        return true;
    }

    printf("Minidump is written using crash server started from inside the app\n");

    // dangerous, but we'll try
    exit(1);

    handled_on_server = true;
    return true;
}

// not used at the moment, as we do exit in filter_cb
static bool minidump_cb(const wchar_t *dump_path,
                 const wchar_t *minidump_id,
                 void *context,
                 EXCEPTION_POINTERS *exinfo,
                 MDRawAssertionInfo *assertion,
                 bool succeeded)
{
    // server-side handling
    if (client_pipe_name_prepared_c_str)
    {
        if (succeeded)
        {
            printf("Unhandled exception.\nWriting minidump to %s\n", get_crash_dir().u8string().c_str());
            printf("Crash server was used.\n");
        }
        else
        {
            printf("Unhandled exception.\nCannot write dump.\n");
        }

        return succeeded;
    }

    // client-side handling
    if (handled_on_server)
    {
        auto p = path(dump_path) / minidump_id;
        p += ".dmp";

        error_code ec;
        fs::remove(p, ec);
        if (ec)
            printf("fs::remove failed\n");
    }
    return succeeded;
}
#endif

static int setup(int argc, char *argv[])
{
#ifdef _WIN32
    auto option_name = get_cl_option_base();
    if (argc > 1 && argv[1] == option_name)
    {
        // server
        cl::opt<std::string> id(cl::arg{ option_name.substr(1) }, cl::Required);
        cl::ParseCommandLineOptions(argc, argv);

        return CrashServerStart(id);
    }
    else if (argc > 1 && String(argv[1]).find(CRASH_SERVER_OPTION) != String::npos)
    {
        // with early explicit server
        client_pipe_name = std::to_string(std::random_device()());
        client_pipe_name_prepared = to_wstring(get_pipe_name_base() + client_pipe_name);
        client_pipe_name_prepared_c_str = client_pipe_name_prepared.c_str();
        if (!CrashServerStart2())
        {
            printf("Cannot start crash server.\n");
            return 1;
        }
        int t = MiniDumpNormal;
        auto p = String(argv[1]).find('=');
        if (p != String::npos)
            t = std::stoi(String(argv[1]).substr(p + 1));
        if (t < 0)
            t = MiniDumpNormal;
        handler = new google_breakpad::ExceptionHandler(get_crash_dir(),
            filter_cb, minidump_cb, nullptr, google_breakpad::ExceptionHandler::HANDLER_ALL,
            (MINIDUMP_TYPE)t, client_pipe_name_prepared_c_str, nullptr);
    }
    else
    {
        // client
        client_pipe_name = std::to_string(std::random_device()());
        handler = new google_breakpad::ExceptionHandler(get_crash_dir(),
            filter_cb, minidump_cb, nullptr, google_breakpad::ExceptionHandler::HANDLER_ALL);
    }
#endif

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // fix current path:
    //  make it canonical (big drive letter on Windows)
    //  make it lexically normal
    //  remove trailing slashes
    auto cp = fs::current_path();
    auto s = normalize_path(cp.lexically_normal());
    while (!s.empty() && s.back() == '/')
        s.resize(s.size() - 1);
#ifdef _WIN32
    s = normalize_path_windows(s);
    // windows does not change case of the disk letter if other parts unchanged
    cp = s;
    // so we take parent dir first, set cd there
    cp = cp.parent_path();
    fs::current_path(cp);
    // then we restore our cwd but with our changes
#endif
    cp = s;
    fs::current_path(cp);

    // init settings early
    // e.g. before parsing command line
    sw::getSettingStorage();

    return 0;
}

int main(int argc, char *argv[])
{
    if (int r = setup(argc, argv))
        return r;
    return SW_MAIN(argc, argv);
}
