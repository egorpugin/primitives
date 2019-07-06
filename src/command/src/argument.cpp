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

Argument::~Argument()
{
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
SimpleArgument::SimpleArgument(const path &p) : a(p.u8string())
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

void Arguments::push_back(Element e)
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
        args.push_back(a->clone());
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

void Arguments::push_front(const path &p)
{
    args.push_front(std::make_unique<SimpleArgument>(p));
}

} // namespace primitives::command