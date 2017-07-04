#pragma once

#include <primitives/string.h>

#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <boost/range.hpp>

#include <unordered_set>

namespace fs = boost::filesystem;
using path = fs::path;

using FilesSorted = std::set<path>;
using Files = std::unordered_set<path>;

path get_home_directory();

String read_file(const path &p, bool no_size_check = false);
void write_file(const path &p, const String &s);
void write_file_if_different(const path &p, const String &s);
Strings read_lines(const path &p);

void remove_file(const path &p);
void remove_all_from_dir(const path &dir);

String normalize_path(const path &p);
bool is_under_root(path p, const path &root_dir);

void copy_dir(const path &source, const path &destination);
void remove_files_like(const path &dir, const String &regex, bool recursive = true);
void remove_files_like(const Files &files, const String &regex);

Files enumerate_files(const path &dir, bool recursive = true);

bool compare_files(const path &fn1, const path &fn2);
bool compare_dirs(const path &dir1, const path &dir2);

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
        old = fs::current_path();
        cwd = old;
    }
    ScopedCurrentPath(const path &p)
        : ScopedCurrentPath()
    {
        if (!p.empty())
        {
            fs::current_path(p);
            // abs path, not possibly relative p
            cwd = fs::current_path();
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
        fs::current_path(old);
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

struct FileIterator
{
    using Buffer = std::vector<uint8_t>;
    using BuffersRef = std::vector<std::reference_wrapper<Buffer>>;

    struct File
    {
        path fn;
        std::unique_ptr<class std::ifstream> ifile;
        uint64_t size;
        Buffer buf;
        uint64_t read = 0;
    };

    std::vector<File> files;
    int buffer_size = 8192;

    FileIterator(const std::vector<path> &fns);

    bool iterate(std::function<bool(const BuffersRef &, uint64_t)> f);
    bool is_same_size() const;
    bool is_same_read_size() const;
};
