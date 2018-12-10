// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/filesystem.h>

#include <primitives/exceptions.h>
#include <primitives/file_iterator.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <regex>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

#ifndef _WIN32
#define _GNU_SOURCE // for dladdr
#include <dlfcn.h>
#endif

path get_home_directory()
{
#ifdef _WIN32
    auto home = _wgetenv(L"USERPROFILE");
    if (!home)
        std::cerr << "Cannot get user's home directory (%USERPROFILE%)\n";
#else
    // prefer this way
    auto p = getpwuid(getuid());
    if (p)
        return p->pw_dir;
    auto home = getenv("HOME");
    if (!home)
        std::cerr << "Cannot get user's home directory ($HOME)\n";
#endif
    if (home == nullptr)
        return "";
    return home;
}

path current_thread_path(const path &p)
{
    thread_local path thread_working_dir = p.empty() ? fs::current_path() : fs::canonical(fs::absolute(p));
    if (p.empty())
        return thread_working_dir;
    return thread_working_dir = fs::canonical(fs::absolute(p));
}

void remove_file(const path &p)
{
    error_code ec;
    fs::remove(p, ec);
    if (ec)
        std::cerr << "Cannot remove file: " << p.u8string() << "\n";
}

void remove_all_from_dir(const path &dir)
{
    for (auto &f : fs::directory_iterator(dir))
        fs::remove_all(f);
}

String normalize_path(const path &p)
{
    if (p.empty())
        return "";
    auto s = p.u8string();
    normalize_string(s);
    return s;
}

String normalize_path_windows(const path &p)
{
    if (p.empty())
        return "";
    auto s = p.u8string();
    normalize_string_windows(s);
    return s;
}

std::wstring wnormalize_path(const path &p)
{
    if (p.empty())
        return L"";
    auto s = p.wstring();
    normalize_string(s);
    return s;
}

std::wstring wnormalize_path_windows(const path &p)
{
    if (p.empty())
        return L"";
    auto s = p.wstring();
    normalize_string_windows(s);
    return s;
}

String read_file_bytes_unchecked(const path &p, uintmax_t offset, uintmax_t count)
{
    ScopedFile ifile(p, "rb");

    String f;
    f.resize(count);
    ifile.seek(offset);
    ifile.read(&f[0], count);
    return f;
}

String read_file_bytes(const path &p, uintmax_t offset, uintmax_t count)
{
    error_code ec;
    if (!fs::exists(p, ec))
        throw SW_RUNTIME_EXCEPTION("File '" + p.u8string() + "' does not exist");
    return read_file_bytes_unchecked(p, offset, count);
}

String read_file_from_offset(const path &p, uintmax_t offset, uintmax_t max_size)
{
    error_code ec;
    if (!fs::exists(p, ec))
        throw SW_RUNTIME_EXCEPTION("File '" + p.u8string() + "' does not exist");

    auto sz = fs::file_size(p) - offset;
    if (sz > max_size)
        throw SW_RUNTIME_EXCEPTION("File " + p.u8string() + " is bigger than allowed limit (" + std::to_string(max_size) + " bytes)");

    return read_file_bytes_unchecked(p, offset, sz);
}

String read_file(const path &p, uintmax_t max_size)
{
    return read_file_from_offset(p, 0, max_size);
}

String read_file_without_bom(const path &p, uintmax_t max_size)
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
    auto s = read_file_bytes(p, 0, max_len);

    for (auto &b : boms)
    {
        if (s.size() >= b.size() && memcmp(s.data(), b.data(), b.size()) == 0)
            return read_file_from_offset(p, b.size(), max_size);
    }

    return read_file(p, max_size);
}

Strings read_lines(const path &p)
{
    auto s = read_file(p);
    return split_lines(s);
}

static void write_file1(const path &p, const String &s, const char *mode)
{
    auto pp = p.parent_path();
    if (!pp.empty())
        fs::create_directories(pp);

    ScopedFile f(p, mode);
    fwrite(s.c_str(), s.size(), 1, f.getHandle());
}

void write_file(const path &p, const String &s)
{
    write_file1(p, s, "wb");
}

void write_file_if_different(const path &p, const String &s)
{
    error_code ec;
    if (fs::exists(p, ec))
    {
        auto s2 = read_file(p);
        if (s == s2)
            return;
    }
    write_file(p, s);
}

void prepend_file(const path &p, const String &s)
{
    auto s2 = read_file(p);
    write_file(p, s + s2);
}

void append_file(const path &p, const String &s)
{
    write_file1(p, s, "ab");
}

void copy_dir(const path &src, const path &dst)
{
    error_code ec;
    fs::create_directories(dst, ec);
    for (auto &f : fs::directory_iterator(src))
    {
        if (fs::is_directory(f))
            copy_dir(f, dst / f.path().filename());
        else
            copy_file(f, dst / f.path().filename(), fs::copy_options::overwrite_existing);
    }
}

void remove_files(const Files &files)
{
    for (auto &f : files)
        fs::remove(f);
}

void remove_files_like(const Files &files, const String &regex)
{
    remove_files(filter_files_like(files, regex));
}

void remove_files_like(const path &dir, const String &regex, bool recursive)
{
    remove_files_like(enumerate_files(dir, recursive), regex);
}

Files enumerate_files(const path &dir, bool recursive)
{
    Files files;
    error_code ec;
    if (!fs::exists(dir, ec))
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

Files enumerate_files_like(const path &dir, const String &regex, bool recursive)
{
    return filter_files_like(enumerate_files(dir, recursive), regex);
}

Files enumerate_files_like(const Files &files, const String &regex)
{
    return filter_files_like(files, regex);
}

Files filter_files_like(const Files &files, const String &regex)
{
    Files fls;
    std::regex r(regex);
    for (auto &f : files)
    {
        if (!std::regex_match(f.filename().u8string(), r))
            continue;
        fls.insert(f);
    }
    return fls;
}

bool is_under_root(path p, const path &root_dir)
{
    if (p.empty())
        return false;
    error_code ec;
    if (fs::exists(p, ec))
        // Converts p, which must exist, to an absolute path
        // that has no symbolic link, dot, or dot-dot elements.
        p = fs::canonical(p);
    while (!p.empty() && p != p.root_path())
    {
        if (fs::equivalent(p, root_dir, ec))
            return true;
        p = p.parent_path();
    }
    return false;
}

bool compare_files(const path &fn1, const path &fn2)
{
    FileIterator fi({ fn1, fn2 });
    return fi.iterate([](const auto &bufs, auto sz)
    {
        if (memcmp(bufs[0].get().data(), bufs[1].get().data(), (size_t)sz) != 0)
            return false;
        return true;
    });
}

bool compare_dirs(const path &dir1, const path &dir2)
{
    auto traverse_dir = [](const auto &dir)
    {
        std::vector<path> files;
        error_code ec;
        if (fs::exists(dir, ec))
        {
            for (auto &f : fs::recursive_directory_iterator(dir))
            {
                if (!fs::is_regular_file(f))
                    continue;
                files.push_back(f);
            }
        }
        return files;
    };

    auto files1 = traverse_dir(dir1);
    auto files2 = traverse_dir(dir2);

    if (files1.empty())
        return false; // throw SW_RUNTIME_EXCEPTION("left side has no files");
    if (files2.empty())
        return false; // throw SW_RUNTIME_EXCEPTION("right side has no files");
    if (files1.size() != files2.size())
        return false; // throw SW_RUNTIME_EXCEPTION("different number of files");

    auto sz = files1.size();
    for (size_t i = 0; i < sz; i++)
    {
        if (!compare_files(files1[i], files2[i]))
            return false;
    }

    return true;
}

FileIterator::FileIterator(const FilesOrdered &fns)
{
    if (fns.empty())
        throw SW_RUNTIME_EXCEPTION("Provide at least one file");

    for (auto &f : fns)
    {
        File d;
        d.size = fs::file_size(f);
        d.ifile = std::make_unique<ScopedFile>(f, "rb");
        files.emplace_back(std::move(d));
    }
}

bool FileIterator::is_same_size() const
{
    auto sz = files.front().size;
    return std::all_of(files.begin(), files.end(),
        [sz](const auto &f) { return f.size == sz; });
}

bool FileIterator::is_same_read_size() const
{
    auto sz = files.front().read;
    return std::all_of(files.begin(), files.end(),
        [sz](const auto &f) { return f.read == sz; });
}

bool FileIterator::iterate(std::function<bool(const BuffersRef &, uint64_t)> f)
{
    if (!is_same_size())
        return false;

    BuffersRef buffers;
    for (auto &f : files)
    {
        f.buf.resize(buffer_size);
        buffers.emplace_back(f.buf);
    }

    while (std::none_of(files.begin(), files.end(),
        [](const auto &f) { return feof(f.ifile->getHandle()); }))
    {
        std::for_each(files.begin(), files.end(), [this](auto &f)
        {
            f.read = f.ifile->read((char *)&f.buf[0], buffer_size);
        });
        if (!is_same_read_size())
            return false;

        if (!f(buffers, files.front().read))
            return false;
    }
    return true;
}

path unique_path(const path &p)
{
    return boost::filesystem::unique_path(p.u8string()).wstring();
}

namespace primitives::filesystem
{

FILE *fopen(const path &p, const char *mode)
{
#ifdef _WIN32
    auto s = p.wstring();
    if (s.size() >= 255)
    {
        s = L"\\\\?\\" + s;
        boost::replace_all(s, L"/", L"\\");
    }
    return _wfopen(s.c_str(), path(mode).wstring().c_str());
#else
    return ::fopen(p.u8string().c_str(), mode);
#endif
}

FILE *fopen_checked(const path &p, const char *mode)
{
    auto f = fopen(p, mode);
    if (!f)
        throw SW_RUNTIME_EXCEPTION("Cannot open file: " + p.u8string() + ", mode = " + mode + ", errno = " + std::to_string(errno));
    return f;
}

void create(const path &p)
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

}

ScopedFile::ScopedFile(const path &p, const char *mode)
{
    f = primitives::filesystem::fopen_checked(p, mode);
}

ScopedFile::~ScopedFile()
{
    if (f)
        fclose(f);
}

size_t ScopedFile::read(void *buf, size_t sz)
{
    return fread(buf, 1, sz, f);
}

void ScopedFile::seek(uintmax_t offset)
{
    fseek(f, offset, SEEK_SET);
}
