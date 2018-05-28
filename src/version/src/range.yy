// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

%{
#include <range.h>
%}

////////////////////////////////////////

// general settings
%require "3.0"
%debug
%start file
%locations
%verbose
//%no-lines
%error-verbose

////////////////////////////////////////

%glr-parser
%skeleton "glr.cc" // c++ skeleton
%define api.prefix {yy_range}
%define api.value.type {MY_STORAGE}

%{
#define PRIMITIVES_VERSION_PARSER
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
%type <v> range_set_or range range_as_range
%type <v> range_set_and range_set_and_as_range
%type <v> compare caret tilde hyphen version
%type <v> basic_version number extra

////////////////////////////////////////

%%

file: range_set_or EOQ
    ;

range_set_or: range_set_and_as_range
    | range_set_or or range_set_and_as_range
    {
        EXTRACT_VALUE(VersionRange, vr, $1);
        EXTRACT_VALUE(VersionRange, vr_and, $3);
        vr |= vr_and;
        SET_VALUE($$, vr);
    }
    ;

range_set_and_as_range: range_set_and
    {
        EXTRACT_VALUE(VersionRange::Range, vr_and, $1);
        VersionRange a;
        a.range.clear();
        if (!vr_and.empty())
        {
            a |= vr_and.back();
            vr_and.pop_back();
            for (auto &vr : vr_and)
                a &= vr;
        }
        SET_VALUE($$, a);
    }
    ;

range_set_and: range_as_range
    {
        EXTRACT_VALUE(VersionRange::Range, vr_and, $1);
        SET_VALUE($$, vr_and);
    }
    | range_set_and and range_as_range
    {
        EXTRACT_VALUE(VersionRange::Range, vr_and, $1);
        EXTRACT_VALUE(VersionRange::Range, rp, $3);
        for (auto &r : rp)
            vr_and.push_back(r);
        SET_VALUE($$, vr_and);
    }
    ;

range_as_range: range
    {
        EXTRACT_VALUE(VersionRange::RangePair, rp, $1);
        VersionRange::Range vr_and;
        vr_and.push_back(rp);
        SET_VALUE($$, vr_and);
    }
    ;

range: compare | caret | tilde | hyphen
    | version
    {
        EXTRACT_VALUE(Version, v, $1);
        VersionRange::RangePair rp;
        rp.first = prepare_version(v);
        rp.second = prepare_version(v);
        SET_VALUE($$, prepare_pair(rp));
    }
    ;

compare:
      LT space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange::RangePair rp;
        rp.first = Version::min(v);
        rp.second = prepare_version(v).getPreviousVersion();
        SET_VALUE($$, prepare_pair(rp));
    }
    | LE space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange::RangePair rp;
        rp.first = Version::min(v);
        rp.second = prepare_version(v);
        SET_VALUE($$, prepare_pair(rp));
    }
    | GT space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange::RangePair rp;
        rp.first = prepare_version(v).getNextVersion();
        rp.second = Version::max(v);
        SET_VALUE($$, prepare_pair(rp));
    }
    | GE space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange::RangePair rp;
        rp.first = prepare_version(v);
        rp.second = Version::max(v);
        SET_VALUE($$, prepare_pair(rp));
    }
    | EQ space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange::RangePair rp;
        rp.first = prepare_version(v);
        rp.second = prepare_version(v);
        SET_VALUE($$, prepare_pair(rp));
    }
    | NE space_or_empty version
    {
        EXTRACT_VALUE(Version, v, $3);
        VersionRange::RangePair rp;
        rp.first = Version::min(v);
        rp.second = prepare_version(v).getPreviousVersion();
        /*vr_or_neq |= rp;
        rp.first = prepare_version(v).getNextVersion();
        rp.second = Version::max(v);
        vr_or_neq |= rp;*/
        SET_VALUE($$, prepare_pair(rp));
    }
    ;

caret: CARET space_or_empty version
    { $$ = $3; }
    ;

tilde: TILDE space_or_empty version
    { $$ = $3; }
    ;

hyphen: version SPACE HYPHEN SPACE version
    {
        EXTRACT_VALUE(Version, v1, $1);
        EXTRACT_VALUE(Version, v2, $5);
        if (v1 > v2)
            std::swap(v1, v2);
        VersionRange::RangePair p;
        p.first = v1;
        p.second = v2;
        SET_VALUE($$, prepare_pair(p));
    }
    ;

version: basic_version
    | basic_version HYPHEN extra
    {
        EXTRACT_VALUE(Version, v, $1);
        EXTRACT_VALUE(Version::Extra, extra, $3);
        v.extra = extra;
        SET_VALUE($$, prepare_version(v));
    }
    ;

basic_version:
      number
    {
        EXTRACT_VALUE(Version::Number, major, $1);
        Version v;
        reset_version(v);
        v.major = major;
        SET_VALUE($$, v);
    }
    | number DOT number
    {
        EXTRACT_VALUE(Version::Number, major, $1);
        EXTRACT_VALUE(Version::Number, minor, $3);
        Version v;
        reset_version(v);
        v.major = major;
        v.minor = minor;
        SET_VALUE($$, v);
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
        SET_VALUE($$, v);
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
        SET_VALUE($$, v);
    }
    ;

number: NUMBER
    | XNUMBER
    { SET_VALUE($$, ANY); }
    ;

extra: EXTRA
    | extra DOT EXTRA
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
