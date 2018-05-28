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
%error-verbose

////////////////////////////////////////

%glr-parser
%skeleton "glr.cc" // c++ skeleton
%define api.value.type {MY_STORAGE}
%define api.prefix {yy_range}

%{
// allows user classes to open their private fields
#define BISON_PARSER 1

#include <primitives/version.h>
using namespace primitives::version;

enum : Version::Number { ANY = -2, UNSET = -1, };

void reset_version(Version &v)
{
    v.major = UNSET;
    v.minor = UNSET;
    v.patch = UNSET;
    v.tweak = UNSET;
}

auto prepare_version(Version &ver, Version::Number val = 0, int level = Version::default_level)
{
    auto l = std::max(ver.level, level);
    auto e = &ver.major;
    for (int i = 0; i < 3; i++)
        *e++ = *e < 0 ? val : *e;
    *e++ = *e < 0 ? (l > 3 ? val : 0) : *e;
    return ver;
}

auto prepare_pair(VersionRange::RangePair &p)
{
    if (p.second < p.first)
        std::swap(p.first, p.second);
    auto level = std::max(p.first.level, p.second.level);
    prepare_version(p.first, 0, level);
    if (p.first < Version::min(level))
        p.first = Version::min(level);
    prepare_version(p.second, Version::maxNumber(), level);
    if (p.second > Version::max(level))
        p.second = Version::max(level);
    return p;
}

%}

////////////////////////////////////////

// tokens and types
%token EOQ 0 "end of file"
%token ERROR_SYMBOL

%token SPACE
%token LT GT LE GE EQ NE
%token AND
%token OR
%token CARET TILDE HYPHEN DOT

%token <v> XNUMBER NUMBER EXTRA

%type <v> file
%type <v> range_set_or range
%type <v> range_set_and
%type <v> compare caret tilde hyphen version
%type <v> basic_version number extra extra_part

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

range: compare | caret | tilde | hyphen
    | version
    {
        EXTRACT_VALUE(Version, v, $1);
        VersionRange::RangePair rp;
        rp.first.patch = 0;
        rp.second.patch = 0;

        // at the moment we do not allow ranges like *.2.* and similar
        // they must be filled only from the beginning

        rp.first = v;
        rp.second.level = v.level;

        if (v.major < 0)
        {
            rp.first = Version::min(v);
            rp.second = Version::max(v);
        }
        else
        {
            if (v.minor < 0)
            {
                rp.second = Version::max(v);
                rp.second.major = v.major;
            }
            else
            {
                if (v.patch < 0)
                {
                    rp.second = Version::max(v);
                    rp.second.major = v.major;
                    rp.second.minor = v.minor;
                }
                else
                {
                    if (v.tweak == ANY)
                    {
                        rp.second = Version::max(v);
                        rp.second.major = v.major;
                        rp.second.minor = v.minor;
                        rp.second.patch = v.patch;
                    }
                    rp.second = rp.first;
                }
            }
        }
        rp.first.extra = v.extra;

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    ;

compare: LT space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange::RangePair rp;
        rp.first = Version::min(v);
        rp.second = prepare_version(v).getPreviousVersion();

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    | LE space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange::RangePair rp;
        rp.first = Version::min(v);
        rp.second = prepare_version(v);

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    | GT space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange::RangePair rp;
        rp.first = prepare_version(v).getNextVersion();
        rp.second = Version::max(v);

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    | GE space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange::RangePair rp;
        rp.first = prepare_version(v);
        rp.second = Version::max(v);

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    | EQ space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange::RangePair rp;
        rp.first = prepare_version(v);
        rp.second = prepare_version(v);

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    | NE space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange::RangePair rp;
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

caret: CARET space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange::RangePair rp;
        rp.first.patch = 0;
        rp.second.patch = 0;

        // at the moment we do not allow ranges like *.2.* and similar
        // they must be filled only from the beginning

        rp.first = v;
        rp.second.level = v.level;

        if (v.major < 0)
            YYABORT;
        else if (v.major > 0)
            rp.second.major = v.major + 1;
        else
        {
            if (v.minor < 0)
            {
                rp.second.major = v.major + 1;
            }
            else if (v.minor > 0)
                rp.second.minor = v.minor + 1;
            else
            {
                if (v.patch < 0)
                {
                    rp.second.major = v.major;
                    rp.second.minor = v.minor + 1;
                }
                else if (v.patch > 0)
                    rp.second.patch = v.patch + 1;
                else
                {
                    if (v.tweak < 0)
                    {
                        rp.second.major = v.major;
                        rp.second.minor = v.minor + 1;
                        rp.second.patch = 0;
                    }
                    else if (v.tweak > 0)
                        rp.second.tweak = v.tweak + 1;
                    else
                    {
                        rp.second.major = v.major;
                        rp.second.minor = v.minor + 1;
                        rp.second.patch = 0;
                        rp.second.tweak = 0;
                    }
                }
            }
        }

        rp.first.extra = v.extra;
        //rp.second.extra = v.extra;
        prepare_version(rp.second, 0, v.level);
        rp.second.decrementVersion();

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    ;

tilde: TILDE space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange::RangePair rp;
        rp.first.patch = 0;
        rp.second.patch = 0;

        // at the moment we do not allow ranges like *.2.* and similar
        // they must be filled only from the beginning

        // unset rp
        rp.first = v;
        rp.second.level = v.level;

        if (v.major < 0)
            YYABORT;
        else
        {
            if (v.minor < 0)
            {
                rp.second.major = v.major + 1;
            }
            else
            {
                if (v.patch < 0)
                {
                    rp.second.major = v.major;
                    rp.second.minor = v.minor + 1;
                }
                else
                {
                    if (v.tweak < 0)
                    {
                        rp.second.major = v.major;
                        rp.second.minor = v.minor + 1;
                        rp.second.patch = 0;
                    }
                    else
                    {
                        rp.second.major = v.major;
                        rp.second.minor = v.minor + 1;
                        rp.second.patch = 0;
                        rp.second.tweak = 0;
                    }
                }
            }
        }
        rp.first.extra = v.extra;
        //rp.second.extra = v.extra;
        rp.second.decrementVersion();

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    ;

hyphen: version SPACE HYPHEN SPACE version
    {
        EXTRACT_VALUE(Version, v1, $1);
        EXTRACT_VALUE(Version, v2, $5);
        VersionRange::RangePair rp;
        rp.first = v1;
        rp.second = v2;

        auto vr = VersionRange::empty();
        vr |= prepare_pair(rp);
        SET_RETURN_VALUE(vr);
    }
    ;

//
// simple types, no much logic
//

version: basic_version
    | basic_version HYPHEN extra
    {
        EXTRACT_VALUE(Version, v, $1);
        EXTRACT_VALUE(Version::Extra, extra, $3);
        v.extra = extra;
        SET_RETURN_VALUE(prepare_version(v));
    }
    ;

basic_version:
      number
    {
        EXTRACT_VALUE(Version::Number, major, $1);
        Version v;
        reset_version(v);
        v.major = major;
        SET_RETURN_VALUE(v);
    }
    | number DOT number
    {
        EXTRACT_VALUE(Version::Number, major, $1);
        EXTRACT_VALUE(Version::Number, minor, $3);
        Version v;
        reset_version(v);
        v.major = major;
        v.minor = minor;
        SET_RETURN_VALUE(v);
    }
    | number DOT number DOT number
    {
        EXTRACT_VALUE(Version::Number, major, $1);
        EXTRACT_VALUE(Version::Number, minor, $3);
        EXTRACT_VALUE(Version::Number, patch, $5);
        Version v;
        reset_version(v);
        v.major = major;
        v.minor = minor;
        v.patch = patch;
        SET_RETURN_VALUE(v);
    }
    | number DOT number DOT number DOT number
    {
        EXTRACT_VALUE(Version::Number, major, $1);
        EXTRACT_VALUE(Version::Number, minor, $3);
        EXTRACT_VALUE(Version::Number, patch, $5);
        EXTRACT_VALUE(Version::Number, tweak, $7);
        Version v;
        reset_version(v);
        v.major = major;
        v.minor = minor;
        v.patch = patch;
        v.tweak = tweak;
        v.level = 4;
        SET_RETURN_VALUE(v);
    }
    ;

number: NUMBER
    | XNUMBER
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
    | NUMBER
    {
        EXTRACT_VALUE(Version::Number, e, $1);
        Version::Extra extra;
        extra.push_back(e);
        SET_RETURN_VALUE(extra);
    }
    ;

or: space_or_empty OR space_or_empty
    ;

and: SPACE
    | space_or_empty AND space_or_empty
    ;

space_or_empty: /* empty */
    | SPACE
    ;

%%
