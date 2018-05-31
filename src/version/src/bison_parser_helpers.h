// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#pragma warning(disable: 4005)
#pragma warning(disable: 4065)

#include <primitives/preprocessor.h>
#include <primitives/stdcompat/optional.h>

#include <any>
#include <list>
#include <sstream>
#include <string>

//
#define THIS_LEXER CONCATENATE(ll_, THIS_PARSER_NAME)
#define THIS_LEXER_UP CONCATENATE(LL_, THIS_PARSER_NAME_UP)
#define LEXER_NAME(v) CONCATENATE(THIS_LEXER, v)
#define LEXER_NAME_UP(v) CONCATENATE(THIS_LEXER_UP, v)

#define THIS_PARSER CONCATENATE(yy_, THIS_PARSER_NAME)
#define THIS_PARSER_UP CONCATENATE(YY_, THIS_PARSER_NAME_UP)
#define PARSER_NAME(v) CONCATENATE(THIS_PARSER, v)
#define PARSER_NAME_UP(v) CONCATENATE(THIS_PARSER_UP, v)

#define MY_PARSER_CAST ((MY_PARSER &)yyparser)
#define MY_PARSER_CAST_BB (((MY_PARSER &)yyparser).bb)

struct basic_block;

struct bison_any_storage
{
    std::any *v;

    template <class U>
    bool checkType(basic_block &bb) const
    {
        auto &t1 = v->type();
        auto &t2 = typeid(U);
        if (t1 != t2)
            bb.error_msg = std::string("Incorrect type: ") + t1.name() + ", wanted type: " + t2.name();
        return t1 == t2;
    }

    template <class U>
    bool hasType(basic_block &bb) const
    {
        return checkType<U>(bb);
    }

    template <class U>
    void setValue(basic_block &bb, const U &v2)
    {
        v = bb.add_data(v2);
    }

    template <class U>
    U getValue() const
    {
        return std::any_cast<U>(*v);
    }
};

#define MY_STORAGE bison_any_storage

// yy macros
#define GET_STORAGE(v) ((MY_STORAGE *)&v)

#define CHECK_TYPE(type, bison_var)                             \
    if (!GET_STORAGE(bison_var)->hasType<type>(MY_PARSER_CAST_BB)) \
    YYABORT

#define EXTRACT_VALUE_UNCHECKED(type, bison_var) \
    GET_STORAGE(bison_var)->getValue<type>()

#define EXTRACT_VALUE(type, var, bison_var) \
    CHECK_TYPE(type, bison_var);            \
    type var = EXTRACT_VALUE_UNCHECKED(type, bison_var)

#define SET_VALUE(bison_var, v) GET_STORAGE(bison_var)->setValue(MY_PARSER_CAST_BB, v)

#define SET_PARSER_RESULT(v) MY_PARSER_CAST_BB.setResult(*v)

// lexer
#define LEXER_FUNCTION std::tuple<int, std::any> LEXER_NAME(lex)(void *yyscanner, THIS_PARSER::parser::location_type &loc)
#define YY_DECL LEXER_FUNCTION
#define MAKE(x) {THIS_PARSER::parser::token::x, {}}
#define MAKE_VALUE(x, v) {THIS_PARSER::parser::token::x, v}

struct basic_block
{
    void *scanner = nullptr;
    bool can_throw = false;
    std::string error_msg;

    // data storage, list iterators are not invalidated
    std::list<std::any> data;

    template <class U>
    auto add_data(U &&u)
    {
        data.emplace_back(std::forward<U>(u));
        return &data.back();
    }

    template <class T>
    void setResult(const T &v)
    {
        return data.push_back(v);
    }

    template <class T>
    optional<T> getResult() const
    {
        if (data.empty())
            return {};
        return std::any_cast<T>(data.back());
    }
};

template <class BaseParser>
struct some_parser : BaseParser
{
    basic_block bb;

    void error(const typename BaseParser::location_type &loc, const std::string& msg) override
    {
        std::ostringstream ss;
        ss << loc << " " << msg << "\n";
        bb.error_msg = ss.str();
        if (bb.can_throw)
            throw std::runtime_error("Error during parse: " + ss.str());
    }
};

// for parse()
// set_debug_level(bb.debug);

#define DECLARE_PARSER                                                                       \
    LEXER_FUNCTION;                                                                          \
                                                                                             \
    int LEXER_NAME(lex_init)(void **scanner);                                                \
    int LEXER_NAME(lex_destroy)(void *yyscanner);                                            \
    struct yy_buffer_state *LEXER_NAME(_scan_string)(const char *yy_str, void *yyscanner);   \
                                                                                             \
    inline void THIS_PARSER::parser::error(const location_type &loc, const std::string &msg) \
    {                                                                                        \
    }                                                                                        \
                                                                                             \
    struct MY_PARSER : some_parser<THIS_PARSER::parser>                                      \
    {                                                                                        \
        using base = THIS_PARSER::parser;                                                    \
                                                                                             \
        int lex(PARSER_NAME_UP(STYPE) * val, location_type *loc)                             \
        {                                                                                    \
            auto ret = LEXER_NAME(lex)(bb.scanner, *loc);                                    \
            if (std::get<1>(ret).has_value())                                                \
                val->v = bb.add_data(std::get<1>(ret));                                      \
            return std::get<0>(ret);                                                         \
        }                                                                                    \
                                                                                             \
        int parse(const std::string &s)                                                      \
        {                                                                                    \
            struct raii                                                                      \
            {                                                                                \
                basic_block &bb;                                                             \
                raii(basic_block &bb) : bb(bb)                                               \
                {                                                                            \
                    LEXER_NAME(lex_init)                                                     \
                    (&bb.scanner);                                                           \
                }                                                                            \
                ~raii()                                                                      \
                {                                                                            \
                    LEXER_NAME(lex_destroy)                                                  \
                    (bb.scanner);                                                            \
                }                                                                            \
            };                                                                               \
                                                                                             \
            bb = basic_block{};                                                              \
            raii holder(bb);                                                                 \
            LEXER_NAME(_scan_string)                                                         \
            (s.c_str(), bb.scanner);                                                         \
            auto r = base::parse();                                                          \
                                                                                             \
            return r;                                                                        \
        }                                                                                    \
    }
