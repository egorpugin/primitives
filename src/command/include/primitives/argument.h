// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>

namespace primitives::command
{

enum class QuoteType
{
    // return as is
    None,

    /// Add " to begin and end
    Simple,

    /// Simple plus escape " inside string
    SimpleAndEscape,

    // cmd
    // shell

    // batch
};

struct PRIMITIVES_COMMAND_API Argument
{
    using Element = String;

    static inline const size_t npos = -1;

    Argument() = default;
    Argument(const Element &);
    Argument(const path &);

    const char *data() const;
    Element quote(QuoteType type = QuoteType::Simple) const;
    size_t size() const;
    size_t find(const Element &) const;
    Element substr(size_t pos, size_t count = npos) const;

    Element &get();
    const Element &get() const;
    operator const Element &() const;
    operator path() const;

    char &operator[](int i);
    const char &operator[](int i) const;

    bool operator==(const Argument &) const;
    bool operator!=(const Argument &) const;
    // other ops

private:
    Element a;

    friend bool operator==(const Argument &, const Element &);
    friend bool operator!=(const Argument &, const Element &);
    friend bool operator==(const Element &, const Argument &);
    friend bool operator!=(const Element &, const Argument &);
};

struct PRIMITIVES_COMMAND_API Arguments
{
    using Element = Argument;
    using Storage = std::vector<Element>;
    using iterator = Storage::iterator;
    using const_iterator = Storage::const_iterator;

    Arguments() = default;
    Arguments(const std::initializer_list<Element::Element> &);
    Arguments(const Storage &);
    Arguments(const Strings &);

    void push_back(const Element &);
    void push_back(const char *);
    void push_back(const Element::Element &);
    void push_back(const path &);
    void push_back(const Arguments &);

    bool empty() const { return args.empty(); }
    size_t size() const;
    void shift(int n = 1);

    Element &operator[](int i);
    const Element &operator[](int i) const;

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

private:
    Storage args;
};

// make dll?
inline
bool operator==(const Argument &a, const Argument::Element &s)
{
    return a.a == s;
}

inline
bool operator!=(const Argument &a, const Argument::Element &s)
{
    return a.a != s;
}

inline
bool operator==(const Argument::Element &s, const Argument &a)
{
    return s == a.a;
}

inline
bool operator!=(const Argument::Element &s, const Argument &a)
{
    return s != a.a;
}

}
