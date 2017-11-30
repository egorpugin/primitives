#pragma once

#include <primitives/string.h>

#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <boost/range.hpp>

#include <unordered_map>
#include <unordered_set>

namespace fs = boost::filesystem;
using path = fs::path;

using FilesSorted = std::set<path>;
using FilesOrdered = std::vector<path>;
using Files = std::unordered_set<path>;
using FilesMap = std::unordered_map<path, path>;

path get_home_directory();
path current_path(const path &p = path()); // thread working directory

String read_file(const path &p, bool no_size_check = false);
void write_file(const path &p, const String &s);
void write_file_if_different(const path &p, const String &s);
Strings read_lines(const path &p);

void remove_file(const path &p);
void remove_all_from_dir(const path &dir);

String normalize_path(const path &p);
bool is_under_root(path p, const path &root_dir);

void copy_dir(const path &source, const path &destination);
void remove_files(const Files &files);
void remove_files_like(const path &dir, const String &regex, bool recursive = true);
void remove_files_like(const Files &files, const String &regex);

Files enumerate_files(const path &dir, bool recursive = true);
Files enumerate_files_like(const path &dir, const String &regex, bool recursive = true);
Files enumerate_files_like(const Files &files, const String &regex);

Files filter_files_like(const Files &files, const String &regex);

bool compare_files(const path &fn1, const path &fn2);
bool compare_dirs(const path &dir1, const path &dir2);

void setup_utf8_filesystem();

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

class ScopedCurrentPath
{
public:
    ScopedCurrentPath()
    {
        old = ::current_path();
        cwd = old;
    }
    ScopedCurrentPath(const path &p)
        : ScopedCurrentPath()
    {
        if (!p.empty())
        {
            ::current_path(p);
            // abs path, not possibly relative p
            cwd = ::current_path();
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
        ::current_path(old);
        cwd = old;
        active = false;
    }

    path get_cwd() const { return cwd; }
    path get_old() const { return old; }

private:
    path old;
    path cwd;
    bool active = true;
};

namespace primitives::filesystem
{

FILE *fopen(const path &p, const char *mode);
void create(const path &p);

}

class ScopedFile
{
public:
    ScopedFile(const path &p, const char *mode);
    ~ScopedFile();

    FILE *getHandle() const { return f; }
    operator bool() const { return f; }

    size_t read(void *buf, size_t sz);

private:
    FILE *f = nullptr;
};
