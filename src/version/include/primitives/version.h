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

using Number = int64_t;

std::string preprocess_input(const std::string &);
bool is_branch(const std::string &);

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
    std::string toString(const std::string &delimeter, const Version &v) const;

    size_t getHash() const;

    /// format string
    [[nodiscard]]
    std::string format(const std::string &s) const;

    bool hasExtra() const;
    bool isRelease() const;
    bool isPreRelease() const;

    Level getMatchingLevel(const Version &v) const;

    void setFirstVersion();

    // operators
    bool operator<(const Version &) const;
    bool operator>(const Version &) const;
    bool operator<=(const Version &) const;
    bool operator>=(const Version &) const;
    bool operator==(const Version &) const;

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

    void check() const;
    Number get(Level level) const;
    std::string printVersion(const std::string &delimeter, Level level) const;

    // modificators
    void decrementVersion();
    void incrementVersion();

    void push_back(Number n) { numbers.push_back(n); }

    /// version will be in an invalid state in case of errors
    static bool parse(Version &v, const std::string &s);

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
