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

//template <3>?
/*struct PRIMITIVES_VERSION_API GenericNumericVersion
{
    using Number = detail::Number;
    using Numbers = std::vector<Number>;

    // level is indexed starting from 1
    using Level = int;

    GenericNumericVersion();
    explicit GenericNumericVersion(const std::initializer_list<Number> &);

    Level getLevel() const;
    // non null level
    Level getRealLevel() const;

    Number &operator[](int i) { return numbers[i]; }
    Number operator[](int i) const { return numbers[i]; }

    size_t getHash() const;

    //static Level checkAndNormalizeLevel(Level);

#ifndef HAVE_BISON_RANGE_PARSER
protected:
#endif

    // data

    std::string printVersion() const;
    std::string printVersion(Level level) const;
    std::string printVersion(const std::string &delimeter) const;
    std::string printVersion(const std::string &delimeter, Level level) const;
    Number get(Level level) const;
    void setLevel(Level level, Number fill = 0);
    void setFirstVersion();
    void fill(Number);
};*/

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
struct PRIMITIVES_VERSION_API Extra
{
    Extra() = default;
    Extra(const char *);
    Extra(const std::string &);

    bool operator<(const Extra &) const;
    bool operator==(const Extra &rhs) const { return value == rhs.value; }
    bool operator!=(const Extra &rhs) const { return value != rhs.value; }

    auto &operator[](int i) { return value[i]; }
    const auto &operator[](int i) const { return value[i]; }

    bool empty() const { return value.empty(); }

    std::string print() const;

private:
    std::vector<comparable_variant<std::string, Number/*, any flag */>> value;

    bool parse(const std::string &s);
};

}

struct PRIMITIVES_VERSION_API Version
{
    using Number = detail::Number;
    using Numbers = std::vector<Number>;
    using Level = int;
    using Extra = detail::Extra;

    /// default is min()
    Version();

    /// construct from other version and extra
    Version(const Version &, const Extra &);

    /// parse from raw string
    Version(const char *); // no explicit
    /// parse from string
    Version(const std::string &); // no explicit

    /// from vector
    Version(const Numbers &); // no explicit
    /// from initializer_list
    Version(const std::initializer_list<Number> &); // no explicit

    Version(int ma);
    Version(Number ma);
    Version(Number ma, Number mi);
    Version(Number ma, Number mi, Number pa);
    Version(Number ma, Number mi, Number pa, Number tw);

    // basic access
    Number getMajor() const;
    Number getMinor() const;
    Number getPatch() const;
    Number getTweak() const;
    Extra &getExtra();
    const Extra &getExtra() const;
    // return as const ref like extra?
    std::string getBranch() const;

    Level getLevel() const;

    std::string toString() const;
    std::string toString(Level level) const;
    std::string toString(const std::string &delimeter) const;
    std::string toString(const std::string &delimeter, Level level) const;
    std::string toString(const Version &v) const;
    std::string toString(const std::string &delimeter, const Version &v) const;

    size_t getHash() const;

    // modificators
    void decrementVersion();
    void decrementVersion(Level level);
    void incrementVersion();
    void incrementVersion(Level level);

    //
    Version getNextVersion() const;
    Version getNextVersion(Level level) const;
    Version getPreviousVersion() const;
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
    static Version min(Level level);
    //static Version min(const Version &v);
    static Version max(Level level);
    //static Version max(const Version &v);
    static Level getDefaultLevel();
    static Number maxNumber();

    // TODO:
    /// extended version parsing, e.g. =v1.2
    //static Version coerce();

#ifndef HAVE_BISON_RANGE_PARSER
private:
#endif
    Numbers numbers;
    Extra extra;
    std::string branch;

    /// version will be in an invalid state in case of errors
    static bool parse(Version &v, const std::string &s);

    void checkNumber() const;
    void checkVersion() const;
    void prepareAndCheckVersion();
    void setFirstVersion();

    Number get(Level level) const;
    std::string printVersion(const std::string &delimeter, Level level) const;

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
