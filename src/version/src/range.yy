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
#include <primitives/version.h>
using namespace primitives::version;

enum : Version::Number { ANY = -2, UNSET = -1, };
enum { OPEN, CLOSED, };

VersionRange::RangePair empty_pair()
{
    VersionRange::RangePair rp;
    rp.first.fill(UNSET);
    rp.second.fill(UNSET);
    return rp;
}

auto prepare_version(Version &ver, Version::Number val = 0, Version::Level level = Version::minimum_level)
{
    auto l = std::max(ver.getLevel(), level);
    for (auto &n : ver.numbers)
    {
        if (n < 0)
            n = val;
    }
    if (ver.numbers.size() < l)
        ver.numbers.resize(l, val);
    return ver;
}

auto prepare_pair(VersionRange::RangePair &p)
{
    if (p.isBranch())
        return p;
    if (p.second < p.first)
        std::swap(p.first, p.second);
    auto level = std::max(p.first.getLevel(), p.second.getLevel());
    prepare_version(p.first, 0, level);
    if (p.first < Version::min(level))
    {
        // keep extra for first
        auto e = p.first.extra;
        p.first = Version::min(level);
        p.first.extra = e;
    }
    prepare_version(p.second, Version::maxNumber(), level);
    if (p.second > Version::max(level))
    {
        // but not second?
        p.second = Version::max(level);
    }
    return p;
}

%}

////////////////////////////////////////

// tokens and types
%token EOQ 0 "end of file"
%token ERROR_SYMBOL LL_FATAL_ERROR

%token SPACE
%token LT GT LE GE EQ NE
%token AND COMMA
%token OR
%token CARET TILDE HYPHEN DOT

%token <v> L_SQUARE_BRACKET R_SQUARE_BRACKET L_BRACKET R_BRACKET
%token <v> NUMBER X_NUMBER STAR_NUMBER EXTRA BRANCH

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
    {
        EXTRACT_VALUE(VersionRange, vr, $1);
        auto vr_and = VersionRange::empty();
        vr_and |= vr;
        SET_RETURN_VALUE(vr_and);
    }
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
        auto rp = empty_pair();

        // at the moment we do not allow ranges like *.2.* and similar
        // they must be filled only from the beginning

        rp.first = v;
        rp.first.extra = v.extra;
        rp.second.numbers = v.numbers;

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    ;

caret: CARET space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        auto rp = empty_pair();

        // at the moment we do not allow ranges like *.2.* and similar
        // they must be filled only from the beginning

        if (v.numbers[0] < 0)
            YYABORT;

        rp.first = v;
        rp.first.extra = v.extra;
        rp.second.numbers = v.numbers;

        bool set = false;
        for (auto &n : rp.second.numbers)
        {
            if (n > 0)
            {
                n++;
                std::fill(&n + 1, &*(rp.second.numbers.end() - 1) + 1, 0);
                set = true;
                break;
            }
        }
        if (!set)
        {
            for (auto i = rp.second.numbers.rbegin(); i != rp.second.numbers.rend(); i++)
            {
                if (*i == 0)
                {
                    (*i)++;
                    std::fill(i.base() + 1, rp.second.numbers.end(), 0);
                    break;
                }
            }
        }

        prepare_version(rp.second);
        rp.second.decrementVersion();

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    ;

tilde: TILDE space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        auto rp = empty_pair();

        // at the moment we do not allow ranges like *.2.* and similar
        // they must be filled only from the beginning

        if (v.numbers[0] < 0)
            YYABORT;

        rp.first = v;
        rp.first.extra = v.extra;
        rp.second.numbers = v.numbers;

        int p = v.numbers.size() > 2 && v.numbers[1] >= 0;
        rp.second.numbers[p]++;
        std::fill(rp.second.numbers.begin() + p + 1, rp.second.numbers.end(), 0);

        prepare_version(rp.second);
        rp.second.decrementVersion();

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    ;

hyphen: version SPACE HYPHEN space_or_empty version
    {
        EXTRACT_VALUE(Version, v1, $1);
        EXTRACT_VALUE(Version, v2, $5);
        auto rp = empty_pair();
        rp.first = v1;
        rp.second = v2;

        // set right side xversion to zeros
        // npm does not use this
        // yarn does

        // If you think about this with ABI in mind, you'll see
        // that 1 - 2 === 1.*.* - 2.*.* expansion is correct.
        // You say to use from 1 to 2 major abi versions inclusive.
        // If we expand 1.0.0 - 2.0.0 it means we use first *in theory* abi broken major version 2.0.0
        // but do not use others.
        // Or if you think that abi is not broken in 2.0.0 and broken in 2.0.1 - that's not how semver works.

        //auto level = std::max(rp.first.getLevel(), rp.second.getLevel());
        //prepare_version(rp.second, 0, level);

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    ;

interval: open_bracket space_or_empty version space_or_empty COMMA space_or_empty version space_or_empty close_bracket
    {
        EXTRACT_VALUE(int, l, $1);
        EXTRACT_VALUE(int, r, $9);
        EXTRACT_VALUE(Version, v1, $3);
        EXTRACT_VALUE(Version, v2, $7);
        auto rp = empty_pair();
        rp.first = v1;
        rp.second = v2;

        auto level = std::max(rp.first.getLevel(), rp.second.getLevel());
        prepare_version(rp.second, 0, level);
        prepare_pair(rp);

        if (l == OPEN)
            rp.first.incrementVersion();
        if (r == OPEN)
            rp.second.decrementVersion();

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    | open_bracket space_or_empty version space_or_empty COMMA space_or_empty close_bracket
    {
        EXTRACT_VALUE(int, l, $1);
        EXTRACT_VALUE(int, r, $7);
        EXTRACT_VALUE(Version, v, $3);
        auto rp = empty_pair();
        rp.first = v;
        rp.second = Version::max(v);
        prepare_version(rp.first);

        if (l == OPEN)
            rp.first.incrementVersion();

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    | open_bracket space_or_empty COMMA space_or_empty version space_or_empty close_bracket
    {
        EXTRACT_VALUE(int, l, $1);
        EXTRACT_VALUE(int, r, $7);
        EXTRACT_VALUE(Version, v, $5);
        auto rp = empty_pair();
        rp.first = Version::min(v);
        rp.second = v;
        prepare_version(rp.second);

        if (r == OPEN)
            rp.second.decrementVersion();

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    ;

compare: LT space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        auto rp = empty_pair();
        rp.first = Version::min(v);
        rp.second = prepare_version(v).getPreviousVersion();

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    | LE space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        auto rp = empty_pair();
        rp.first = Version::min(v);
        rp.second = prepare_version(v);

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    | GT space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        auto rp = empty_pair();
        rp.first = prepare_version(v).getNextVersion();
        rp.second = Version::max(v);

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    | GE space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        auto rp = empty_pair();
        rp.first = prepare_version(v);
        rp.second = Version::max(v);

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    | EQ space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        auto rp = empty_pair();
        rp.first = prepare_version(v);
        rp.second = prepare_version(v);

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    | NE space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        auto rp = empty_pair();
        rp.first = Version::min(v);
        rp.second = prepare_version(v).getPreviousVersion();

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        rp.first = prepare_version(v).getNextVersion();
        rp.second = Version::max(v);
        vr |= prepare_pair(rp);

        SET_RETURN_VALUE(vr);
    }
    ;

////////////////////////////////////////////////////////////////////////////////
// simple types, no much logic
////////////////////////////////////////////////////////////////////////////////

version: basic_version
    {
        EXTRACT_VALUE(GenericNumericVersion::Numbers, numbers, $1);
        if (numbers.size() < Version::minimum_level)
            numbers.resize(Version::minimum_level, UNSET);
        Version v;
        v.numbers = numbers;
        SET_RETURN_VALUE(v);
    }
    | basic_version HYPHEN extra
    {
        EXTRACT_VALUE(GenericNumericVersion::Numbers, numbers, $1);
        EXTRACT_VALUE(Version::Extra, extra, $3);
        if (numbers.size() < Version::minimum_level)
            numbers.resize(Version::minimum_level, UNSET);
        Version v;
        v.numbers = numbers;
        v.extra = extra;
        SET_RETURN_VALUE(v);
    }
    | BRANCH
    {
        EXTRACT_VALUE(std::string, branch, $1);
        Version v;
        v.branch = branch;
        SET_RETURN_VALUE(v);
    }
    ;

basic_version: number
    {
        EXTRACT_VALUE(Version::Number, major, $1);
        GenericNumericVersion::Numbers numbers;
        numbers.push_back(major);
        SET_RETURN_VALUE(numbers);
    }
    | basic_version DOT number
    {
        EXTRACT_VALUE(GenericNumericVersion::Numbers, numbers, $1);
        EXTRACT_VALUE(Version::Number, n, $3);
        numbers.push_back(n);
        SET_RETURN_VALUE(numbers);
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
        extra.insert(extra.end(), e.begin(), e.end());
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
    | BRANCH
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
