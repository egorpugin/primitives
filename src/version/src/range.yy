// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

%{
#include <range_parser.h>
%}

////////////////////////////////////////

// general settings
%require "3.0"
//%debug
%start file
%locations
%verbose
//%no-lines
%define parse.error verbose

////////////////////////////////////////

%glr-parser
%skeleton "glr.cc" // C++ skeleton
%define api.value.type { MY_STORAGE }
%code provides { #include <primitives/helper/bison_yy.h> }

%expect 3

%{
#include <primitives/version_range.h>
using namespace primitives::version;

enum : Version::Number { ANY = -2, UNSET = -1, };
enum { OPEN, CLOSED, };

using primitives::version::detail::RangePair;

%}

////////////////////////////////////////

// tokens and types
%token EOQ 0 "end of file"
%token ERROR_SYMBOL

%token SPACE
%token LT GT LE GE EQ NE
%token AND COMMA
%token OR
%token CARET TILDE HYPHEN DOT

%token <v> L_SQUARE_BRACKET R_SQUARE_BRACKET L_BRACKET R_BRACKET
%token <v> NUMBER X_NUMBER STAR_NUMBER EXTRA STRING

%type <v> file
%type <v> range_set_or range
%type <v> range_set_and
%type <v> compare caret tilde hyphen version interval
%type <v> basic_version number extra extra_part
%type <v> open_bracket close_bracket

////////////////////////////////////////

%%

file: range_set_or EOQ
    {
        SET_PARSER_RESULT($1);

// in-rule definition to convert $$
#define SET_RETURN_VALUE(xxxxx) SET_VALUE($$, xxxxx)
    }
    ;

range_set_or: range_set_and
    | range_set_or or range_set_and
    {
        EXTRACT_VALUE(VersionRange, vr, $1);
        EXTRACT_VALUE(VersionRange, vr_and, $3);
        vr |= vr_and;
        SET_RETURN_VALUE(vr);
    }
    ;

range_set_and: range
    | range_set_and and range
    {
        EXTRACT_VALUE(VersionRange, vr_and, $1);
        EXTRACT_VALUE(VersionRange, vr, $3);
        vr_and &= vr;
        SET_RETURN_VALUE(vr_and);
    }
    ;

////////////////////////////////////////////////////////////////////////////////
// single ranges
////////////////////////////////////////////////////////////////////////////////

range: compare | caret | tilde | hyphen | interval
    | version
    {
        EXTRACT_VALUE(Version, v, $1);

        if (v.numbers.empty())
        {
            // all star
            RangePair rp(Version::min(), true, Version::max(), true);
            VersionRange vr;
            vr |= rp;
            SET_RETURN_VALUE(vr);
        }
        else
        {
            // construct without extra part
            Version v2;
            v2.numbers = v.numbers;
            v2++;

            RangePair rp(v, false, v2, true);
            VersionRange vr;
            vr |= rp;
            SET_RETURN_VALUE(vr);
        }
    }
    ;

caret: CARET space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);

        // construct without extra part
        Version v2;
        v2.numbers = v.numbers;

        if (v2 > 0)
        {
            auto sz = v2.getLevel();
            for (int i = 0; i < sz; i++)
            {
                auto &n = v2.numbers[i];
                if (!n)
                    continue;
                v2.numbers.resize(i + 1);
                break;
            }
        }

        RangePair rp(v, false, ++v2, true);
        VersionRange vr;
        vr |= rp;
        SET_RETURN_VALUE(vr);
    }
    ;

tilde: TILDE space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);

        // construct without extra part
        Version v2;
        v2.numbers = v.numbers;
        if (v2.getLevel() > 2)
            v2.numbers.resize(2);
        v2++;

        RangePair rp(v, false, v2, true);

        VersionRange vr;
        vr |= rp;
        SET_RETURN_VALUE(vr);
    }
    ;

hyphen: version SPACE HYPHEN space_or_empty version
    {
        EXTRACT_VALUE(Version, v1, $1);
        EXTRACT_VALUE(Version, v2, $5);
        RangePair rp(v1, false, ++v2, true);

        // set right side xversion to zeros
        // npm does not use this
        // yarn does

        // If you think about this with ABI in mind, you'll see
        // that 1 - 2 === 1.*.* - 2.*.* expansion is correct.
        // You say to use from 1 to 2 major abi versions inclusive.
        // If we expand 1.0.0 - 2.0.0 it means we use first *in theory* abi broken major version 2.0.0
        // but do not use others.
        // Or if you think that abi is not broken in 2.0.0 and broken in 2.0.1 - that's not how semver works.

        VersionRange vr;
        vr |= rp;
        SET_RETURN_VALUE(vr);
    }
    ;

interval: open_bracket space_or_empty version space_or_empty COMMA space_or_empty version space_or_empty close_bracket
    {
        EXTRACT_VALUE(int, l, $1);
        EXTRACT_VALUE(int, r, $9);
        EXTRACT_VALUE(Version, v1, $3);
        EXTRACT_VALUE(Version, v2, $7);
        VersionRange vr;
        vr |= RangePair(v1, l == OPEN, v2, r == OPEN);
        SET_RETURN_VALUE(vr);
    }
    | open_bracket space_or_empty version space_or_empty COMMA space_or_empty close_bracket
    {
        EXTRACT_VALUE(int, l, $1);
        EXTRACT_VALUE(int, r, $7);
        EXTRACT_VALUE(Version, v, $3);
        VersionRange vr;
        vr |= RangePair(v, l == OPEN, Version::max(), true);
        SET_RETURN_VALUE(vr);
    }
    | open_bracket space_or_empty COMMA space_or_empty version space_or_empty close_bracket
    {
        EXTRACT_VALUE(int, l, $1);
        EXTRACT_VALUE(int, r, $7);
        EXTRACT_VALUE(Version, v, $5);
        VersionRange vr;
        vr |= RangePair(Version::min(), true, v, r == OPEN);
        SET_RETURN_VALUE(vr);
    }
    ;

compare: LT space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange vr;
        vr |= RangePair(0, true, v, true);
        SET_RETURN_VALUE(vr);
    }
    | LE space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange vr;
        vr |= RangePair(0, true, v, false);
        SET_RETURN_VALUE(vr);
    }
    | GT space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange vr;
        vr |= RangePair(v, true, Version::maxNumber() + 1, true);
        SET_RETURN_VALUE(vr);
    }
    | GE space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange vr;
        vr |= RangePair(v, false, Version::maxNumber() + 1, true);
        SET_RETURN_VALUE(vr);
    }
    | EQ space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange vr;
        vr |= RangePair(v, false, v, false);
        SET_RETURN_VALUE(vr);
    }
    | NE space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange vr;
        vr |= RangePair(Version::min(), true, v, true);
        vr |= RangePair(v, true, Version::max(), true);
        SET_RETURN_VALUE(vr);
    }
    ;

////////////////////////////////////////////////////////////////////////////////
// simple types, no much logic
////////////////////////////////////////////////////////////////////////////////

version: basic_version
    | basic_version HYPHEN extra
    {
        EXTRACT_VALUE(Version, v, $1);
        EXTRACT_VALUE(Version::Extra, extra, $3);
        if (v == Version::min())
            throw SW_RUNTIME_ERROR("Cannot set extra part on zero version");
        v.extra = extra;
        SET_RETURN_VALUE(v);
    }
    ;

// at the moment we do not allow ranges like *.2.* and similar
// they must be filled only from the beginning
basic_version: number
    {
        EXTRACT_VALUE(Version::Number, n, $1);
        Version v;
        if (n != ANY)
            v.push_back(n);
        SET_RETURN_VALUE(v);
    }
    | basic_version DOT number
    {
        EXTRACT_VALUE(Version, v, $1);
        EXTRACT_VALUE(Version::Number, n, $3);
        if (n != ANY)
            v.push_back(n);
        SET_RETURN_VALUE(v);
    }
    ;

number: NUMBER
    | X_NUMBER
    { SET_RETURN_VALUE((Version::Number)ANY); }
    | STAR_NUMBER
    { SET_RETURN_VALUE((Version::Number)ANY); }
    ;

extra: extra_part
    | extra DOT extra_part
    {
        EXTRACT_VALUE(Version::Extra, extra, $1);
        EXTRACT_VALUE(Version::Extra, e, $3);
        extra.append(e);
        SET_RETURN_VALUE(extra);
    }
    ;

extra_part: EXTRA
    {
        EXTRACT_VALUE(std::string, e, $1);
        Version::Extra extra;
        extra.push_back(e);
        SET_RETURN_VALUE(extra);
    }
    | STRING
    {
        EXTRACT_VALUE(std::string, e, $1);
        Version::Extra extra;
        extra.push_back(e);
        SET_RETURN_VALUE(extra);
    }
    | X_NUMBER
    {
        EXTRACT_VALUE(std::string, e, $1);
        Version::Extra extra;
        extra.push_back(e);
        SET_RETURN_VALUE(extra);
    }
    | NUMBER
    {
        EXTRACT_VALUE(Version::Number, e, $1);
        Version::Extra extra;
        extra.push_back(e);
        SET_RETURN_VALUE(extra);
    }
    ;

////////////////////////////////////////////////////////////////////////////////
// logical and stuff
////////////////////////////////////////////////////////////////////////////////

or: space_or_empty OR space_or_empty
    ;

and: SPACE
    | space_or_empty and_token space_or_empty
    ;

and_token: AND
    | COMMA
    ;

space_or_empty: /* empty */
    | SPACE
    ;

open_bracket: L_SQUARE_BRACKET
    { SET_RETURN_VALUE((int)CLOSED); }
    | L_BRACKET
    { SET_RETURN_VALUE((int)OPEN); }
    ;

close_bracket: R_SQUARE_BRACKET
    { SET_RETURN_VALUE((int)CLOSED); }
    | R_BRACKET
    { SET_RETURN_VALUE((int)OPEN); }
    ;

%%
