#define SW_MAIN_IMPL
#include <primitives/sw/main.h>

#define PRIMITIVES_STATIC_LIB_VISIBILITY
#include <primitives/sw/settings.h>
#include <primitives/main.h>
#include <primitives/filesystem.h>

#include <boost/dll.hpp>

#include <random>

static path temp_directory_path(const path &subdir)
{
    auto p = fs::temp_directory_path() / "sw" / subdir;
    fs::create_directories(p);
    return p;
}

String getProgramName();

static path get_crash_dir()
{
    auto p = temp_directory_path(getProgramName()) / "dump";
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

int CrashServerStart(const std::string &id)
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
        SetEvent(ev);
        Sleep(10000);
        if (client_connected)
            Sleep(INFINITE);
        else
            r = false;
    }
    return !r;
}

bool filter_cb(void* context, EXCEPTION_POINTERS* exinfo, MDRawAssertionInfo* assertion)
{
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

    google_breakpad::CrashGenerationClient c(pipe_name.c_str(), MiniDumpNormal, 0);
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
    // dangerous, but we'll try
    exit(1);

    handled_on_server = true;
    return true;
}

bool minidump_cb(const wchar_t* dump_path,
    const wchar_t* minidump_id,
    void* context,
    EXCEPTION_POINTERS* exinfo,
    MDRawAssertionInfo* assertion,
    bool succeeded)
{
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

int main(int argc, char *argv[])
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
    else
    {
        // client
        client_pipe_name = std::to_string(std::random_device()());
        handler = new google_breakpad::ExceptionHandler(get_crash_dir(),
            filter_cb, minidump_cb, nullptr, google_breakpad::ExceptionHandler::HANDLER_ALL);
    }
#endif

    // init settings early
    // e.g. before parsing command line
    sw::getSettingStorage();
    return SW_MAIN(argc, argv);
}
