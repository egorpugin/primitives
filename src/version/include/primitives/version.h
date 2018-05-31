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

//template <3>
struct PRIMITIVES_VERSION_API GenericNumericVersion
{
    using Number = detail::Number;
    using Numbers = std::vector<Number>;
    using Level = size_t;

    GenericNumericVersion();
    explicit GenericNumericVersion(Level level);
    explicit GenericNumericVersion(const std::initializer_list<Number> &, Level level = 0);

    static const Level minimum_level = 3;

#ifndef BISON_PARSER
protected:
#endif
    struct empty_tag {};

    explicit GenericNumericVersion(empty_tag, Level level = minimum_level);

    Numbers numbers;

    Number get(Level level) const;
    std::string printVersion() const;
    Level getLevel() const;
    void setLevel(Level level, Number fill = 0);
    void setFirstVersion();
    void fill(Number);
};

struct PRIMITIVES_VERSION_API Version : GenericNumericVersion
{
    // enough numbers, here string goes first
    //using Extra = std::vector<detail::comparable_variant<std::string, Number>>;
    using Extra = detail::access_vector<detail::comparable_variant<std::string, Number>>;

    /// default is min()
    Version();

    /// default is min()
    explicit Version(const Extra &e);

    /// parse from string
    explicit Version(const std::string &v);

    explicit Version(Level level);
    explicit Version(const std::initializer_list<Number> &);

    explicit Version(int ma);
    explicit Version(Number ma);
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

    std::string toString(Level level = minimum_level) const;
    std::string toString(const Version &v) const;

    // modificators
    void decrementVersion();
    void incrementVersion();
    Version getNextVersion() const;
    Version getPreviousVersion() const;

    void decrementVersion(Level level);
    void incrementVersion(Level level);
    Version getNextVersion(Level level) const;
    Version getPreviousVersion(Level level) const;

    /// format string
    void format(std::string &s) const;
    std::string format(const std::string &s) const;

    //
    bool isBranch() const;
    bool isVersion() const;
    //bool isTag() const; todo - add tags

    bool operator<(const Version &) const;
    bool operator>(const Version &) const;
    bool operator<=(const Version &) const;
    bool operator>=(const Version &) const;
    bool operator==(const Version &) const;
    bool operator!=(const Version &) const;

    // add +,-?
    // int compare()?

public:
    static Version min(Level level = minimum_level);
    static Version min(const Version &v);
    static Version max(Level level = minimum_level);
    static Version max(const Version &v);

    static Number maxNumber();

#ifndef BISON_PARSER
private:
#endif
    Extra extra;
    std::string branch;

    /// version will be in an invalid state in case of errors
    static bool parse(Version &v, const std::string &s);
    static bool parseExtra(Version &v, const std::string &s);

    std::string printExtra() const;

    void checkExtra() const;
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
    VersionRange(GenericNumericVersion::Level level = Version::minimum_level);

    /// from two versions
    VersionRange(const Version &v1, const Version &v2);

    /// parse from string
    VersionRange(const std::string &v);

    std::string toString() const;

    bool isEmpty() const;
    bool isOutside(const Version &) const;
    bool hasVersion(const Version &) const;

    optional<Version> getMinSatisfyingVersion(const std::set<Version> &) const;
    optional<Version> getMaxSatisfyingVersion(const std::set<Version> &) const;

public:
    bool operator==(const VersionRange &) const;
    bool operator!=(const VersionRange &) const;

    VersionRange operator|(const VersionRange &) const;
    VersionRange operator&(const VersionRange &) const;
    VersionRange &operator|=(const VersionRange &);
    VersionRange &operator&=(const VersionRange &);

public:
    /// get range from string without throw
    static optional<VersionRange> parse(const std::string &s);

    /// returns empty set
    static VersionRange empty();

#ifndef BISON_PARSER
private:
#endif
    // always sorted with less
    Range range;

    VersionRange &operator|=(const RangePair &);
    VersionRange &operator&=(const RangePair &);

    /// range will be in an invalid state in case of errors
    /// optional error will be returned
    static optional<std::string> parse(VersionRange &v, const std::string &s);

    friend bool operator<(const VersionRange &, const Version &);
    friend bool operator<(const Version &, const VersionRange &);
    friend bool operator>(const VersionRange &, const Version &);
    friend bool operator>(const Version &, const VersionRange &);
};

/// Return true if version is greater than all the versions possible in the range.
bool operator<(const VersionRange &, const Version &);

/// Return true if version is less than all the versions possible in the range.
bool operator<(const Version &, const VersionRange &);

/// Return true if version is less than all the versions possible in the range.
bool operator>(const VersionRange &, const Version &);

/// Return true if version is greater than all the versions possible in the range.
bool operator>(const Version &, const VersionRange &);

}
