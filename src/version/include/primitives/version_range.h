// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "version_helpers.h"

namespace primitives::version
{

enum class VersionRangePairStringRepresentationType
{
    // examples provided with with default_level == 3
    // SameDefaultLevel equals to SameRealLevel when default_level >= 3

    // >1.0.0 <2.0.0 becomes >1.0.0 <2.0.0
    // >1.1.0 <2.0.0 becomes >1.1.0 <2.0.0
    // >1.2.3.4 <2.0.0 becomes >1.2.3.4 <2.0.0.0
    SameDefaultLevel        = 0,

    // >1.0.0 <2.0.0 becomes >1 <2
    // >1.1.0 <2.0.0 becomes >1.1 <2.0
    // >1.2.3.4 <2.0.0 becomes >1.2.3.4 <2.0.0.0
    SameRealLevel           = 1,

    // >1 <2         becomes >1 <2
    // >1.1.0 <2.0.0 becomes >1.1 <2
    // >1.2.3.4 <2.0.0 becomes >1.2.3.4 <2
    IndividualRealLevel     = 2,
};

namespace detail
{

/// version interval
struct RangePair
{
    struct Side
    {
        Version v;
        bool strong_relation;

        bool operator==(const Side &rhs) const { return std::tie(v, strong_relation) == std::tie(rhs.v, rhs.strong_relation); }
        bool operator<(const Side &rhs) const { return std::tie(v, strong_relation) < std::tie(rhs.v, rhs.strong_relation); }

        bool operator<(const Version &) const;
        bool operator>(const Version &) const;

        std::string detail::RangePair::toString(VersionRangePairStringRepresentationType t) const;
    };

public:
    RangePair(const Version &, bool strong_relation1, const Version &, bool strong_relation2);

    std::string toString(VersionRangePairStringRepresentationType) const;
    bool contains(const Version &) const;
    size_t getHash() const;
    std::optional<Version> toVersion() const;

    const Version &getFirst() const { return first.v; }
    const Version &getSecond() const { return second.v; }

    bool operator==(const RangePair &rhs) const { return std::tie(first, second) == std::tie(rhs.first, rhs.second); }

    std::optional<RangePair> operator&(const RangePair &) const;
    // returns {} in case of non overlapping pairs
    std::optional<RangePair> operator|(const RangePair &) const;

private:
    Side first;
    Side second;

    RangePair() = default;
    RangePair(const Side &, const Side &);

    std::string toStringInterval() const;
};

}

struct PRIMITIVES_VERSION_API VersionRange
{
    VersionRange();

    /// from one version
    //explicit VersionRange(const Version &);

    /// from two versions [from, to]
    VersionRange(const Version &from, const Version &to);

    /// parse from raw string
    VersionRange(const char *); // no explicit
    /// parse from string
    VersionRange(const std::string &); // no explicit

    /// convert to single version if possible
    /// consists of one range and first == second
    std::optional<Version> toVersion() const;

    // maybe merge two toString()s?

    /// convert to string, default is SameDefaultLevel
    std::string toString() const;

    /// convert to string
    std::string toString(VersionRangePairStringRepresentationType) const;

    size_t getHash() const;

    bool isEmpty() const;
    [[deprecated("use contains()")]]
    bool hasVersion(const Version &v) const { return contains(v); }
    bool contains(const Version &) const;
    bool contains(const VersionRange &) const;
    size_t size() const;

    std::optional<Version> getMinSatisfyingVersion(const std::set<Version> &) const;
    std::optional<Version> getMaxSatisfyingVersion(const std::set<Version> &) const;

    // operators
    bool operator==(const VersionRange &) const;

    // TODO:
    //VersionRange operator~() const;

    VersionRange operator|(const VersionRange &) const;
    VersionRange operator&(const VersionRange &) const;
    VersionRange &operator|=(const VersionRange &);
    VersionRange &operator&=(const VersionRange &);

    VersionRange &operator|=(const detail::RangePair &);
    VersionRange &operator&=(const detail::RangePair &);

    /// get range from string without throw
    static std::optional<VersionRange> parse(const std::string &s);

private:
    using Range = std::vector<detail::RangePair>;

    // always sorted with less
    Range range;

    /// range will be in an invalid state in case of errors
    /// optional error will be returned
    static std::optional<std::string> parse(VersionRange &v, const std::string &s);

    /*friend PRIMITIVES_VERSION_API bool operator<(const VersionRange &, const Version &);
    friend PRIMITIVES_VERSION_API bool operator<(const Version &, const VersionRange &);
    friend PRIMITIVES_VERSION_API bool operator>(const VersionRange &, const Version &);
    friend PRIMITIVES_VERSION_API bool operator>(const Version &, const VersionRange &);*/
};

/// Return true if version is greater than all the versions possible in the range.
/*PRIMITIVES_VERSION_API
bool operator<(const VersionRange &, const Version &);

/// Return true if version is less than all the versions possible in the range.
PRIMITIVES_VERSION_API
bool operator<(const Version &, const VersionRange &);

/// Return true if version is less than all the versions possible in the range.
PRIMITIVES_VERSION_API
bool operator>(const VersionRange &, const Version &);

/// Return true if version is greater than all the versions possible in the range.
PRIMITIVES_VERSION_API
bool operator>(const Version &, const VersionRange &);*/

struct PRIMITIVES_VERSION_API PackageVersionRange
{
    using Branch = PackageVersion::Branch;

    /// default is any version or * or Version::min() - Version::max()
    PackageVersionRange();

    // checkers
    bool isBranch() const;
    bool isRange() const;

    const VersionRange &getRange() const;
    const Branch &getBranch() const;

    std::string toString() const;

private:
    std::variant<VersionRange, Branch> value;
};

} // namespace primitives::version

namespace std
{

template<> struct hash<primitives::version::VersionRange>
{
    size_t operator()(const primitives::version::VersionRange &v) const
    {
        return v.getHash();
    }
};

}
