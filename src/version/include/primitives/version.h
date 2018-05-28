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
#include <primitives/stdcompat/variant.h>

namespace primitives::version
{

namespace detail
{

using Number = int64_t;

template <class T, class ... Args>
struct access_vector : std::vector<T, Args...>
{
    using base = std::vector<T, Args...>;

    using base::base;

    const T &operator[](int i) const
    {
        return const_cast<access_vector*>(this)->base::operator[](i);
    }
};

template <class ... Args>
struct comparable_variant : std::variant<Args...>
{
    using base = std::variant<Args...>;

    using base::base;

    bool operator==(Number n) const
    {
        return std::get<Number>(*this) == n;
    }

    bool operator==(const std::string &s) const
    {
        return std::get<std::string>(*this) == s;
    }
};

}

// extract GenericVersion - GenericNumericVersion?

struct PRIMITIVES_VERSION_API Version
{
    using Number = detail::Number;
    // enough numbers, here string goes first
    using Extra = detail::access_vector<detail::comparable_variant<std::string, Number>>;

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
    const Extra &getExtra() const;
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

#ifndef BISON_PARSER
private:
#endif
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

    // used in RangePair::toString();
    friend struct VersionRange;
};

struct PRIMITIVES_VERSION_API VersionRange
{
    struct RangePair : std::pair<Version, Version>
    {
        using base = std::pair<Version, Version>;
        using base::base;

        std::string toString() const;
        bool hasVersion(const Version &v) const;

        optional<RangePair> operator&(const RangePair &) const;
    };
    using Range = std::vector<RangePair>;

    /// default is any version or * or Version::min() - Version::max()
    VersionRange(int level = Version::default_level);

    /// from two versions
    VersionRange(const Version &v1, const Version &v2);

    /// parse from string
    VersionRange(const std::string &v);

    std::string toString() const;

    bool isEmpty() const;
    bool hasVersion(const Version &v) const;

    Version getMinSatisfyingVersion(const std::set<Version> &) const;
    Version getMaxSatisfyingVersion(const std::set<Version> &) const;

    VersionRange operator|(const VersionRange &) const;
    VersionRange operator&(const VersionRange &) const;
    VersionRange &operator|=(const VersionRange &);
    VersionRange &operator&=(const VersionRange &);

    // think like it's private
    // does not work in dll builds when private
    VersionRange &operator|=(const RangePair &);
    VersionRange &operator&=(const RangePair &);

    static optional<VersionRange> parse(const std::string &s);

    static VersionRange empty();

#ifndef BISON_PARSER
private:
#endif
    // always sorted with less
    Range range;

    /// range will be in an invalid state in case of errors
    static std::tuple<bool, std::string> parse(VersionRange &v, const std::string &s);
    static bool parse1(VersionRange &v, const std::string &s);
};

}
