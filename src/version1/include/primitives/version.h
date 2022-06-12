// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <set>
#include <string>
#include <vector>

#include <optional>
#include <variant>

namespace primitives::version
{

namespace detail
{

std::string preprocess_input(const std::string &);

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
// TODO: add any extra (non-release)
// maybe special '*' sign
// e.g.: any non release is *-*
struct Extra : std::vector<comparable_variant<std::string, Number/*, any flag */>>
{
    using Base = std::vector<comparable_variant<std::string, Number>>;

    bool operator<(const Extra &) const;
};

}

//template <3>
struct PRIMITIVES_VERSION1_API GenericNumericVersion
{
    using Number = detail::Number;
    using Numbers = std::vector<Number>;

    // level is indexed starting from 1
    using Level = int;

    GenericNumericVersion();
    explicit GenericNumericVersion(const std::initializer_list<Number> &, Level level = minimum_level);

    Level getLevel() const;
    Level getRealLevel() const;

    Number &operator[](int i) { return numbers[i]; }
    Number operator[](int i) const { return numbers[i]; }

    static inline const Level minimum_level = 3;
    static Level checkAndNormalizeLevel(Level);
    static Number maxNumber();

#ifndef HAVE_BISON_RANGE_PARSER
protected:
#endif

    // data
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

struct PRIMITIVES_VERSION1_API Version : GenericNumericVersion
{
    using Extra = detail::Extra;

    /// default is min()
    Version();

    /// default is min()
    explicit Version(const Extra &);

    /// construct from other version and extra
    Version(const Version &, const Extra &);

    /// construct from other version and extra
    Version(const Version &version, const std::string &extra);

    /// parse from string
    Version(const std::string &version); // no explicit

    /// parse from string + extra string
    Version(const std::string &version, const std::string &extra);

    /// parse from raw string
    Version(const char *); // no explicit

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
    Extra &getExtra();
    const Extra &getExtra() const;
    // return as const ref like extra?
    std::string getBranch() const;

    std::string toString() const;
    std::string toString(Level level) const;
    std::string toString(const std::string &delimeter) const;
    std::string toString(const std::string &delimeter, Level level) const;
    std::string toString(const Version &v) const;
    std::string toString(const std::string &delimeter, const Version &v) const;

    size_t getHash() const;

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

    bool hasExtra() const;
    bool isRelease() const;
    bool isPreRelease() const;

    Level getMatchingLevel(const Version &v) const;

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

    template <class F>
    static bool cmp(const Version &v1, const Version &v2, F f);
};

} // namespace primitives::version

namespace std
{

template<> struct hash<primitives::version::Version>
{
    size_t operator()(const primitives::version::Version &v) const
    {
        return v.getHash();
    }
};

}
