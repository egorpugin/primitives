// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>

#include <boost/algorithm/string.hpp>
#include <primitives/exceptions.h>

#include <deque>

namespace primitives::command
{

enum class QuoteType
{
    // return as is
    None,

    /// Add " to begin and end
    Simple,

    /// Escape " and \ inside string
    Escape,

    /// Simple plus Escape
    SimpleAndEscape,

    // cmd
    // shell

    // batch
};

template<class InputIt1, class InputIt2>
static bool position_compare(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2)
{
    for (; first1 != last1 && first2 != last2; ++first1, ++first2)
    {
        if (*first1 < *first2) return true;
        if (*first1 > *first2) return false;
    }

    // equal, but we do not meet such case here for now
    // because equals check is earlier before this call
    if (first1 == last1 && first2 == last2)
        return false;

    if (first1 == last1)
    {
        // why loop?
        // because 1,2,3 == 1,2,3,0
        // and     1,2,3 != 1,2,3,0,0,0,-1
        for (; first2 != last2; ++first2)
        {
            // 1,2,3 < 1,2,3,1
            if (*first2 > 0) return true;

            // 1,2,3 > 1,2,3,-1
            if (*first2 < 0) return false;
        }
    }
    else // first2 == last2
    {
        // why loop?
        // because 1,2,3,0        == 1,2,3
        // and     1,2,3,0,0,0,-1 != 1,2,3
        for (; first1 != last1; ++first1)
        {
            // 1,2,3,-1 < 1,2,3
            if (*first1 < 0) return true;

            // 1,2,3,1 > 1,2,3
            if (*first1 > 0) return false;
        }
    }

    // 1,2,3,0,0 == 1,2,3,0,0,0,0
    return false;
}

struct ArgumentPosition
{
    bool operator<(const ArgumentPosition &rhs) const
    {
        if (positions.size() == rhs.positions.size())
            return positions < rhs.positions;
        return position_compare(positions.begin(), positions.end(), rhs.positions.begin(), rhs.positions.end());
    }
    void push_back(int v)
    {
        positions.push_back(v);
    }

private:
    std::vector<int> positions;
};

struct Argument
{
    virtual ~Argument(){}

    virtual String toString() const = 0;
    virtual std::unique_ptr<Argument> clone() const = 0;
    virtual const ArgumentPosition &getPosition() const
    {
        static const ArgumentPosition p;
        return p;
    }

    String quote(QuoteType type = QuoteType::Simple) const
    {
        return quote(toString(), type);
    }
    static String quote(const String &in_s, QuoteType type = QuoteType::Simple)
    {
        switch (type)
        {
        case QuoteType::None:
            return in_s;
        case QuoteType::Simple:
            if (!in_s.empty() && in_s[0] == '\"')
                return in_s;
            return "\"" + in_s + "\"";
        case QuoteType::Escape:
        {
            auto s = in_s;
            boost::replace_all(s, "\\", "\\\\");
            boost::replace_all(s, "\"", "\\\"");
            return s;
        }
        case QuoteType::SimpleAndEscape:
            return quote(quote(in_s, QuoteType::Escape), QuoteType::Simple);
        default:
            SW_UNIMPLEMENTED;
        }
    }
};

struct PRIMITIVES_COMMAND_API SimpleArgument : Argument
{
    SimpleArgument() = default;
    SimpleArgument(const char *s) : a(s) {
    }
    SimpleArgument(const String &s) : a(s)
    {
    }
    SimpleArgument(const path &p) : a(to_string(to_path_string(p)))
    {
    }

    String toString() const override
    {
        return a;
    }
    std::unique_ptr<Argument> clone() const override
    {
        return std::make_unique<SimpleArgument>(a);
    }

private:
    String a;
public:
    bool affects_output{true};
};

struct PRIMITIVES_COMMAND_API SimplePositionalArgument : SimpleArgument
{
    using SimpleArgument::SimpleArgument;

    const ArgumentPosition &getPosition() const override { return pos; }
    ArgumentPosition &getPosition() { return pos; }

    std::unique_ptr<Argument> clone() const override { return std::make_unique<SimplePositionalArgument>(*this); }

private:
    ArgumentPosition pos;
};

struct PRIMITIVES_COMMAND_API Arguments
{
    using Element = std::unique_ptr<Argument>;
    using Storage = std::deque<Element>;
    using iterator = Storage::iterator;
    using const_iterator = Storage::const_iterator;

    Arguments() = default;
    Arguments(const std::initializer_list<String> &l) {
        for (auto &e : l)
            push_back(e);
    }
    Arguments(const Strings &l) {
        for (auto &e : l)
            push_back(e);
    }
    Arguments(const Files &l) {
        for (auto &e : l)
            push_back(e);
    }
    Arguments(const Arguments &rhs) {
        operator=(rhs);
    }
    Arguments &operator=(const Arguments &rhs) {
        args.clear();
        for (auto &e : rhs)
            push_back(e->clone());
        return *this;
    }

    template <typename A>
    A &push_back(std::unique_ptr<A> e) {
        auto p = e.get();
        args.push_back(std::move(e));
        return *p;
    }
    auto &push_back(auto &&arg) {
        return push_back(std::make_unique<SimpleArgument>(FWD(arg)));
    }
    void push_back(const Arguments &args2) {
        for (auto &a : args2)
            push_back(a->clone());
    }

    void push_front(Element &&e) {
        args.push_front(std::move(e));
    }
    void push_front(const char *s) {
        push_front(String(s));
    }
    void push_front(const String &s) {
        push_front(std::make_unique<SimpleArgument>(s));
    }
    void push_front(const path &p) {
        push_front(std::make_unique<SimpleArgument>(p));
    }
    void push_front(const Arguments &args2) {
        for (auto &a : args2)
            push_front(a->clone());
    }

    bool empty() const { return args.empty(); }
    size_t size() const
    {
        return args.size();
    }

    Element &operator[](int i)
    {
        return args[i];
    }
    const Element &operator[](int i) const
    {
        return args[i];
    }

    iterator begin()
    {
        return args.begin();
    }
    iterator end()
    {
        return args.end();
    }
    const_iterator begin() const
    {
        return args.begin();
    }
    const_iterator end() const
    {
        return args.end();
    }

private:
    Storage args;
};

}
