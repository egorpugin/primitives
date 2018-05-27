// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <set>
#include <string>
#include <vector>

#include <primitives/stdcompat/optional.h>

namespace primitives::version
{

// extract GenericVersion - GenericNumericVersion?

struct PRIMITIVES_VERSION_API Version
{
    using Number = int64_t;
    using Extra = std::vector<std::string>;

    /// default is min()
    Version();

    /// default is min()
    Version(const Extra &e);

    /// parse from string
    Version(const std::string &v);

    Version(Number ma);
    Version(Number ma, Number mi);
    Version(Number ma, Number mi, Number pa);
    Version(Number ma, Number mi, Number pa, Number tw);

    Version(Number ma, const Extra &e);
    Version(Number ma, Number mi, const Extra &e);
    Version(Number ma, Number mi, Number pa, const Extra &e);
    Version(Number ma, Number mi, Number pa, Number tw, const Extra &e);

    Version(Number ma, const std::string &e);
    Version(Number ma, Number mi, const std::string &e);
    Version(Number ma, Number mi, Number pa, const std::string &e);
    Version(Number ma, Number mi, Number pa, Number tw, const std::string &e);

    // basic access
    Number getMajor() const;
    Number getMinor() const;
    Number getPatch() const;
    Number getTweak() const;
    Extra getExtra() const;
    std::string getBranch() const;

    std::string toString(int level = default_level) const;
    std::string toString(const Version &v) const;

    // modificators
    void decrementVersion();
    void incrementVersion();
    Version getNextVersion() const;
    Version getPreviousVersion() const;

    void decrementVersion(int level);
    void incrementVersion(int level);
    Version getNextVersion(int level) const;
    Version getPreviousVersion(int level) const;

    bool isBranch() const;
    bool isVersion() const;

    bool operator<(const Version &) const;
    bool operator>(const Version &) const;
    bool operator<=(const Version &) const;
    bool operator>=(const Version &) const;
    bool operator==(const Version &) const;
    bool operator!=(const Version &) const;

    // add +,-?
    // int compare()?

public:
    static Version min(int level = default_level);
    static Version min(const Version &v);
    static Version max(int level = default_level);
    static Version max(const Version &v);

    static Number maxNumber();

private:
    Number major = 0;
    Number minor = 0;
    Number patch = 0;
    Number tweak = 0;
    // shows how many numbers are used
    int level = default_level;
    Extra extra;
    std::string branch;

    /// version will be in an invalid state in case of errors
    static bool parse(Version &v, const std::string &s);
    static bool parseExtra(Version &v, const std::string &s);

    std::string printVersion(int level = default_level) const;
    std::string printExtra() const;

    void checkExtra() const;

    static const int default_level = 3;

    friend struct VersionRange; // used in RangePair::toString();
};

struct PRIMITIVES_VERSION_API VersionRange
{
    struct RangePair : std::pair<Version, Version>
    {
        using base = std::pair<Version, Version>;
        using base::base;

        std::string toString() const;
        bool hasVersion(const Version &v) const;
    };
    using Range = std::vector<RangePair>;

    /// default is any version or * or Version::min() - Version::max()
    VersionRange();

    /// parse from string
    VersionRange(const std::string &v);

    std::string toString() const;

    bool empty() const;
    bool hasVersion(const Version &v) const;

    Version getMinSatisfyingVersion(const std::set<Version> &) const;
    Version getMaxSatisfyingVersion(const std::set<Version> &) const;

    VersionRange operator|(const VersionRange &) const;
    VersionRange operator&(const VersionRange &) const;
    VersionRange &operator|=(const VersionRange &);
    VersionRange &operator&=(const VersionRange &);

    static optional<VersionRange> parse(const std::string &s);

private:
    Range range;

    /// range will be in an invalid state in case of errors
    static bool parse(VersionRange &v, const std::string &s);
    static bool parse1(VersionRange &v, const std::string &s);

    VersionRange &operator|=(const RangePair &);
    VersionRange &operator&=(const RangePair &);
};

}
