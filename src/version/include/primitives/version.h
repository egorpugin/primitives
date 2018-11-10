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

// enough numbers, here string goes first
struct Extra : std::vector<comparable_variant<std::string, Number>>
{
    using Base = std::vector<comparable_variant<std::string, Number>>;

    bool operator<(const Extra &) const;
};

}

//template <3>
struct PRIMITIVES_VERSION_API GenericNumericVersion
{
    using Number = detail::Number;
    using Numbers = std::vector<Number>;

    // level is indexed starting from 1
    using Level = size_t;

    GenericNumericVersion();
    explicit GenericNumericVersion(Level level);
    explicit GenericNumericVersion(const std::initializer_list<Number> &, Level level = 0);

    Level getLevel() const;

    Number &operator[](int i) { return numbers[i]; }
    Number operator[](int i) const { return numbers[i]; }

    static inline const Level minimum_level = 3;

#ifndef HAVE_BISON_RANGE_PARSER
protected:
#endif
    struct empty_tag {};

    explicit GenericNumericVersion(empty_tag, Level level = minimum_level);

    Numbers numbers;

    std::string printVersion() const;
    std::string printVersion(Level level = minimum_level) const;
    std::string printVersion(const std::string &delimeter) const;
    std::string printVersion(const std::string &delimeter, Level level) const;
    Number get(Level level) const;
    void setLevel(Level level, Number fill = 0);
    void setFirstVersion();
    void fill(Number);
};

struct PRIMITIVES_VERSION_API Version : GenericNumericVersion
{
    using Extra = detail::Extra;

    /// default is min()
    Version();

    /// default is min()
    explicit Version(const Extra &);

    /// parse from string
    Version(const std::string &); // no explicit

    /// parse from raw string
    Version(const char *); // no explicit

    explicit Version(Level level);
    Version(const std::initializer_list<Number> &); // no explicit

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
    // return as copy to prevent overwrites?
    const Extra &getExtra() const;
    // return as const ref like extra?
    std::string getBranch() const;

    std::string toString() const;
    std::string toString(Level level) const;
    std::string toString(const std::string &delimeter) const;
    std::string toString(const std::string &delimeter, Level level) const;
    std::string toString(const Version &v) const;
    std::string toString(const std::string &delimeter, const Version &v) const;

    size_t getStdHash() const;

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

    // checkers
    bool isBranch() const;
    bool isVersion() const;
    //bool isTag() const; todo - add tags

    // operators
    bool operator<(const Version &) const;
    bool operator>(const Version &) const;
    bool operator<=(const Version &) const;
    bool operator>=(const Version &) const;
    bool operator==(const Version &) const;
    bool operator!=(const Version &) const;

    Version &operator++();
    Version &operator--();
    Version operator++(int);
    Version operator--(int);

    // add +,-?
    // int compare()?

public:
    static Version min(Level level = minimum_level);
    static Version min(const Version &v);
    static Version max(Level level = minimum_level);
    static Version max(const Version &v);

    static Number maxNumber();

    // TODO:
    /// extended version parsing, e.g. =v1.2
    //static Version coerce();

#ifndef HAVE_BISON_RANGE_PARSER
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
    /// default is any version or * or Version::min() - Version::max()
    VersionRange(GenericNumericVersion::Level level = Version::minimum_level);

    /// from one version
    VersionRange(const Version &);

    /// from two versions
    VersionRange(const Version &from, const Version &to);

    /// parse from raw string
    VersionRange(const char *); // no explicit

    /// parse from string
    VersionRange(const std::string &); // no explicit

    /// convert to string
    std::string toString() const;

    /// call to cppan1
    std::string toStringV1() const;

    size_t getStdHash() const;

    bool isEmpty() const;
    bool isOutside(const Version &) const;
    bool hasVersion(const Version &) const;
    size_t size() const;

    optional<Version> getMinSatisfyingVersion(const std::set<Version> &) const;
    optional<Version> getMaxSatisfyingVersion(const std::set<Version> &) const;

    // operators
    bool operator<(const VersionRange &) const;
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

#ifndef HAVE_BISON_RANGE_PARSER
private:
#endif
    struct RangePair : std::pair<Version, Version>
    {
        using base = std::pair<Version, Version>;
        using base::base;

        std::string toString() const;
        bool hasVersion(const Version &v) const;
        bool isBranch() const;
        size_t getStdHash() const;

        optional<RangePair> operator&(const RangePair &) const;
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
    static optional<std::string> parse(VersionRange &v, const std::string &s);

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

}

namespace std
{

template<> struct hash<primitives::version::Version>
{
    size_t operator()(const primitives::version::Version &v) const
    {
        return v.getStdHash();
    }
};

template<> struct hash<primitives::version::VersionRange>
{
    size_t operator()(const primitives::version::VersionRange &v) const
    {
        return v.getStdHash();
    }
};

}
