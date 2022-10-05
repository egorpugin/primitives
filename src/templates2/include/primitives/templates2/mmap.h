#pragma once

#include <filesystem>
#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace primitives::templates2 {

namespace fs = std::filesystem;

#ifdef _WIN32
struct handle {
    HANDLE h{INVALID_HANDLE_VALUE};
    handle() = default;
    handle(HANDLE h, auto &&err) : h{h} {
        if (h == INVALID_HANDLE_VALUE) {
            err();
        }
    }
    handle(HANDLE h) : handle{h, []{throw std::runtime_error{"invalid handle"};}} {
    }
    handle(const handle &) = delete;
    handle &operator=(const handle &) = delete;
    handle(handle &&rhs) {
        std::swap(h, rhs.h);
    }
    handle &operator=(handle &&rhs) {
        std::swap(h, rhs.h);
        return *this;
    }
    ~handle() {
        if (h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
        }
    }
    operator HANDLE() const { return h; }
};
#endif

template <typename T = char>
struct mmap_file {
    using size_type = uint64_t;

#ifdef _WIN32
    handle f;
    handle m;
#else
    int fd;
#endif
    T *p{nullptr};
    size_type sz;

    mmap_file(const fs::path &fn) {
        sz = fs::file_size(fn);
        if (sz == 0) {
            return;
        }
#ifdef _WIN32
        f = handle{CreateFileW(fn.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0),
            [&]{ throw std::runtime_error{"cannot open file: " + fn.string()}; }
        };
        m = CreateFileMappingW(f, 0, PAGE_READONLY, 0, sz, 0);
        if (m == 0) {
            throw std::runtime_error{"cannot create file mapping"};
        }
        p = (T *)MapViewOfFile(m, FILE_MAP_READ, 0, 0, sz);
        if (!p) {
            throw std::runtime_error{"cannot map file"};
        }
#else
        fd = open(fn.string().c_str(), O_RDONLY);
        if (fd == -1) {
            throw std::runtime_error{"cannot open file: " + fn.string()};
        }
        p = (char *)mmap(0, sz, PROT_READ, MAP_PRIVATE, fd, 0);
        if (p == MAP_FAILED) {
            close(fd);
            throw std::runtime_error{"cannot create file mapping"};
        }
#endif
    }
    ~mmap_file() {
#ifdef _WIN32
        if (p) {
            UnmapViewOfFile(p);
        }
#else
        close(fd);
#endif
    }
    auto &operator[](int i) { return p[i]; }
    const auto &operator[](int i) const { return p[i]; }
    bool eof(size_type pos) const { return pos >= sz; }
    operator T*() const { return p; }
    template <typename U>
    operator U*() const { return (U*)p; }

    auto begin() const { return p; }
    auto end() const { return p+sz; }
};

} // namespace primitives::templates2
