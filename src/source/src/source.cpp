// Copyright (C) 2016-2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/source.h>

#include <fmt/format.h>
#include <primitives/command.h>
#include <primitives/exceptions.h>
#include <primitives/filesystem.h>
#include <primitives/hash.h>
#include <primitives/http.h>
#include <primitives/overload.h>
#include <primitives/pack.h>
#include <primitives/yaml.h>
#include <nlohmann/json.hpp>

#include <regex>

//#include <primitives/log.h>
//DECLARE_STATIC_LOGGER(logger, "source");

#define JSON_ADD(x) j[#x] = x
#define JSON_ADD_NOT_EMPTY(x) if (!x.empty()) JSON_ADD(x)
#define JSON_ADD_NOT_MINUS_ONE(x) if (x != -1) JSON_ADD(x)

#define JSON_GET_STRING(x) if (j.find(#x) != j.end()) x = j[#x].get<std::string>()
#define JSON_GET_INT(x) if (j.find(#x) != j.end()) x = j[#x].get<int64_t>()

#define STRING_PRINT_VALUE(x, v) r += #x ": " + v + "\n"
#define STRING_PRINT(x) STRING_PRINT_VALUE(#x, x)
#define STRING_PRINT_NOT_EMPTY(x) if (!x.empty()) STRING_PRINT(x)
#define STRING_PRINT_NOT_MINUS_ONE(x) if (x != -1) STRING_PRINT_VALUE(#x, std::to_string(x))

#define YAML_SET(x, n) root[n] = x
#define YAML_SET_NOT_EMPTY(x) if (!x.empty()) YAML_SET(x, #x)
#define YAML_SET_NOT_MINUS_ONE(x) if (x != -1) YAML_SET(x, #x)

#define KV_ADD_IF_NOT_EMPTY(k, v) \
    if (!v.empty())               \
    m.push_back({k, v})

#define KV_ADD_IF_NOT_EMPTY_NUMBER(k, v) \
    if (v != -1)                         \
    m.push_back({k, std::to_string(v)})

namespace primitives::source
{

static bool isValidSourceUrl(const String &url)
{
    if (url.empty())
        return false;
    if (!isUrl(url))
        return false;
    if (url.find_first_of(R"bbb('"`\|;$ @!#^*()<>[],)bbb") != url.npos)
        return false;
    if (url.find("&&") != url.npos)
        return false;
#ifndef CPPAN_TEST
    if (url.find("file:") == 0) // prevent loading local files
        return false;
#endif
    for (auto &c : url)
    {
        if (c < 0 || c > 127)
            return false;
    }
    return true;
}

static void checkSourceUrl(const String &url)
{
    if (!isValidSourceUrl(url))
        throw SW_RUNTIME_ERROR("Bad source url: " + url);
}

static void download_file_checked(const String &url, const path &fn, int64_t max_file_size = 0)
{
    checkSourceUrl(url);
    download_file(url, fn, max_file_size);
}

static void download_and_unpack(const String &url, path fn, const path &unpack_dir, int64_t max_file_size = 0)
{
    if (!fn.is_absolute())
        fn = unpack_dir / fn;
    download_file_checked(url, fn, max_file_size);
    unpack_file(fn, unpack_dir);
    fs::remove(fn);
}

template <typename F>
static void downloadRepository(F &&f)
{
    std::exception_ptr eptr;
    int n_tries = 3;
    while (n_tries--)
    {
        try
        {
            f();
            break;
        }
        catch (...)
        {
            if (!eptr)
                eptr = std::current_exception();
            if (n_tries == 0)
                std::rethrow_exception(eptr);
        }
    }
}

static void execute_command_in_dir(const path &dir, const Strings &args)
{
    Command c;
    c.working_directory = dir;
    c.args = args;
    c.out.inherit = true;
    c.err.inherit = true;
    c.execute();
}

static int isEmpty(int64_t i)
{
    return i == -1;
}

static int isEmpty(const String &s)
{
    return s.empty();
}

static int checkEmpty()
{
    return 0;
}

static int checkNotEmpty()
{
    return 0;
}

template <typename First, typename ... Args>
static int checkEmpty(First &&f, Args && ... args)
{
    return isEmpty(f) + checkEmpty(std::forward<Args>(args)...);
}

template <typename First, typename ... Args>
static int checkNotEmpty(First &&f, Args && ... args)
{
    return !isEmpty(f) + checkNotEmpty(std::forward<Args>(args)...);
}

template <typename ... Args>
static void checkOne(const String &name, Args && ... args)
{
    int n = checkNotEmpty(std::forward<Args>(args)...);
    if (n == 0)
        throw SW_RUNTIME_ERROR("No " + name + " sources available");
    if (n > 1)
        throw SW_RUNTIME_ERROR("Only one " + name + " source must be specified");
}

static String toString(SourceType t)
{
    switch (t)
    {
#define SOURCE(e, s) case SourceType::e: return s;
#include <primitives/source.inl>
#undef SOURCE
    }

    SW_UNREACHABLE;
}

static SourceType fromString(const String &t)
{
    if (0) return {};
#define SOURCE(e, s) else if (t == s) return SourceType::e;
#include <primitives/source.inl>
#undef SOURCE
    else throw SW_RUNTIME_ERROR("Bad source: " + t);
}

String Source::getHash() const
{
    return shorten_hash(blake2b_512(print()), 12);
}

String Source::print() const
{
    return getString() + ":\n" + print1();
}

void Source::download(const path &dir) const
{
    // main entry point and main purpose of all sources

    // so we check url validity here
    // (before this place url may contain tricky format things from fmt lib etc.)
    checkUrl();

    fs::create_directories(dir);
    download1(dir);
}

String Source::getString() const
{
    return toString(getType());
}

void Source::save(nlohmann::json &j) const
{
    save1(j[getString()]);
}

void Source::save(yaml &root) const
{
    auto r = root[getString()];
    save1(r);
}

void Source::save(SourceKvMap &m) const
{
    save1(m);
}

void Source::save1(SourceKvMap &m) const
{
    KV_ADD_IF_NOT_EMPTY("Source", getString());
}

Source::SourceKvMap Source::printKv() const
{
    SourceKvMap m;
    save(m);
    return m;
}

std::unique_ptr<Source> Source::load(const nlohmann::json &j)
{
    if (j.size() != 1)
        throw SW_RUNTIME_ERROR("Bad json source (0 or >1 objects)");
    if (!j.is_object())
        throw SW_RUNTIME_ERROR("Bad yaml source (not an object)");

    auto type_string = j.begin().key();
    auto t = fromString(type_string);

    switch (t)
    {
#define SOURCE(e, s) case SourceType::e: return std::make_unique<e>(j.begin().value());
#include <primitives/source.inl>
#undef SOURCE
    }

    SW_UNREACHABLE;
}

std::unique_ptr<Source> Source::load(const yaml &root)
{
    if (root.size() != 1)
        throw SW_RUNTIME_ERROR("Bad yaml source (0 or >1 objects)");
    if (!root.IsMap())
        throw SW_RUNTIME_ERROR("Bad yaml source (not a map object)");

    auto type_string = root.begin()->first.template as<String>();
    auto t = fromString(type_string);

    switch (t)
    {
#define SOURCE(e, s) case SourceType::e: return std::make_unique<e>(root.begin()->second);
#include <primitives/source.inl>
#undef SOURCE
    }

    SW_UNREACHABLE;
}

SourceUrl::SourceUrl(const String &url)
    : url(url)
{
}

SourceUrl::SourceUrl(const nlohmann::json &j)
{
    JSON_GET_STRING(url);
}

SourceUrl::SourceUrl(const yaml &root)
{
    YAML_EXTRACT_AUTO(url);
}

void SourceUrl::checkUrl() const
{
    checkSourceUrl(url);
}

void SourceUrl::save1(nlohmann::json &j) const
{
    JSON_ADD(url);
}

void SourceUrl::save1(yaml &root) const
{
    YAML_SET_NOT_EMPTY(url);
}

String SourceUrl::print1() const
{
    String r;
    STRING_PRINT(url);
    return r;
}

void SourceUrl::save1(SourceKvMap &m) const
{
    Source::save1(m);
    KV_ADD_IF_NOT_EMPTY("Url", url);
}

void SourceUrl::applyVersion(const Version &v)
{
    v.format(url);
}

Git::Git(const String &url, const String &tag, const String &branch, const String &commit)
    : Git(url, tag, branch, commit, true)
{
    checkValid();
}

Git::Git(const String &url, const String &tag, const String &branch, const String &commit, bool unchecked)
    : SourceUrl(url), tag(tag), branch(branch), commit(commit)
{
}

Git::Git(const nlohmann::json &j)
    : Git(j, true)
{
    checkValid();
}

Git::Git(const nlohmann::json &j, bool unchecked)
    : SourceUrl(j)
{
    JSON_GET_STRING(tag);
    JSON_GET_STRING(branch);
    JSON_GET_STRING(commit);
}

Git::Git(const yaml &root)
    : Git(root, true)
{
    checkValid();
}

Git::Git(const yaml &root, bool unchecked)
    : SourceUrl(root)
{
    YAML_EXTRACT_AUTO(tag);
    YAML_EXTRACT_AUTO(branch);
    YAML_EXTRACT_AUTO(commit);
}

void Git::checkValid() const
{
    checkOne(getString(), tag, branch, commit);
}

void Git::download1(const path &dir) const
{
    // try to speed up git downloads from github
    // add more sites below
    if (url.find("github.com") != url.npos)
    {
        auto github_url = url;

        // remove possible .git suffix
        String suffix = ".git";
        if (github_url.rfind(suffix) == github_url.size() - suffix.size())
            github_url = github_url.substr(0, github_url.size() - suffix.size());

        path fn;
        github_url += "/archive/";
        if (!tag.empty())
        {
            github_url += tag + ".tar.gz";
            fn = dir / "1.tar.gz";
        }
        else if (!branch.empty())
        {
            github_url += branch + ".zip"; // but use .zip for branches!
            fn = dir / "1.zip";
        }
        else if (!commit.empty())
        {
            github_url += commit + ".zip"; // but use .zip for branches!
            fn = dir / "1.zip";
        }

        try
        {
            download_and_unpack(github_url, fn, dir);
            return;
        }
        catch (std::exception &e)
        {
            // go to usual git download1
            //LOG_WARN(logger, e.what());
            if (fs::exists(fn))
                fs::remove(fn);
        }
    }

    // usual git download1 via clone
#ifdef CPPAN_TEST
    if (fs::exists(".git"))
        return;
#endif

    downloadRepository([this, dir]()
    {
        execute_command_in_dir(dir, { "git", "init" });
        execute_command_in_dir(dir, { "git", "remote", "add", "origin", url });
        if (!tag.empty())
        {
            execute_command_in_dir(dir, { "git", "fetch", "--depth", "1", "origin", "refs/tags/" + tag });
            execute_command_in_dir(dir, { "git", "reset", "--hard", "FETCH_HEAD" });
        }
        else if (!branch.empty())
        {
            execute_command_in_dir(dir, { "git", "fetch", "--depth", "1", "origin", branch });
            execute_command_in_dir(dir, { "git", "reset", "--hard", "FETCH_HEAD" });
        }
        else if (!commit.empty())
        {
            execute_command_in_dir(dir, { "git", "fetch" });
            execute_command_in_dir(dir, { "git", "checkout", commit });
        }
    });
}

void Git::save1(nlohmann::json &j) const
{
    SourceUrl::save1(j);
    JSON_ADD_NOT_EMPTY(tag);
    JSON_ADD_NOT_EMPTY(branch);
    JSON_ADD_NOT_EMPTY(commit);
}

void Git::save1(yaml &root) const
{
    SourceUrl::save1(root);
    YAML_SET_NOT_EMPTY(tag);
    YAML_SET_NOT_EMPTY(branch);
    YAML_SET_NOT_EMPTY(commit);
}

String Git::print1() const
{
    auto r = SourceUrl::print1();
    STRING_PRINT_NOT_EMPTY(tag);
    else STRING_PRINT_NOT_EMPTY(branch);
    else STRING_PRINT_NOT_EMPTY(commit);
    return r;
}

void Git::save1(SourceKvMap &m) const
{
    SourceUrl::save1(m);
    KV_ADD_IF_NOT_EMPTY("Tag", tag);
    KV_ADD_IF_NOT_EMPTY("Branch", branch);
    KV_ADD_IF_NOT_EMPTY("Commit", commit);
}

void Git::applyVersion(const Version &v)
{
    SourceUrl::applyVersion(v);
    v.format(tag);
    v.format(branch);
}

Hg::Hg(const String &url, const String &tag, const String &branch, const String &commit, int64_t revision)
    : Git(url, tag, branch, commit, true), revision(revision)
{
    checkValid();
}

Hg::Hg(const nlohmann::json &j)
    : Git(j, true)
{
    JSON_GET_INT(revision);
    checkValid();
}

Hg::Hg(const yaml &root)
    : Git(root, true)
{
    YAML_EXTRACT_AUTO(revision);
    checkValid();
}

void Hg::checkValid() const
{
    checkOne(getString(), tag, branch, commit, revision);
}

void Hg::download1(const path &dir) const
{
    downloadRepository([this, dir]()
    {
        execute_command_in_dir(dir, { "hg", "clone", url });

        if (!tag.empty())
            execute_command_in_dir(dir, { "hg", "update", tag });
        else if (!branch.empty())
            execute_command_in_dir(dir, { "hg", "update", branch });
        else if (!commit.empty())
            execute_command_in_dir(dir, { "hg", "update", commit });
        else if (revision != -1)
            execute_command_in_dir(dir, { "hg", "update", std::to_string(revision) });
    });
}

void Hg::save1(nlohmann::json &j) const
{
    Git::save1(j);
    JSON_ADD_NOT_MINUS_ONE(revision);
}

void Hg::save1(yaml &root) const
{
    Git::save1(root);
    YAML_SET_NOT_MINUS_ONE(revision);
}

String Hg::print1() const
{
    auto r = Git::print1();
    STRING_PRINT_NOT_MINUS_ONE(revision);
    return r;
}

void Hg::save1(SourceKvMap &m) const
{
    Git::save1(m);
    KV_ADD_IF_NOT_EMPTY_NUMBER("Revision", revision);
}

Bzr::Bzr(const String &url, const String &tag, int64_t revision)
    : SourceUrl(url), tag(tag), revision(revision)
{
    checkValid();
}

Bzr::Bzr(const nlohmann::json &j)
    : SourceUrl(j)
{
    JSON_GET_STRING(tag);
    JSON_GET_INT(revision);
    checkValid();
}

Bzr::Bzr(const yaml &root)
    : SourceUrl(root)
{
    YAML_EXTRACT_AUTO(tag);
    YAML_EXTRACT_AUTO(revision);
    checkValid();
}

void Bzr::checkValid() const
{
    checkOne(getString(), tag, revision);
}

void Bzr::download1(const path &dir) const
{
    downloadRepository([this, dir]()
    {
        execute_command_in_dir(dir, { "bzr", "branch", url });

        if (!tag.empty())
            execute_command_in_dir(dir, { "bzr", "update", "-r", "tag:" + tag });
        else if (revision != -1)
            execute_command_in_dir(dir, { "bzr", "update", "-r", std::to_string(revision) });
    });
}

void Bzr::applyVersion(const Version &v)
{
    SourceUrl::applyVersion(v);
    v.format(tag);
}

void Bzr::save1(nlohmann::json &j) const
{
    SourceUrl::save1(j);
    JSON_ADD_NOT_EMPTY(tag);
    JSON_ADD_NOT_MINUS_ONE(revision);
}

void Bzr::save1(yaml &root) const
{
    SourceUrl::save1(root);
    YAML_SET_NOT_EMPTY(tag);
    YAML_SET_NOT_MINUS_ONE(revision);
}

String Bzr::print1() const
{
    auto r = SourceUrl::print1();
    STRING_PRINT_NOT_EMPTY(tag);
    else STRING_PRINT_NOT_MINUS_ONE(revision);
    return r;
}

void Bzr::save1(SourceKvMap &m) const
{
    SourceUrl::save1(m);
    KV_ADD_IF_NOT_EMPTY("Tag", tag);
    KV_ADD_IF_NOT_EMPTY_NUMBER("Revision", revision);
}

void Fossil::download1(const path &dir) const
{
    downloadRepository([this, dir]()
    {
        execute_command_in_dir(dir, { "fossil", "clone", url, "temp.fossil" });
        execute_command_in_dir(dir, { "fossil", "open", "temp.fossil" });

        if (!tag.empty())
            execute_command_in_dir(dir, { "fossil", "update", tag });
        else if (!branch.empty())
            execute_command_in_dir(dir, { "fossil", "update", branch });
        else if (!commit.empty())
            execute_command_in_dir(dir, { "fossil", "update", commit });
    });
}

Cvs::Cvs(const String &url, const String &module, const String &tag, const String &branch, const String &revision)
    : SourceUrl(url), module(module), tag(tag), branch(branch), revision(revision)
{
    checkValid();
}

Cvs::Cvs(const nlohmann::json &j)
    : SourceUrl(j)
{
    JSON_GET_STRING(tag);
    JSON_GET_STRING(branch);
    JSON_GET_STRING(revision);
    JSON_GET_STRING(module);
    checkValid();
}

Cvs::Cvs(const yaml &root)
    : SourceUrl(root)
{
    YAML_EXTRACT_AUTO(tag);
    YAML_EXTRACT_AUTO(branch);
    YAML_EXTRACT_AUTO(revision);
    YAML_EXTRACT_AUTO(module);
    checkValid();
}

void Cvs::checkUrl() const
{
    static const std::regex checkCvs("-d:([a-z0-9_-]+):([a-z0-9_-]+)@(\\S*):(\\S*)");
    if (!std::regex_match(url, checkCvs))
        throw SW_RUNTIME_ERROR("Invalid cvs url: " + url);
}

void Cvs::checkValid() const
{
    if (module.empty())
        throw SW_RUNTIME_ERROR("cvs: empty module");
    checkOne(getString(), tag, branch, revision);
}

void Cvs::download1(const path &dir) const
{
    downloadRepository([this, dir]()
    {
        execute_command_in_dir(dir, { "cvs", url, "co", module });

        if (!tag.empty())
            execute_command_in_dir(dir, { "cvs", "update", "-r", tag });
        else if (!branch.empty())
            execute_command_in_dir(dir, { "cvs", "update", "-r", branch });
        else if (!revision.empty())
            execute_command_in_dir(dir, { "cvs", "update", "-r", revision });
    });
}

void Cvs::applyVersion(const Version &v)
{
    SourceUrl::applyVersion(v);
    //v.format(module); // ?
    v.format(tag);
    v.format(branch);
    v.format(revision); // ?
}

void Cvs::save1(nlohmann::json &j) const
{
    SourceUrl::save1(j);
    JSON_ADD_NOT_EMPTY(tag);
    JSON_ADD_NOT_EMPTY(branch);
    JSON_ADD_NOT_EMPTY(revision);
    JSON_ADD_NOT_EMPTY(module);
}

void Cvs::save1(yaml &root) const
{
    SourceUrl::save1(root);
    YAML_SET_NOT_EMPTY(tag);
    YAML_SET_NOT_EMPTY(branch);
    YAML_SET_NOT_EMPTY(revision);
    YAML_SET_NOT_EMPTY(module);
}

String Cvs::print1() const
{
    auto r = SourceUrl::print1();
    STRING_PRINT_NOT_EMPTY(tag);
    else STRING_PRINT_NOT_EMPTY(branch);
    else STRING_PRINT_NOT_EMPTY(revision);
    STRING_PRINT_NOT_EMPTY(module);
    return r;
}

void Cvs::save1(SourceKvMap &m) const
{
    SourceUrl::save1(m);
    KV_ADD_IF_NOT_EMPTY("Tag", tag);
    KV_ADD_IF_NOT_EMPTY("Branch", branch);
    KV_ADD_IF_NOT_EMPTY("Revision", revision);
    KV_ADD_IF_NOT_EMPTY("Module", module);
}

Svn::Svn(const String &url, const String &tag, const String &branch, int64_t revision)
    : SourceUrl(url), tag(tag), branch(branch), revision(revision)
{
    checkValid();
}

Svn::Svn(const nlohmann::json &j)
    : SourceUrl(j)
{
    JSON_GET_STRING(tag);
    JSON_GET_STRING(branch);
    JSON_GET_INT(revision);
    checkValid();
}

Svn::Svn(const yaml &root)
    : SourceUrl(root)
{
    YAML_EXTRACT_AUTO(tag);
    YAML_EXTRACT_AUTO(branch);
    YAML_EXTRACT_AUTO(revision);
    checkValid();
}

void Svn::checkValid() const
{
    checkOne(getString(), tag, branch, revision);
}

void Svn::download1(const path &dir) const
{
    downloadRepository([this, dir]()
    {
        if (!tag.empty())
            execute_command_in_dir(dir, { "svn", "checkout", url + "/tags/" + tag }); //tag
        else if (!branch.empty())
            execute_command_in_dir(dir, { "svn", "checkout", url + "/branches/" + branch }); //branch
        else if (revision != -1)
            execute_command_in_dir(dir, { "svn", "checkout", "-r", std::to_string(revision), url });
        else
            execute_command_in_dir(dir, { "svn", "checkout", url + "/trunk" });
    });
}

void Svn::applyVersion(const Version &v)
{
    SourceUrl::applyVersion(v);
    v.format(tag);
    v.format(branch);
}

void Svn::save1(nlohmann::json &j) const
{
    SourceUrl::save1(j);
    JSON_ADD_NOT_EMPTY(tag);
    JSON_ADD_NOT_EMPTY(branch);
    JSON_ADD_NOT_MINUS_ONE(revision);
}

void Svn::save1(yaml &root) const
{
    SourceUrl::save1(root);
    YAML_SET_NOT_EMPTY(tag);
    YAML_SET_NOT_EMPTY(branch);
    YAML_SET_NOT_MINUS_ONE(revision);
}

String Svn::print1() const
{
    auto r = SourceUrl::print1();
    STRING_PRINT_NOT_EMPTY(tag);
    else STRING_PRINT_NOT_EMPTY(branch);
    else STRING_PRINT_NOT_MINUS_ONE(revision);
    return r;
}

void Svn::save1(SourceKvMap &m) const
{
    SourceUrl::save1(m);
    KV_ADD_IF_NOT_EMPTY("Tag", tag);
    KV_ADD_IF_NOT_EMPTY("Branch", branch);
    KV_ADD_IF_NOT_EMPTY_NUMBER("Revision", revision);
}

void RemoteFile::download1(const path &dir) const
{
    download_and_unpack(url, dir / path(url).filename(), dir);
}

RemoteFiles::RemoteFiles(const StringSet &urls)
    : urls(urls)
{
}

RemoteFiles::RemoteFiles(const nlohmann::json &j)
{
    Strings s = j["url"];
    urls.insert(s.begin(), s.end());
}

RemoteFiles::RemoteFiles(const yaml &root)
{
    urls = get_sequence_set<String>(root);
}

void RemoteFiles::checkUrl() const
{
    for (auto &url : urls)
        checkSourceUrl(url);
}

void RemoteFiles::download1(const path &dir) const
{
    for (auto &rf : urls)
        download_file_checked(rf, dir / path(rf).filename());
}

void RemoteFiles::save1(nlohmann::json &j) const
{
    for (auto &rf : urls)
        j["url"].push_back(rf);
}

void RemoteFiles::save1(yaml &root) const
{
    for (auto &rf : urls)
        root[getString()].push_back(rf);
}

String RemoteFiles::print1() const
{
    String r;
    for (auto &rf : urls)
        STRING_PRINT_VALUE(url, rf);
    return r;
}

void RemoteFiles::save1(SourceKvMap &m) const
{
    Source::save1(m);
    for (auto &url : urls)
        KV_ADD_IF_NOT_EMPTY("Url", url);
}

void RemoteFiles::applyVersion(const Version &v)
{
    for (auto &u : urls)
        v.format(u);
}

} // namespace primitives::source
