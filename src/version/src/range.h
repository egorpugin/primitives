// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#pragma warning(disable: 4005)
#pragma warning(disable: 4065)

#include <primitives/preprocessor.h>

#include <any>
#include <list>
#include <string>

// setup parser
#define THIS_PARSER_NAME range
#define THIS_PARSER_NAME_UP RANGE
#define MY_PARSER RangeParser

//      vvvvvvvv change here
#define yy_rangelex(val,loc) ((RangeParser*)&yyparser)->lex(val,loc)

#define THIS_LEXER CONCATENATE(ll_, THIS_PARSER_NAME)
#define THIS_LEXER_UP CONCATENATE(LL_, THIS_PARSER_NAME_UP)
#define LEXER_NAME(v) CONCATENATE(THIS_LEXER, v)
#define LEXER_NAME_UP(v) CONCATENATE(THIS_LEXER_UP, v)

#define THIS_PARSER CONCATENATE(yy_, THIS_PARSER_NAME)
#define THIS_PARSER_UP CONCATENATE(YY_, THIS_PARSER_NAME_UP)
#define PARSER_NAME(v) CONCATENATE(THIS_PARSER, v)
#define PARSER_NAME_UP(v) CONCATENATE(THIS_PARSER_UP, v)

// before parser
template <class T>
struct PARSER_NAME(storage)
{
    std::any *v;
    std::any *v2;

    template <class U>
    bool hasType() const
    {
        auto r = v->type() == typeid(U);
        if (!r)
            throw std::runtime_error(std::string("Incorrect type: ") + v->type().name() + ", wanted type: " + typeid(U).name());
        return r;
    }

    template <class U>
    void setValue(T &storage, const U &v2)
    {
        if (v)
        {
            *v = v2;
        }
        else
        {
            storage.data.emplace_back(v2);
            v = &storage.data.back();
        }
    }

    template <class U>
    U getValue() const
    {
        return std::any_cast<U>(*v);
    }
};

struct MY_PARSER;
#define MY_STORAGE PARSER_NAME(storage)<MY_PARSER>

// yy macros
#define GET_STORAGE(v) ((MY_STORAGE *)&v)

#define CHECK_TYPE(type, bison_var)               \
    if (!GET_STORAGE(bison_var)->hasType<type>()) \
    YYABORT

#define EXTRACT_VALUE_UNCHECKED(type, bison_var) \
    GET_STORAGE(bison_var)->getValue<type>()

#define EXTRACT_VALUE(type, var, bison_var) \
    CHECK_TYPE(type, bison_var);            \
    type var = EXTRACT_VALUE_UNCHECKED(type, bison_var)

#define SET_VALUE(bison_var, v) GET_STORAGE(bison_var)->setValue((MY_PARSER &)yyparser, v)

// last include
#include <range.yy.hpp>

// lexer
#define LEXER_FUNCTION std::tuple<int, std::any> LEXER_NAME(lex)(void *yyscanner, THIS_PARSER::parser::location_type &loc)
#define YY_DECL LEXER_FUNCTION
LEXER_FUNCTION;

#define MAKE(x) {THIS_PARSER::parser::token::x, {}}
#define MAKE_VALUE(x, v) {THIS_PARSER::parser::token::x, v}

struct MY_PARSER : THIS_PARSER::parser
{
    using base = THIS_PARSER::parser;

    bool debug = true;
    bool can_throw = false;

    int parse(const std::string &s);

    template <class T>
    T getResult() const
    {
        return std::any_cast<T>(data.back());
    }

    // private
    int lex(PARSER_NAME_UP(STYPE) *val, location_type *loc);

private:
    void *scanner;

    void error(const location_type &loc, const std::string& msg) override;

    // data storage, iterators are not invalidated
    std::list<std::any> data;

    template <class T>
    friend struct PARSER_NAME(storage);
};
