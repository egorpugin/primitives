// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/argument.h>

#include <boost/algorithm/string.hpp>
#include <primitives/exceptions.h>

namespace primitives::command
{

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

bool ArgumentPosition::operator<(const ArgumentPosition &rhs) const
{
    if (positions.size() == rhs.positions.size())
        return positions < rhs.positions;
    return position_compare(positions.begin(), positions.end(), rhs.positions.begin(), rhs.positions.end());
}

void ArgumentPosition::push_back(int v)
{
    positions.push_back(v);
}

Argument::~Argument()
{
}

const ArgumentPosition &Argument::getPosition() const
{
    static const ArgumentPosition p;
    return p;
}

String Argument::quote(QuoteType type) const
{
    return quote(toString(), type);
}

String Argument::quote(const String &in_s, QuoteType type)
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

SimpleArgument::SimpleArgument(const String &s) : a(s)
{
}

// do not normalize, get as is
SimpleArgument::SimpleArgument(const path &p) : a(to_string(to_path_string(p)))
{
}

String SimpleArgument::toString() const
{
    return a;
}

std::unique_ptr<Argument> SimpleArgument::clone() const
{
    return std::make_unique<SimpleArgument>(a);
}

Arguments::Arguments(const std::initializer_list<String> &l)
{
    for (auto &e : l)
        push_back(e);
}

Arguments::Arguments(const Strings &l)
{
    for (auto &e : l)
        push_back(e);
}

Arguments::Arguments(const Arguments &rhs)
{
    operator=(rhs);
}

Arguments &Arguments::operator=(const Arguments &rhs)
{
    args.clear();
    for (auto &e : rhs)
        push_back(e->clone());
    return *this;
}

Arguments::Element &Arguments::operator[](int i)
{
    return args[i];
}

const Arguments::Element &Arguments::operator[](int i) const
{
    return args[i];
}

void Arguments::push_back(Element &&e)
{
    args.push_back(std::move(e));
}

void Arguments::push_back(const char *s)
{
    push_back(String(s));
}

void Arguments::push_back(const String &s)
{
    push_back(std::make_unique<SimpleArgument>(s));
}

void Arguments::push_back(const path &p)
{
    push_back(std::make_unique<SimpleArgument>(p));
}

void Arguments::push_back(const Arguments &args2)
{
    for (auto &a : args2)
        push_back(a->clone());
}

void Arguments::push_front(Element &&e)
{
    args.push_front(std::move(e));
}

void Arguments::push_front(const char *s)
{
    push_front(String(s));
}

void Arguments::push_front(const String &s)
{
    push_front(std::make_unique<SimpleArgument>(s));
}

void Arguments::push_front(const path &p)
{
    push_front(std::make_unique<SimpleArgument>(p));
}

void Arguments::push_front(const Arguments &args2)
{
    for (auto &a : args2)
        push_front(a->clone());
}

size_t Arguments::size() const
{
    return args.size();
}

Arguments::iterator Arguments::begin()
{
    return args.begin();
}

Arguments::iterator Arguments::end()
{
    return args.end();
}

Arguments::const_iterator Arguments::begin() const
{
    return args.begin();
}

Arguments::const_iterator Arguments::end() const
{
    return args.end();
}

} // namespace primitives::command
