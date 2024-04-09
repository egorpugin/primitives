#pragma once

#ifdef _WIN32

#include <windows.h>

namespace win32 {

#define WINAPI_CALL(x) if (!(x)) {throw ::win32::winapi_exception{#x};}
#define WINAPI_CALL_HANDLE(x) ::win32::handle{x,[]{throw ::sw::win32::winapi_exception{"bad handle from " #x};}}

struct winapi_exception : std::runtime_error {
    template <typename F>
    struct scope_exit {
        F &&f;
        ~scope_exit() {
            f();
        }
    };

    using base = std::runtime_error;
    winapi_exception(const std::string &msg) : base{msg + ": "s + get_last_error()} {
    }
    std::string get_last_error() const {
        auto code = GetLastError();

        LPVOID lpMsgBuf;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                      code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
        scope_exit se{[&]{LocalFree(lpMsgBuf);}};
        std::string msg = (const char *)lpMsgBuf;

        return "error code = "s + std::to_string(code) + ": " + msg;
    }
};

struct handle {
    HANDLE h{INVALID_HANDLE_VALUE};

    handle() = default;
    handle(HANDLE h, auto &&err) : h{h} {
        if (h == INVALID_HANDLE_VALUE || !h) {
            err();
        }
    }
    handle(HANDLE h) : handle{h,[]{throw winapi_exception{"bad handle"};}} {
    }
    handle(const handle &) = delete;
    handle &operator=(const handle &) = delete;
    handle(handle &&rhs) noexcept {
        operator=(std::move(rhs));
    }
    handle &operator=(handle &&rhs) noexcept {
        this->~handle();
        h = rhs.h;
        rhs.h = INVALID_HANDLE_VALUE;
        return *this;
    }
    ~handle() {
        reset();
    }

    operator HANDLE() & { return h; }
    operator HANDLE() && { return release(); }
    operator HANDLE*() { return &h; }

    void reset() {
        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }
    HANDLE release() {
        auto hold = h;
        h = INVALID_HANDLE_VALUE;
        return hold;
    }
};

auto create_job_object() {
    handle job = CreateJobObject(0, 0);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ji;
    WINAPI_CALL(QueryInformationJobObject(job, JobObjectExtendedLimitInformation, &ji, sizeof(ji), 0));
    ji.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    WINAPI_CALL(SetInformationJobObject(job, JobObjectExtendedLimitInformation, &ji, sizeof(ji)));
    return job;
}
HANDLE default_job_object() {
    static auto job = []() {
        // we do release here, otherwise process will be terminated with 0 exit code
        // on CloseHandle during normal exit process
        auto job = create_job_object().release();
        /*BOOL injob{false};
        // check if we are a child in some job already
        // injob is probably true under debugger/visual studio
        WINAPI_CALL(IsProcessInJob(GetCurrentProcess(), 0, &injob));*/
        // assign this process to a job
        if (1 /*|| !injob*/) {
            WINAPI_CALL(AssignProcessToJobObject(job, GetCurrentProcess()));
        }
        return job;
    }();
    return job;
}

} // namespace win32

#endif