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
    using Value = comparable_variant<std::string, Number/*, any flag */>;

    Extra() = default;
    Extra(const char *);
    Extra(const std::string &);

    bool operator<(const Extra &) const;
    bool operator==(const Extra &rhs) const { return values == rhs.values; }
    bool operator!=(const Extra &rhs) const { return values != rhs.values; }

    auto &operator[](int i) { return values[i]; }
    const auto &operator[](int i) const { return values[i]; }

    bool empty() const { return values.empty(); }

    void push_back(const Value &v) { values.push_back(v); }
    void append(const Extra &rhs) { values.insert(values.end(), rhs.values.begin(), rhs.values.end()); }

    std::string print() const;

private:
    std::vector<Value> values;

    bool parse(const std::string &s);
};

PRIMITIVES_VERSION_API
bool isBranch(const std::string &);

}

struct PRIMITIVES_VERSION_API Version
{
    using Number = detail::Number;
    struct Numbers : std::vector<Number> { using std::vector<Number>::vector; }; // for natvis
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

    Level getLevel() const;
    // non-zero level
    Level getRealLevel() const;

    std::string toString() const;
    std::string toString(Level level) const;
    std::string toString(const std::string &delimeter) const;
    std::string toString(const std::string &delimeter, Level level) const;
    //std::string toString(const Version &v) const;
    std::string toString(const std::string &delimeter, const Version &v) const;

    size_t getHash() const;

    void push_back(Number n) { numbers.push_back(n); }

    // modificators
    void decrementVersion();
    void incrementVersion();
    /*void decrementVersion(Level level);
    void incrementVersion(Level level);*/

    //
    /*Version getNextVersion() const;
    Version getNextVersion(Level level) const;
    Version getPreviousVersion() const;
    Version getPreviousVersion(Level level) const;*/

    /// format string
    [[nodiscard]]
    std::string format(const std::string &s) const;

    bool hasExtra() const;
    bool isRelease() const;
    bool isPreRelease() const;

    Level getMatchingLevel(const Version &v) const;

    //void prepareAndCheckVersion();
    void check() const;
    void setFirstVersion();

    // operators
    bool operator<(const Version &) const;
    bool operator>(const Version &) const;
    bool operator<=(const Version &) const;
    bool operator>=(const Version &) const;
    bool operator==(const Version &) const;
    bool operator!=(const Version &) const;

    // all calculations are on real level
    Version &operator++();
    Version operator++(int);
    Version &operator--();
    Version operator--(int);

    // add +,-?
    // int compare()?

public:
    // [min(), max())
    static Version min();
    static Version max();
    static Number maxNumber();
    static Level getDefaultLevel();

    // TODO:
    /// extended version parsing, e.g. =v1.2
    //static Version coerce();

#ifndef HAVE_BISON_RANGE_PARSER
private:
#endif
    Numbers numbers;
    Extra extra;

    /// version will be in an invalid state in case of errors
    static bool parse(Version &v, const std::string &s);

    Number get(Level level) const;
    std::string printVersion(const std::string &delimeter, Level level) const;

    template <class F>
    static bool cmp(const Version &v1, const Version &v2, F f);
};

struct PRIMITIVES_VERSION_API PackageVersion
{
    using Branch = std::string;

    PackageVersion();
    PackageVersion(const char *);
    PackageVersion(const std::string &);
    PackageVersion(const Version &);
    //PackageVersion(const PackageVersion &) = default;

    // checkers
    bool isBranch() const;
    bool isVersion() const;
    //bool isTag() const; todo - add tags?

    Version &getVersion();
    const Version &getVersion() const;
    const Branch &getBranch() const;

    bool isRelease() const;
    bool isPreRelease() const;

    [[nodiscard]]
    std::string format(const std::string &s) const;
    [[nodiscard]]
    std::string toString() const;

    // operators
    bool operator<(const PackageVersion &) const;
    bool operator>(const PackageVersion &) const;
    bool operator<=(const PackageVersion &) const;
    bool operator>=(const PackageVersion &) const;
    bool operator==(const PackageVersion &) const;
    bool operator!=(const PackageVersion &) const;

    //static Version min();

private:
    std::variant<Version, Branch> value;

    void checkAndSetFirstVersion();
    void check() const;
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
