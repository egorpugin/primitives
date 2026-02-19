// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/exceptions.h>
#include <primitives/enums.h>
#include <primitives/string.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <iostream>
#include <mutex>
#include <regex>
#include <span>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#endif

#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

#ifndef _WIN32
#ifndef _GNU_SOURCE
#define _GNU_SOURCE // for dladdr
#endif
#include <dlfcn.h>
#endif

namespace fs = std::filesystem;
using error_code = std::error_code;

using path = fs::path;

#ifdef _WIN32
// on win and C++20 we can use std::u8string for path strings
// to convert paths in both directions safely
#define PRIMITIVES_FS_USE_UTF8_PATH_STRINGS 1
#else
#define PRIMITIVES_FS_USE_UTF8_PATH_STRINGS 0
#endif

#if PRIMITIVES_FS_USE_UTF8_PATH_STRINGS
// on win and C++20 we can use std::u8string for path strings
// to convert paths in both directions safely
using path_u8string = std::u8string;
using path_char_t = char8_t;
#else
using path_u8string = String;
using path_char_t = char;
#endif

using FilesSorted = std::set<path>;
using FilesOrdered = std::vector<path>;
using Files = std::unordered_set<path>;
using FilesMap = std::unordered_map<path, path>;

static void set_uppercase_disk(std::string &s)
{
    if (s.size() >= 2 && s[1] == ':')
        s[0] = toupper(s[0]);
}

static void set_uppercase_disk(std::u8string &s)
{
    if (s.size() >= 2 && s[1] == ':')
        s[0] = toupper(s[0]);
}

static void set_uppercase_disk(std::wstring &s)
{
    if (s.size() >= 2 && s[1] == L':')
        s[0] = towupper(s[0]);
}

/// change all '\\' into '/', make drive letter uppercase
inline path normalize_path(const path &p)
{
    if (p.empty())
        return {};
#ifdef _WIN32
    // win32 has internal wchar_t path, so it is faster than u8string conversion
    auto s = p.wstring();
#else
    auto s = p.u8string();
#endif
    normalize_string(s);
    set_uppercase_disk(s);
    return s;
}

/// change all '/' into '\\', make drive letter uppercase
inline path normalize_path_windows(const path &p)
{
    if (p.empty())
        return {};
    auto s = p.u8string();
    normalize_string_windows(s);
    set_uppercase_disk(s);
    return s;
}

/// returns utf-8 string
inline path_u8string to_path_string(const path &p)
{
#if PRIMITIVES_FS_USE_UTF8_PATH_STRINGS
    return p.u8string();
#else
    return p.string();
#endif
}

/// returns utf-8 string in std::string
inline String to_printable_string(const path &p)
{
#if PRIMITIVES_FS_USE_UTF8_PATH_STRINGS
    return to_string(p.u8string());
#else
    return p.string();
#endif
}

namespace primitives::filesystem
{

inline void remove_file(const path &p)
{
    error_code ec;
    fs::remove(p, ec);
    if (ec)
        std::cerr << "Cannot remove file: " << to_printable_string(p) << "\n";
}

inline path canonical(const path &p)
{
#ifdef _WIN32
    try
    {
        return fs::canonical(p);
    }
    catch (const std::system_error &e)
    {
        if (e.code().value() != 1)
            throw;
        // WINDOWS BUG: ram disk
    }
    auto r = fs::absolute(p);
    r = r.lexically_normal();
    return normalize_path_windows(r);
#else
    return fs::canonical(p);
#endif
}


inline FILE *fopen(const path &p, const char *mode = "rb")
{
#ifdef _WIN32
    auto s = p.wstring();
    if (s.size() >= 255)
    {
        s = L"\\\\?\\" + s;
        boost::replace_all(s, L"/", L"\\");
    }
    FILE *f;
    auto err = _wfopen_s(&f, s.c_str(), path(mode).wstring().c_str());
    if (err)
        return nullptr;
    return f;
#else
    return ::fopen((const char *)p.u8string().c_str(), mode);
#endif
}

static String errno2str(auto e)
{
    char buf[1024] = {0};
#if defined(__MINGW32__)
    strerror_s(buf, sizeof(buf), e);
    return String(buf);
#elif defined(_WIN32)
    strerror_s(buf, e);
    return String(buf);
#else
    auto x = strerror_r(e, buf, sizeof(buf));
    if constexpr (std::same_as<decltype(x), int>) {
        return String(buf);
    } else {
        return String(x);
    }
#endif
}

static String errno2str()
{
    return errno2str(errno);
}

inline FILE *fopen_checked(const path &p, const char *mode = "rb")
{
    auto f = fopen(p, mode);
    if (f)
        return f;
    if (strchr(mode, 'w') || strchr(mode, 'a'))
    {
        fs::create_directories(p.parent_path());
        f = fopen(p, mode);
        if (f)
            return f;
    }
    throw SW_RUNTIME_ERROR("Cannot open file: " + to_printable_string(p) + ", mode = " + mode + ", errno = " + std::to_string(errno) + ": " + errno2str());
}

inline void create(const path &p)
{
    // probably this is fixed in MS STL
    /*auto s = p.parent_path().u8string();
#ifdef _WIN32
    if (s.size() >= 255)
    {
        s = "\\\\?\\" + s;
        boost::replace_all(s, "/", "\\");
    }
#endif
    fs::create_directories(s);*/
    fs::create_directories(p.parent_path());
    fclose(fopen(p, "wb"));
}

} // namespace primitives::filesystem

// was __GLIBCXX__ < 20220421 // __GLIBCXX__ < 11.3 but seems it was fedora patches
#if 0 \
    || defined(_MSC_VER) && _MSC_VER < 1932 \
    || defined(__APPLE__) && !defined(__GLIBCXX__) && defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 170000 \
    || defined(__GLIBCXX__) && __GLIBCXX__ <= 20220421 // __GLIBCXX__ <= 11.3
namespace std
{
    template<> struct hash<path>
    {
        size_t operator()(const path& p) const
        {
            return fs::hash_value(p);
        }
    };
}
#endif

class ScopedFile
{
public:
    ScopedFile(const path &p, const char *mode = "rb")
    {
        f = primitives::filesystem::fopen_checked(p, mode);
    }
    ScopedFile(const ScopedFile &) = delete;
    ScopedFile &operator=(const ScopedFile &) = delete;
    ScopedFile(ScopedFile &&rhs)
    {
        f = rhs.f;
        rhs.f = nullptr;
    }
    ScopedFile &operator=(ScopedFile &&) = delete;
    ~ScopedFile()
    {
        close();
    }

    FILE *getHandle() const { return f; }

    size_t read(void *buf, size_t sz)
    {
        return fread(buf, 1, sz, f);
    }
    void seek(uintmax_t offset)
    {
    #ifdef _WIN32
        _fseeki64(f, offset, SEEK_SET);
    #else
        fseek(f, offset, SEEK_SET);
    #endif
    }
    void close()
    {
        if (f)
        {
            if (fclose(f) != 0)
                std::cerr << "Cannot close file, errno = " << std::to_string(errno) << ": " << primitives::filesystem::errno2str() << std::endl;
            f = nullptr;
        }
    }

private:
    FILE *f = nullptr;
};

inline path get_home_directory()
{
#ifdef _WIN32

    wchar_t *w;
    size_t len;
    auto err = _wdupenv_s(&w, &len, L"USERPROFILE");
    if (err)
    {
        std::cerr << "Cannot get user's home directory (%USERPROFILE%)\n";
        return "";
    }
    path home = w;
    free(w);
#else
    // prefer this way
    auto p = getpwuid(getuid());
    if (p)
        return p->pw_dir;
    auto home = getenv("HOME");
    if (!home)
        std::cerr << "Cannot get user's home directory ($HOME)\n";
    if (home == nullptr)
        return "";
#endif
    return home;
}

inline path current_thread_path(const path &p = path())
{
    thread_local path thread_working_dir = p.empty() ? fs::current_path() : primitives::filesystem::canonical(p);
    if (p.empty())
        return thread_working_dir;
    return thread_working_dir = primitives::filesystem::canonical(p);
}

inline String read_file(const path &p, uintmax_t offset = 0, uintmax_t count = UINTMAX_MAX)
{
    ScopedFile ifile(p, "rb");

    if (count == UINTMAX_MAX)
        count = fs::file_size(p) - offset;

    String f;
    f.resize(count);
    ifile.seek(offset);
    ifile.read(&f[0], count);
    return f;
}

inline String read_file_without_bom(const path &p, uintmax_t offset_after_bom = 0, uintmax_t count = UINTMAX_MAX)
{
    static const std::vector<std::vector<uint8_t>> boms
    {
        // UTF-8
        { 0xEF, 0xBB, 0xBF, },

        // UTF-16 BE, LE
        { 0xFE, 0xFF, },
        { 0xFF, 0xFE, },

        // UTF-32 BE, LE
        { 0x00, 0x00, 0xFE, 0xFF, },
        { 0xFF, 0xFE, 0x00, 0x00, },

        // UTF-7
        { 0x2B, 0x2F, 0x76, 0x38 },
        { 0x2B, 0x2F, 0x76, 0x39 },
        { 0x2B, 0x2F, 0x76, 0x2B },
        { 0x2B, 0x2F, 0x76, 0x2F },
        { 0x2B, 0x2F, 0x76, 0x38, 0x2D },

        // UTF-1
        { 0xF7, 0x64, 0x4C, },

        // UTF-EBCDIC
        { 0xDD, 0x73, 0x66, 0x73 },

        // SCSU
        { 0x0E, 0xFE, 0xFF, },

        // BOCU-1
        { 0xFB, 0xEE, 0x28, },

        // GB-18030
        { 0x84, 0x31, 0x95, 0x33 },
    };

    static const auto max_len = std::max_element(boms.begin(), boms.end(),
        [](const auto &e1, const auto &e2) { return e1.size() < e2.size(); })->size();

    // read max bom length
    auto s = read_file(p, 0, max_len);

    for (auto &b : boms)
    {
        if (s.size() >= b.size() && memcmp(s.data(), b.data(), b.size()) == 0)
            return read_file(p, b.size() + offset_after_bom, count);
    }

    // no bom detected
    return read_file(p, offset_after_bom, count);
}


static void write_file1(const path &p, const void *v, size_t sz, const char *mode)
{
    auto pp = p.parent_path();
    if (!pp.empty())
        fs::create_directories(pp);

    ScopedFile f(p, mode);
    fwrite(v, sz, 1, f.getHandle());
}

inline void write_file(const path &p, const String &s)
{
    write_file1(p, s.data(), s.size(), "wb");
}

inline void write_file(const path &p, const std::vector<uint8_t> &s)
{
    write_file1(p, s.data(), s.size(), "wb");
}

inline void write_file(const path &p, const std::span<uint8_t> &s) {
    auto pp = p.parent_path();
    if (!pp.empty())
        fs::create_directories(pp);

    ScopedFile f(p, "wb");
    fwrite(s.data(), s.size(), 1, f.getHandle());
}

inline bool write_file_if_different(const path &p, const String &s)
{
    if (fs::exists(p))
    {
        auto s2 = read_file(p);
        if (s == s2)
            return false;
    }
    write_file(p, s);
    return true;
}

inline void write_file_if_not_exists(const path &p, const String &s)
{
    if (!fs::exists(p))
        write_file(p, s);
}

inline void prepend_file(const path &p, const String &s)
{
    auto s2 = read_file(p);
    write_file(p, s + s2);
}

inline void append_file(const path &p, const String &s)
{
    write_file1(p, s.data(), s.size(), "ab");
}

inline Strings read_lines(const path &p) {
    auto s = read_file(p);
    return split_lines(s);
}

inline void write_lines(const path &p, auto &&lines)
{
    std::string t;
    for (auto &&s : lines) {
        t += s + "\n"s;
    }
    write_file(p, t);
}

inline void remove_all_from_dir(const path &dir)
{
    for (auto &f : fs::directory_iterator(dir))
        fs::remove_all(f);
}

// checks for real file
inline bool is_under_root(path existing_path, const path &root_dir)
{
    // Converts p, which must exist, to an absolute path
    // that has no symbolic link, dot, or dot-dot elements.
    existing_path = primitives::filesystem::canonical(existing_path);
    while (!existing_path.empty() && existing_path != existing_path.root_path())
    {
        error_code ec;
        if (fs::equivalent(existing_path, root_dir, ec))
            return true;
        existing_path = existing_path.parent_path();
    }
    return false;
}

// checks path that may not exist
inline bool is_under_root_by_prefix_path(const path &p, const path &root_dir)
{
    auto r = to_printable_string(normalize_path(root_dir));
    auto v = to_printable_string(normalize_path(p));
    // disallow .. elements
    return v.find(r) == 0 && v.find("..") == v.npos;
}

inline void copy_dir(const path &source, const path &destination)
{
    error_code ec;
    fs::create_directories(destination, ec);
    for (auto &f : fs::directory_iterator(source))
    {
        if (fs::is_directory(f))
            copy_dir(f, destination / f.path().filename());
        else
            copy_file(f, destination / f.path().filename(), fs::copy_options::overwrite_existing);
    }
}

inline void remove_files(const Files &files)
{
    for (auto &f : files)
        fs::remove(f);
}

inline Files enumerate_files(const path &dir, bool recursive = true)
{
    Files files;
    if (!fs::exists(dir))
        return files;
    if (recursive)
    {
        for (auto &f : fs::recursive_directory_iterator(dir))
        {
            if (!fs::is_regular_file(f))
                continue;
            files.insert(f);
        }
    }
    else
    {
        for (auto &f : fs::directory_iterator(dir))
        {
            if (!fs::is_regular_file(f))
                continue;
            files.insert(f);
        }
    }
    return files;
}

inline Files filter_files_like(const Files &files, const String &regex)
{
    Files fls;
    std::regex r(regex);
    for (auto &f : files)
    {
        if (std::regex_match(to_printable_string(f.filename()), r))
            fls.insert(f);
    }
    return fls;
}

inline void remove_files_like(const Files &files, const String &regex)
{
    remove_files(filter_files_like(files, regex));
}

inline void remove_files_like(const path &dir, const String &regex, bool recursive = true)
{
    remove_files_like(enumerate_files(dir, recursive), regex);
}

inline Files enumerate_files_like(const path &dir, const String &regex, bool recursive = true)
{
    return filter_files_like(enumerate_files(dir, recursive), regex);
}

inline Files enumerate_files_like(const Files &files, const String &regex)
{
    return filter_files_like(files, regex);
}

inline path unique_path(const path &p = "%%%%-%%%%-%%%%-%%%%")
{
    return boost::filesystem::unique_path(p.wstring()).wstring();
}

inline time_t file_time_type2time_t(fs::file_time_type t)
{
    // remove defs when using C++20
#if defined(_MSVC_STL_VERSION) && !defined(__cpp_lib_char8_t) || defined(__GNUC__) || !defined(__APPLE__)
    return *(time_t *)&t; // msvc bug
#else
    return fs::file_time_type::clock::to_time_t(t); // available only on apple right now
#endif
}

inline fs::file_time_type time_t2file_time_type(time_t t)
{
    // remove defs when using C++20
#if defined(_MSVC_STL_VERSION) && !defined(__cpp_lib_char8_t) || defined(__GNUC__) || !defined(__APPLE__)
    fs::file_time_type ft; // msvc bug
    *(time_t*)&ft = t;
    return ft;
#else
    return fs::file_time_type::clock::from_time_t(t); // available only on apple right now
#endif
}

enum class CurrentPathScope
{
    Process     = 0x1,
    Thread      = 0x2,

    Max,
    All = Process | Thread,
};
ENABLE_ENUM_CLASS_BITMASK(CurrentPathScope);

class ScopedCurrentPath
{
public:
    ScopedCurrentPath(CurrentPathScope scope = CurrentPathScope::Process)
        : ctp(scope)
    {
        if (bitmask_includes_all(ctp, CurrentPathScope::Thread))
            old[(int)CurrentPathScope::Thread] = current_thread_path();
        if (bitmask_includes_all(ctp, CurrentPathScope::Process))
            old[(int)CurrentPathScope::Process] = fs::current_path();

        set_cwd();
    }
    ScopedCurrentPath(const path &p, CurrentPathScope scope = CurrentPathScope::Process)
        : ScopedCurrentPath(scope)
    {
        if (!p.empty())
        {
            if (bitmask_includes_all(ctp, CurrentPathScope::Thread))
                current_thread_path(p);
            if (bitmask_includes_all(ctp, CurrentPathScope::Process))
                fs::current_path(p);

            set_cwd();
        }
    }
    ~ScopedCurrentPath()
    {
        return_back();
    }

    void return_back()
    {
        if (!active)
            return;

        if (bitmask_includes_all(ctp, CurrentPathScope::Thread))
            current_thread_path(cwd = old[(int)CurrentPathScope::Thread]);
        if (bitmask_includes_all(ctp, CurrentPathScope::Process))
            fs::current_path(cwd = old[(int)CurrentPathScope::Process]);

        active = false;
    }

    path get_cwd() const
    {
        return cwd;
    }
    path get_old(CurrentPathScope scope = CurrentPathScope::Process) const
    {
        return old[(int)scope];
    }


private:
    path old[(int)CurrentPathScope::Max];
    path cwd;
    bool active = true;
    CurrentPathScope ctp = CurrentPathScope::Process;

    void set_cwd()
    {
        // abs path, not possibly relative p
        if (bitmask_includes_all(ctp, CurrentPathScope::Thread))
            cwd = current_thread_path();
        if (bitmask_includes_all(ctp, CurrentPathScope::Process))
            cwd = fs::current_path();
    }
};
