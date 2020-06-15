// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

////////////////////////////////////////////////////////////////////////////////
//
// Protected
//
////////////////////////////////////////////////////////////////////////////////

// manual guard
#ifndef PRIMITIVES_HELPER_BISON
#define PRIMITIVES_HELPER_BISON

#ifdef _MSC_VER
#pragma warning(disable : 4005)
#pragma warning(disable : 4065)
#endif

#include <primitives/exceptions.h>
#include <primitives/preprocessor.h>
#include <optional>

#include <sstream>
#include <string>
#include <tuple>

#ifdef __GNUG__
#include <cstdlib>
#include <memory>
#include <cxxabi.h>

inline std::string demangle(const char* name) {

    int status = -4; // some arbitrary value to eliminate the compiler warning

    // enable c++11 by passing the flag -std=c++11 to g++
    std::unique_ptr<char, void(*)(void*)> res{
        abi::__cxa_demangle(name, NULL, NULL, &status),
        std::free
    };

    return (status == 0) ? res.get() : name;
}

#else

// does nothing if not g++
inline std::string demangle(const char* name) {
    return name;
}
#endif

namespace primitives::bison
{

struct basic_block
{
    void *scanner = nullptr;
    bool can_throw = false;
    std::string error_msg;
    std::string bad_symbol;
};

}

// lexer
#define PUSH_STATE(x) BEGIN(x)
#define POP_STATE() BEGIN(0)

////////////////////////////////////////////////////////////////////////////////
// GLR parser in C++ with custom storage based on std::any
////////////////////////////////////////////////////////////////////////////////

// glr
#ifdef GLR_CPP_PARSER

#include <any>
#include <list>

namespace primitives::bison
{

struct basic_block1 : basic_block
{
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
    std::optional<T> getResult() const
    {
        if (data.empty())
            return {};
        return std::any_cast<T>(data.back());
    }
};

struct any_storage
{
    std::any *v;

    template <class U>
    bool checkType(basic_block1 &bb) const
    {
        auto &t1 = v->type();
        auto &t2 = typeid(U);
        if (t1 != t2)
            bb.error_msg = std::string("Incorrect type: ") + demangle(t1.name()) + ", wanted type: " + demangle(t2.name());
        return t1 == t2;
    }

    template <class U>
    bool hasType(basic_block1 &bb) const
    {
        return checkType<U>(bb);
    }

    template <class U>
    void setValue(basic_block1 &bb, const U &v2)
    {
        v = bb.add_data(v2);
    }

    template <class U>
    U getValue() const
    {
        return std::any_cast<U>(*v);
    }
};

template <class BaseParser>
struct base_parser : BaseParser
{
    using basic_block_type = basic_block1;

    basic_block_type bb;
};

}

#endif // GLR_CPP_PARSER

////////////////////////////////////////////////////////////////////////////////
// LALR(1) parser in C++ with bison variants
////////////////////////////////////////////////////////////////////////////////

// lalr1
#ifdef LALR1_CPP_VARIANT_PARSER

// empty

#endif // LALR1_CPP_VARIANT_PARSER

#endif // PRIMITIVES_HELPER_BISON

////////////////////////////////////////////////////////////////////////////////
//
// Unprotected
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Names
////////////////////////////////////////////////////////////////////////////////

// lexer
#define THIS_LEXER CONCATENATE(ll_, THIS_LEXER_NAME)
#define THIS_LEXER_UP CONCATENATE(LL_, THIS_LEXER_NAME_UP)
#define LEXER_NAME(v) CONCATENATE(THIS_LEXER, v)
#define LEXER_NAME_UP(v) CONCATENATE(THIS_LEXER_UP, v)

// parser
#define THIS_PARSER CONCATENATE(yy_, THIS_PARSER_NAME)
#define THIS_PARSER_UP CONCATENATE(YY_, THIS_PARSER_NAME_UP)
#define PARSER_NAME(v) CONCATENATE(THIS_PARSER, v)
#define PARSER_NAME_UP(v) CONCATENATE(THIS_PARSER_UP, v)

////////////////////////////////////////////////////////////////////////////////
// Global setup
////////////////////////////////////////////////////////////////////////////////

// lexer
#undef yylex

#define YY_DECL LEXER_FUNCTION
#define YY_USER_ACTION loc.step(); loc.columns(yyleng);

#if YY_DEBUG_STATE
#define YY_DEBUG_LEXER_STATE(x, msg) YY_PRINT_STATE(x, msg)
#else
#define YY_DEBUG_LEXER_STATE(x, msg)
#endif

#define YY_PRINT_STATE(x, msg)                            \
    do                                                    \
    {                                                     \
        printf(msg " state: %d, current stack = ", (x));  \
        for (int i = 0; i < yyg->yy_start_stack_ptr; i++) \
            printf("%d ", yyg->yy_start_stack[i]);        \
    } while (0)
#define YY_PUSH_STATE(x) do { yy_push_state((x), yyscanner); YY_DEBUG_LEXER_STATE(x, "entering"); } while (0)
#define YY_POP_STATE() do { yy_pop_state(yyscanner); YY_DEBUG_LEXER_STATE(YYSTATE, "entering"); } while (0)
#define YY_TOP_STATE() (((struct yyguts_t*)yyscanner)->yy_start_stack ? yy_top_state(yyscanner) : YYSTATE)

// parser

#define THIS_PARSER2 THIS_PARSER::parser

// bison is missing this definition
#define BISON_PARSER_ERROR_FUNCTION                                                                                    \
    inline void THIS_PARSER2::error(const location_type &loc, const std::string &msg)                                  \
    {                                                                                                                  \
        std::ostringstream ss;                                                                                         \
        ss << loc << " " << msg;                                                                                       \
        auto self = (primitives::bison::BASE_PARSER_NAME<THIS_PARSER2> *)(this);                                       \
        self->bb.error_msg = ss.str();                                                                                 \
        if (!self->bb.bad_symbol.empty())                                                                              \
            self->bb.error_msg += ", bad symbol = '" + self->bb.bad_symbol + "'";                                      \
        self->bb.error_msg += "\n";                                                                                    \
        if (self->bb.can_throw)                                                                                        \
            throw SW_RUNTIME_ERROR("Error during parse: " + ss.str());                                                 \
    }

// debug level
#define SET_DEBUG_LEVEL

#define MY_PARSER_DECLARATIONS                                                                                         \
    LEXER_FUNCTION;                                                                                                    \
    BISON_PARSER_ERROR_FUNCTION                                                                                        \
                                                                                                                       \
    int LEXER_NAME(lex_init)(void **scanner);                                                                          \
    int LEXER_NAME(lex_destroy)(void *yyscanner);                                                                      \
    struct yy_buffer_state *LEXER_NAME(_scan_string)(const char *yy_str, void *yyscanner);                             \
    void LEXER_NAME(set_in)(FILE * f, void *yyscanner)

#define MY_PARSER_PARSE_FUNCTION_PROLOGUE                                                                              \
    bb = basic_block_type{};                                                                                           \
    raii holder(bb)

#define MY_PARSER_PARSE_FUNCTION_EPILOGUE                                                                              \
    SET_DEBUG_LEVEL;                                                                                                   \
    return THIS_PARSER2::parse()

#define MY_PARSER_PARSE_FUNCTION                                                                                       \
    int parse(const std::string &s)                                                                                    \
    {                                                                                                                  \
        MY_PARSER_PARSE_FUNCTION_PROLOGUE;                                                                             \
        LEXER_NAME(_scan_string)(s.c_str(), bb.scanner);                                                               \
        MY_PARSER_PARSE_FUNCTION_EPILOGUE;                                                                             \
    }                                                                                                                  \
                                                                                                                       \
    int parse(FILE *f)                                                                                                 \
    {                                                                                                                  \
        MY_PARSER_PARSE_FUNCTION_PROLOGUE;                                                                             \
        LEXER_NAME(set_in)(f, bb.scanner);                                                                             \
        MY_PARSER_PARSE_FUNCTION_EPILOGUE;                                                                             \
    }

#define DECLARE_PARSER_BASE                                                                                            \
    MY_PARSER_DECLARATIONS;                                                                                            \
                                                                                                                       \
    struct MY_PARSER : primitives::bison::BASE_PARSER_NAME<THIS_PARSER2>                                               \
    {                                                                                                                  \
        struct raii                                                                                                    \
        {                                                                                                              \
            basic_block_type &bb;                                                                                      \
            raii(basic_block_type &bb) : bb(bb)                                                                        \
            {                                                                                                          \
                LEXER_NAME(lex_init)                                                                                   \
                (&bb.scanner);                                                                                         \
            }                                                                                                          \
            ~raii()                                                                                                    \
            {                                                                                                          \
                LEXER_NAME(lex_destroy)                                                                                \
                (bb.scanner);                                                                                          \
            }                                                                                                          \
        }

#define DECLARE_PARSER                                                                                                 \
    DECLARE_PARSER_BASE;                                                                                               \
                                                                                                                       \
    MY_PARSER_PARSE_FUNCTION                                                                                           \
    MY_LEXER_FUNCTION                                                                                                  \
    }

////////////////////////////////////////////////////////////////////////////////
// GLR parser in C++ with custom storage based on std::any
////////////////////////////////////////////////////////////////////////////////

// glr
#ifdef GLR_CPP_PARSER

#include <any>
#include <list>

// parser
#define yylex(val, loc) MY_PARSER_CAST.lex(val, loc)

#define BASE_PARSER_NAME base_parser

#define MY_PARSER_CAST ((MY_PARSER &)yyparser)
#define MY_PARSER_CAST_BB (((MY_PARSER &)yyparser).bb)

#define MY_STORAGE primitives::bison::any_storage

// yy macros
#define GET_STORAGE(v) ((MY_STORAGE *)&v)

#define CHECK_TYPE(type, bison_var)                                \
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
#define LEXER_RETURN_VALUE std::tuple<int, std::any>
#define LEXER_FUNCTION LEXER_RETURN_VALUE LEXER_NAME(lex)(void *yyscanner, THIS_PARSER2::location_type &loc)
#define MAKE(x) {THIS_PARSER2::token::x, {}}
#define MAKE_VALUE(x, v) {THIS_PARSER2::token::x, v}

#define MY_LEXER_FUNCTION_SIGNATURE_RETURN_VALUE int
#define MY_LEXER_FUNCTION_SIGNATURE_BODY lex(THIS_PARSER2::semantic_type * val, THIS_PARSER2::location_type * loc)
#define MY_LEXER_FUNCTION_SIGNATURE MY_LEXER_FUNCTION_SIGNATURE_RETURN_VALUE MY_LEXER_FUNCTION_SIGNATURE_BODY
#define MY_LEXER_FUNCTION_SIGNATURE_CLASS(x) MY_LEXER_FUNCTION_SIGNATURE_RETURN_VALUE x::MY_LEXER_FUNCTION_SIGNATURE_BODY

#define MY_LEXER_FUNCTION_EPILOGUE                                                                                     \
    if (std::get<0>(ret) == THIS_PARSER2::token::ERROR_SYMBOL && std::get<1>(ret).has_value())                         \
        bb.bad_symbol = std::any_cast<std::string>(std::get<1>(ret));                                                  \
    if (std::get<1>(ret).has_value())                                                                                  \
        val->v = bb.add_data(std::get<1>(ret));                                                                        \
    return std::get<0>(ret)

#define MY_LEXER_FUNCTION                                                                                              \
    MY_LEXER_FUNCTION_SIGNATURE                                                                                        \
    {                                                                                                                  \
        auto ret = LEXER_NAME(lex)(bb.scanner, *loc);                                                                  \
        MY_LEXER_FUNCTION_EPILOGUE;                                                                                    \
    }

#endif // GLR_CPP_PARSER

////////////////////////////////////////////////////////////////////////////////
// LALR(1) parser in C++ with bison variants
////////////////////////////////////////////////////////////////////////////////

// lalr1
#ifdef LALR1_CPP_VARIANT_PARSER

// parser
#define yylex() ((MY_PARSER_DRIVER*)this)->lex()

// lexer
#define LEXER_RETURN_VALUE THIS_PARSER2::symbol_type
#define LEXER_FUNCTION LEXER_RETURN_VALUE LEXER_NAME(lex)(void *yyscanner, THIS_PARSER2::location_type &loc, MY_PARSER_DRIVER &driver)
#define MAKE(x) THIS_PARSER2::make_ ## x(loc)
#define MAKE_VALUE(x, v) THIS_PARSER2::make_ ## x((v), loc)

#define YY_FATAL_ERROR(msg) throw std::logic_error(msg)

#define MY_PARSER_DRIVER CONCATENATE(MY_PARSER, Driver)
struct MY_PARSER_DRIVER;

#define BASE_PARSER_NAME CONCATENATE(base_parser_, MY_PARSER)

namespace primitives::bison
{

template <class BaseParser>
struct BASE_PARSER_NAME : BaseParser
{
    typename BaseParser::location_type loc;
    using basic_block_type = basic_block;

    basic_block_type bb;

    BASE_PARSER_NAME() : BaseParser((MY_PARSER_DRIVER&)*this) {}
};

}

#define MY_LEXER_FUNCTION                                                   \
    LEXER_RETURN_VALUE lex()                                                \
    {                                                                       \
        return LEXER_NAME(lex)(bb.scanner, loc, (MY_PARSER_DRIVER &)*this); \
    }

#endif // LALR1_CPP_VARIANT_PARSER

////////////////////////////////////////////////////////////////////////////////
//
// Bison settings examples
//
////////////////////////////////////////////////////////////////////////////////

/*
// GLR parser in C++ with custom storage based on std::any

%glr-parser
%skeleton "glr.cc" // C++ skeleton
%define api.value.type { MY_STORAGE }
%code provides { #include <primitives/helper/bison_yy.h> }

*/

/*
// LALR(1) parser in C++ with bison variants

%skeleton "lalr1.cc" // C++ skeleton
%define api.value.type variant
%define api.token.constructor // C++ style of handling variants
%define parse.assert // check C++ variant types
%code provides { #include <primitives/helper/bison_yy.h> }
%parse-param { MY_PARSER_DRIVER &driver } // param to yy::parser() constructor (the parsing context)

*/

/*
// Other useful stuff.

// prevent duplication of locations (location.hh and position.hh)
// set name of other parser   vvvvvvvv
%define api.location.type { yy_settings::location }
%code requires { #include <location.hh> }

// LALR(1): driver code
%code provides
{

struct MY_PARSER_DRIVER : MY_PARSER
{
    int result;
    // ...
};

}

*/
