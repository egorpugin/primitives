// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>

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

struct PRIMITIVES_COMMAND_API ArgumentPosition
{
    bool operator<(const ArgumentPosition &) const;
    void push_back(int);

private:
    std::vector<int> positions;
};

struct PRIMITIVES_COMMAND_API Argument
{
    virtual ~Argument() = 0;

    virtual String toString() const = 0;
    virtual std::unique_ptr<Argument> clone() const = 0;
    virtual const ArgumentPosition &getPosition() const;

    String quote(QuoteType type = QuoteType::Simple) const;
    static String quote(const String &, QuoteType type = QuoteType::Simple);
};

struct PRIMITIVES_COMMAND_API SimpleArgument : Argument
{
    SimpleArgument() = default;
    SimpleArgument(const String &);
    SimpleArgument(const path &);

    String toString() const override;
    std::unique_ptr<Argument> clone() const override;

private:
    String a;
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
    Arguments(const std::initializer_list<String> &);
    Arguments(const Strings &);

    Arguments(const Arguments &);
    Arguments &operator=(const Arguments &);

    void push_back(Element &&);
    void push_back(const char *);
    void push_back(const String &);
    void push_back(const path &);
    void push_back(const Arguments &);

    void push_front(Element &&);
    void push_front(const char *);
    void push_front(const String &);
    void push_front(const path &);
    void push_front(const Arguments &);

    bool empty() const { return args.empty(); }
    size_t size() const;

    Element &operator[](int i);
    const Element &operator[](int i) const;

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

private:
    Storage args;
};

}
