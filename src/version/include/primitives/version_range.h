// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "version_helpers.h"

namespace primitives::version
{

struct PRIMITIVES_VERSION_API VersionRange
{
    /// default is any version or * or Version::min() - Version::max()
    VersionRange();

    // separate ctor!
    /// default is any version or * or Version::min() - Version::max()
    explicit VersionRange(Version::Level level);

    /// from one version
    VersionRange(const Version &);

    /// from two versions
    VersionRange(const Version &from, const Version &to);

    /// parse from raw string
    VersionRange(const char *); // no explicit

    /// parse from string
    VersionRange(const std::string &); // no explicit

    /// convert to single version if possible
    /// consists of one range and first == second
    std::optional<Version> toVersion() const;

    /// convert to string
    std::string toString() const;

    /// call to cppan1
    std::string toStringV1() const;

    size_t getHash() const;

    bool isEmpty() const;
    bool isOutside(const Version &) const;
    bool hasVersion(const Version &) const;
    size_t size() const;

    std::optional<Version> getMinSatisfyingVersion(const std::set<Version> &) const;
    std::optional<Version> getMaxSatisfyingVersion(const std::set<Version> &) const;

    // operators
    bool operator<(const VersionRange &) const;
    bool operator==(const VersionRange &) const;
    bool operator!=(const VersionRange &) const;

    // TODO:
    //VersionRange operator~() const;

    VersionRange operator|(const VersionRange &) const;
    VersionRange operator&(const VersionRange &) const;
    VersionRange &operator|=(const VersionRange &);
    VersionRange &operator&=(const VersionRange &);

public:
    /// get range from string without throw
    static std::optional<VersionRange> parse(const std::string &s);

    /// returns empty set
    static VersionRange empty();

#ifndef HAVE_BISON_RANGE_PARSER
private:
#endif
    /// [from, to] interval
    struct RangePair : std::pair<Version, Version>
    {
        // replace with [from, to)?
        // but we do not know what is next version for "15.9.03232.13 - 16.0.123.13-preview"
        // so we can't use [,)
        // to do this, we must teach Extra to know it's next version
        //
        // Example: for string "a" next version will be "aa" - in case if we allow only [a-z]
        //

        using base = std::pair<Version, Version>;
        using base::base;

        std::string toString() const;
        bool hasVersion(const Version &v) const;
        bool isBranch() const;
        size_t getHash() const;
        std::optional<Version> toVersion() const;

        std::optional<RangePair> operator&(const RangePair &) const;
    };

    using Range = std::vector<RangePair>;

    // always sorted with less
    Range range;
    std::string branch;

    bool isBranch() const;

    VersionRange &operator|=(const RangePair &);
    VersionRange &operator&=(const RangePair &);

    /// range will be in an invalid state in case of errors
    /// optional error will be returned
    static std::optional<std::string> parse(VersionRange &v, const std::string &s);

    friend PRIMITIVES_VERSION_API bool operator<(const VersionRange &, const Version &);
    friend PRIMITIVES_VERSION_API bool operator<(const Version &, const VersionRange &);
    friend PRIMITIVES_VERSION_API bool operator>(const VersionRange &, const Version &);
    friend PRIMITIVES_VERSION_API bool operator>(const Version &, const VersionRange &);
};

/// Return true if version is greater than all the versions possible in the range.
PRIMITIVES_VERSION_API
bool operator<(const VersionRange &, const Version &);

/// Return true if version is less than all the versions possible in the range.
PRIMITIVES_VERSION_API
bool operator<(const Version &, const VersionRange &);

/// Return true if version is less than all the versions possible in the range.
PRIMITIVES_VERSION_API
bool operator>(const VersionRange &, const Version &);

/// Return true if version is greater than all the versions possible in the range.
PRIMITIVES_VERSION_API
bool operator>(const Version &, const VersionRange &);

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
