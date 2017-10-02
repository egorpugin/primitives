#include <primitives/filesystem.h>

#include <primitives/file_iterator.h>

#include <boost/algorithm/string.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/integration/filesystem.hpp>
#include <boost/nowide/convert.hpp>

#include <iostream>
#include <regex>

#ifdef _WIN32
static auto in_mode = 0x01;
static auto out_mode = 0x02;
static auto app_mode = 0x08;
static auto binary_mode = 0x20;
#else
static auto in_mode = std::ios::in;
static auto out_mode = std::ios::out;
static auto app_mode = std::ios::app;
static auto binary_mode = std::ios::binary;
#endif

path get_home_directory()
{
#ifdef WIN32
    auto home = getenv("USERPROFILE");
    if (!home)
        std::cerr << "Cannot get user's home directory (%USERPROFILE%)\n";
#else
    auto home = getenv("HOME");
    if (!home)
        std::cerr << "Cannot get user's home directory ($HOME)\n";
#endif
    if (home == nullptr)
        return "";
    return home;
}

void remove_file(const path &p)
{
    boost::system::error_code ec;
    fs::remove(p, ec);
    if (ec)
        std::cerr << "Cannot remove file: " << p.string() << "\n";
}

void remove_all_from_dir(const path &dir)
{
    for (auto &f : boost::make_iterator_range(fs::directory_iterator(dir), {}))
        fs::remove_all(f);
}

String normalize_path(const path &p)
{
    if (p.empty())
        return "";
    String s = p.string();
    normalize_string(s);
    return s;
}

String read_file(const path &p, bool no_size_check)
{
    if (!fs::exists(p))
        throw std::runtime_error("File '" + p.string() + "' does not exist");

    auto fn = p.string();
    boost::nowide::ifstream ifile(fn, in_mode | binary_mode);
    if (!ifile)
        throw std::runtime_error("Cannot open file '" + fn + "' for reading");

    size_t sz = (size_t)fs::file_size(p);
    if (!no_size_check && sz > 10'000'000)
        throw std::runtime_error("File " + fn + " is very big (> ~10 MB)");

    String f;
    f.resize(sz);
    ifile.read(&f[0], sz);
    return f;
}

Strings read_lines(const path &p)
{
    auto s = read_file(p);
    return split_lines(s);
}

void write_file(const path &p, const String &s)
{
    auto pp = p.parent_path();
    if (!pp.empty())
        fs::create_directories(pp);

#ifdef _MSC_VER
    auto f = _wfopen(p.wstring().c_str(), L"wb");
    if (!f)
        throw std::runtime_error("Cannot open file '" + p.string() + "' for writing");
    fwrite(s.c_str(), s.size(), 1, f);
    fclose(f);
#else
    boost::nowide::ofstream ofile(p.string(), out_mode | binary_mode);
    if (!ofile)
        throw std::runtime_error("Cannot open file '" + p.string() + "' for writing");
    ofile << s;
#endif
}

void write_file_if_different(const path &p, const String &s)
{
    if (fs::exists(p))
    {
        auto s2 = read_file(p, true);
        if (s == s2)
            return;
    }
    write_file(p, s);
}

void copy_dir(const path &src, const path &dst)
{
    boost::system::error_code ec;
    fs::create_directories(dst, ec);
    for (auto &f : boost::make_iterator_range(fs::directory_iterator(src), {}))
    {
        if (fs::is_directory(f))
            copy_dir(f, dst / f.path().filename());
        else
            fs::copy_file(f, dst / f.path().filename(), fs::copy_option::overwrite_if_exists);
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
    if (!fs::exists(dir))
        return files;
    if (recursive)
    {
        for (auto &f : boost::make_iterator_range(fs::recursive_directory_iterator(dir), {}))
        {
            if (!fs::is_regular_file(f))
                continue;
            files.insert(f);
        }
    }
    else
    {
        for (auto &f : boost::make_iterator_range(fs::directory_iterator(dir), {}))
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
        if (!std::regex_match(f.filename().string(), r))
            continue;
        fls.insert(f);
    }
    return fls;
}

bool is_under_root(path p, const path &root_dir)
{
    if (p.empty())
        return false;
    if (fs::exists(p))
        // Converts p, which must exist, to an absolute path
        // that has no symbolic link, dot, or dot-dot elements.
        p = fs::canonical(p);
    while (!p.empty())
    {
        if (p == root_dir)
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
        if (fs::exists(dir))
            for (auto &f : boost::make_iterator_range(fs::recursive_directory_iterator(dir), {}))
            {
                if (!fs::is_regular_file(f))
                    continue;
                files.push_back(f);
            }
        return files;
    };

    auto files1 = traverse_dir(dir1);
    auto files2 = traverse_dir(dir2);

    if (files1.empty())
        return false; // throw std::runtime_error("left side has no files");
    if (files2.empty())
        return false; // throw std::runtime_error("right side has no files");
    if (files1.size() != files2.size())
        return false; // throw std::runtime_error("different number of files");

    auto sz = files1.size();
    for (size_t i = 0; i < sz; i++)
    {
        if (!compare_files(files1[i], files2[i]))
            return false;
    }

    return true;
}

FileIterator::FileIterator(const std::vector<path> &fns)
{
    if (fns.empty())
        throw std::runtime_error("Provide at least one file");

    for (auto &f : fns)
    {
        File d;
        d.size = fs::file_size(f);
        d.ifile = std::make_unique<boost::nowide::ifstream>(f.string(), in_mode | binary_mode);
        files.push_back(std::move(d));
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
        [](const auto &f) { return f.ifile->eof(); }))
    {
        std::for_each(files.begin(), files.end(), [this](auto &f)
        {
            f.ifile->read((char *)&f.buf[0], buffer_size);
            f.read = f.ifile->gcount();
        });
        if (!is_same_read_size())
            return false;

        if (!f(buffers, files.front().read))
            return false;
    }
    return true;
}

void setup_utf8_filesystem()
{
    boost::nowide::nowide_filesystem();
}
