// Copyright (C) 2016-2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>
#include <primitives/version.h>

#include <nlohmann/json_fwd.hpp>
#include <variant>

namespace YAML { class Node; }
using yaml = YAML::Node;

namespace primitives::source
{

using primitives::version::Version;

enum class SourceType
{
// qt moc fails here
#ifndef Q_MOC_RUN
#define SOURCE(e, s) e,
#include "source.inl"
#undef SOURCE
#endif

    // aliases
    Hg = Mercurial,
    Bzr = Bazaar,
    Empty = EmptySource,
};

struct PRIMITIVES_SOURCE_API Source
{
    using SourceKvMap = std::vector<std::pair<String, String>>;

    virtual ~Source() = default;

    String getHash() const;
    String print() const;
    SourceKvMap printKv() const;

    /// download files to destination directory
    void download(const path &dir) const;

    /// clone
    virtual std::unique_ptr<Source> clone() const = 0;

    // save

    /// save to current object with 'getString()' subobject
    void save(nlohmann::json &j) const;

    /// save to current object with 'getString()' subobject
    void save(yaml &root) const;

    /// save key-value map
    void save(SourceKvMap &m) const;

    // virtual

    ///
    virtual void applyVersion(const Version &v) = 0;

    ///
    virtual SourceType getType() const = 0;

    // static

    /// load from current (passed) object, detects 'getString()' subobject
    static std::unique_ptr<Source> load(const nlohmann::json &j);

    /// load from current (passed) object, detects 'getString()' subobject
    static std::unique_ptr<Source> load(const yaml &root);

protected:
    String getString() const;
    virtual void save1(SourceKvMap &m) const;

private:
    virtual String print1() const = 0;
    virtual void download1(const path &dir) const = 0;
    virtual void save1(nlohmann::json &j) const = 0;
    virtual void save1(yaml &root) const = 0;
    virtual void checkUrl() const = 0;
};

struct EmptySource : Source
{
    EmptySource() {}

    EmptySource(const nlohmann::json &j) {}
    EmptySource(const yaml &root) {}

    void applyVersion(const Version &v) override {}

private:
    SourceType getType() const override { return SourceType::Empty; }
    String print1() const override { return ""; }
    void download1(const path &dir) const override {}
    void save1(nlohmann::json &p) const override {}
    void save1(yaml &root) const override {}
    void checkUrl() const override {}
    std::unique_ptr<Source> clone() const override { return std::make_unique<EmptySource>(*this); }
};

struct PRIMITIVES_SOURCE_API SourceUrl : Source
{
    String url;

    SourceUrl(const String &url);

    SourceUrl(const nlohmann::json &j);
    SourceUrl(const yaml &root);

    void applyVersion(const Version &v) override;

protected:
    String print1() const override;
    void save1(nlohmann::json &p) const override;
    void save1(yaml &root) const override;
    void save1(SourceKvMap &m) const override;

private:
    void checkUrl() const override;
};

struct PRIMITIVES_SOURCE_API Git : SourceUrl
{
    String tag;
    String branch;
    String commit;

    Git(const String &url, const String &tag = "", const String &branch = "", const String &commit = "");

    Git(const nlohmann::json &j);
    Git(const yaml &root);

    void applyVersion(const Version &v) override;
    void tryVTagPrefixDuringDownload(bool try_v_tag_prefix = true);

protected:
    Git(const String &url, const String &tag, const String &branch, const String &commit, bool unchecked);

    Git(const nlohmann::json &j, bool unchecked);
    Git(const yaml &root, bool unchecked);

    void checkValid() const;

    String print1() const override;
    void save1(nlohmann::json &p) const override;
    void save1(yaml &root) const override;
    void save1(SourceKvMap &m) const override;

private:
    bool try_v_tag_prefix = false;

    void download2(const path &dir, const String &tag) const;
    SourceType getType() const override { return SourceType::Git; }
    void download1(const path &dir) const override;
    std::unique_ptr<Source> clone() const override { return std::make_unique<Git>(*this); }
};

struct PRIMITIVES_SOURCE_API Hg : Git
{
    int64_t revision;

    Hg(const String &url, const String &tag = "", const String &branch = "", const String &commit = "", int64_t revision = -1);

    Hg(const nlohmann::json &j);
    Hg(const yaml &root);

private:
    void checkValid() const;
    SourceType getType() const override { return SourceType::Mercurial; }
    String print1() const override;
    void download1(const path &dir) const override;
    void save1(nlohmann::json &p) const override;
    void save1(yaml &root) const override;
    void save1(SourceKvMap &m) const override;
    std::unique_ptr<Source> clone() const override { return std::make_unique<Hg>(*this); }
};

using Mercurial = Hg;

struct PRIMITIVES_SOURCE_API Bzr : SourceUrl
{
    String tag;
    int64_t revision = -1;

    Bzr(const String &url, const String &tag = "", int64_t revision = -1);

    Bzr(const nlohmann::json &j);
    Bzr(const yaml &root);

    void applyVersion(const Version &v) override;

private:
    void checkValid() const;
    SourceType getType() const override { return SourceType::Bazaar; }
    String print1() const override;
    void download1(const path &dir) const override;
    void save1(nlohmann::json &p) const override;
    void save1(yaml &root) const override;
    void save1(SourceKvMap &m) const override;
    std::unique_ptr<Source> clone() const override { return std::make_unique<Bzr>(*this); }
};

using Bazaar = Bzr;

struct PRIMITIVES_SOURCE_API Fossil : Git
{
    using Git::Git;

private:
    SourceType getType() const override { return SourceType::Fossil; }
    void download1(const path &dir) const override;
    std::unique_ptr<Source> clone() const override { return std::make_unique<Fossil>(*this); }
};

struct PRIMITIVES_SOURCE_API Cvs : SourceUrl
{
    String tag;
    String branch;
    String revision;
    String module;

    Cvs(const String &url, const String &module, const String &tag = "", const String &branch = "", const String &revision = "");

    Cvs(const nlohmann::json &j);
    Cvs(const yaml &root);

    void applyVersion(const Version &v) override;

private:
    void checkValid() const;
    SourceType getType() const override { return SourceType::Cvs; }
    void checkUrl() const override;
    String print1() const override;
    void download1(const path &dir) const override;
    void save1(nlohmann::json &p) const override;
    void save1(yaml &root) const override;
    void save1(SourceKvMap &m) const override;
    std::unique_ptr<Source> clone() const override { return std::make_unique<Cvs>(*this); }
};

struct Svn : SourceUrl
{
    String tag;
    String branch;
    int64_t revision = -1;

    Svn(const String &url, const String &tag = "", const String &branch = "", int64_t revision = -1);

    Svn(const nlohmann::json &j);
    Svn(const yaml &root);

    void applyVersion(const Version &v) override;

private:
    void checkValid() const;
    SourceType getType() const override { return SourceType::Svn; }
    String print1() const override;
    void download1(const path &dir) const override;
    void save1(nlohmann::json &p) const override;
    void save1(yaml &root) const override;
    void save1(SourceKvMap &m) const override;
    std::unique_ptr<Source> clone() const override { return std::make_unique<Svn>(*this); }
};

struct PRIMITIVES_SOURCE_API RemoteFile : SourceUrl
{
    using SourceUrl::SourceUrl;

private:
    SourceType getType() const override { return SourceType::RemoteFile; }
    void download1(const path &dir) const override;
    std::unique_ptr<Source> clone() const override { return std::make_unique<RemoteFile>(*this); }
};

struct PRIMITIVES_SOURCE_API RemoteFiles : Source
{
    StringSet urls;

    RemoteFiles(const StringSet &urls);

    RemoteFiles(const nlohmann::json &j);
    RemoteFiles(const yaml &root);

    void applyVersion(const Version &v) override;

private:
    SourceType getType() const override { return SourceType::RemoteFiles; }
    String print1() const override;
    void download1(const path &dir) const override;
    void save1(nlohmann::json &p) const override;
    void save1(yaml &root) const override;
    void save1(SourceKvMap &m) const override;
    void checkUrl() const override;
    std::unique_ptr<Source> clone() const override { return std::make_unique<RemoteFiles>(*this); }
};

} // namespace primitives::source
