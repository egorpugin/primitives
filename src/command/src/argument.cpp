// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/argument.h>

#include <primitives/exceptions.h>

namespace primitives::command
{

Argument::Argument(const Element &s) : a(s)
{
}

// do not normalize, get as is
Argument::Argument(const path &p) : a(p.u8string())
{
}

const char *Argument::data() const
{
    return a.data();
}

Argument::Element Argument::substr(size_t pos, size_t count) const
{
    return a.substr(pos, count);
}

Argument::Element Argument::quote(QuoteType type) const
{
    switch (type)
    {
    case QuoteType::None:
        return a;
    case QuoteType::Simple:
        return "\"" + a + "\"";
    default:
        SW_UNIMPLEMENTED;
    }
}

size_t Argument::find(const Element &s) const
{
    return a.find(s);
}

char &Argument::operator[](int i)
{
    return a[i];
}

const char &Argument::operator[](int i) const
{
    return a[i];
}

Argument::operator path() const
{
    return a;
}

Argument::operator const Element &() const
{
    return a;
}

Argument::Element &Argument::get()
{
    return a;
}

const Argument::Element &Argument::get() const
{
    return a;
}

size_t Argument::size() const
{
    return a.size();
}

bool Argument::operator==(const Argument &rhs) const
{
    return a == rhs.a;
}

bool Argument::operator!=(const Argument &rhs) const
{
    return a != rhs.a;
}

Arguments::Arguments(const std::initializer_list<String> &l)
{
    for (auto &e : l)
        push_back(e);
}

Arguments::Arguments(const Storage &l)
{
    for (auto &e : l)
        push_back(e);
}

Arguments::Arguments(const Strings &l)
{
    for (auto &e : l)
        push_back(e);
}

Arguments::Element &Arguments::operator[](int i)
{
    return args[i];
}

const Arguments::Element &Arguments::operator[](int i) const
{
    return args[i];
}

void Arguments::push_back(const Element &s)
{
    args.push_back(s);
}

void Arguments::push_back(const char *s)
{
    args.push_back(String(s));
}

void Arguments::push_back(const String &s)
{
    args.push_back(s);
}

void Arguments::push_back(const path &p)
{
    args.push_back(p);
}

void Arguments::push_back(const Arguments &args2)
{
    for (auto &a : args2)
        args.push_back(a);
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

void Arguments::shift(int n)
{
    if (n > args.size())
        throw SW_RUNTIME_ERROR("shift: n > size()");
    auto t = std::move(args);
    args.assign(t.begin() + n, t.end());
}

} // namespace primitives::command
