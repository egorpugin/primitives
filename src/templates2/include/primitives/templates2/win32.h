// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2022 Egor Pugin <egor.pugin@gmail.com>

#pragma once

#ifdef _WIN32

#include <format>
#include <string>
#include <stdexcept>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#define WINAPI_CALL(x) if (!(x)) {if (auto code = GetLastError()) throw ::win32::winapi_exception{#x, code};}
#define WINAPI_CALL_HRESULT(x) {if (auto hr = (x); hr != S_OK) {throw ::win32::winapi_exception{#x, hr};}}

// we use this for easy command line building/bootstrapping
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "OleAut32.lib")

namespace win32 {

using namespace std::literals;

struct winapi_exception : std::runtime_error {
    using base = std::runtime_error;
    winapi_exception(const std::string &msg) : base{msg + ": "s + get_last_error()} {
    }
    winapi_exception(const std::string &msg, DWORD code) : base{ msg + ": "s + get_error_text(code) } {
    }
    winapi_exception(const std::string &msg, HRESULT code) : base{ msg + std::format(": 0x{:X}", (uint32_t)code) } {
    }
    std::string get_last_error() const {
        auto code = GetLastError();
        return get_error_text(code);
    }
    static std::string get_error_text(auto code) {
        std::string msg;
        char buf[8192]{};
        if (FormatMessageA(
            //FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                      code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, sizeof(buf), NULL)) {
            msg = buf;
            //LocalFree(lpMsgBuf);
        } else {
            auto err = GetLastError();
            msg = std::format("FormatMessageA() failed, cannot retrieve error, error code = {}", err);
        }

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

    operator HANDLE() { return h; }
    operator HANDLE*() { return &h; }

    void reset() {
        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }
};

} // namespace win32

#endif
