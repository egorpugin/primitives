// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/version.h>

#include <algorithm>

namespace primitives::version
{

bool Version::parse(Version &v, const std::string &s)
{
    bool ok = false;
	int cs = 0;
	auto p = s.data();
    auto pe = p + s.size();
    auto eof = pe;

    Version::Number n; // number
    auto sb = p; // string begin

    auto part = &v.major;

    %%{
        machine Version;
	    write data;

        action SB { sb = p; }
        action OK { ok = p == pe; }

        alpha_ = alpha | '_';
        alnum_ = alnum | '_';

        number = digit+ >SB %{ n = std::stoll(std::string{sb, p}); };
        number_version_part = number %{ *part++ = n; };
        #basic_version = [=]? [v]? number_version_part ('.' number_version_part){,3};
        basic_version = number_version_part ('.' number_version_part){,3};
        extra_part = alnum_+ >SB %{ v.extra.emplace_back(sb, p); };
        extra = extra_part ('.' extra_part)*;
        version = basic_version ('-' extra)?;

        branch = (alpha_ (alnum_ | '-')*) >SB %{ v.branch.assign(sb, p); };

        main := (version | branch) %OK;

		write init;
		write exec;
    }%%

    if (v.major == 0 && v.minor == 0 && v.patch == 0 && v.tweak == 0)
    {
        if (part != &v.tweak)
            v.patch = 1;
        else
        {
            v.tweak = 1;
            v.level = 4;
        }
    }

    return ok;
}

bool Version::parseExtra(Version &v, const std::string &s)
{
    bool ok = false;
	int cs = 0;
	auto p = s.data();
    auto pe = p + s.size();
    auto eof = pe;

    auto sb = p; // string begin

    %%{
        machine VersionExtra;
	    write data;

        action SB { sb = p; }
        action OK { ok = p == pe; }

        alnum_ = alnum | '_';

        extra_part = alnum_+ >SB %{ v.extra.emplace_back(sb, p); };
        extra = extra_part ('.' extra_part)*;

        main := extra %OK;

		write init;
		write exec;
    }%%

    return ok;
}

bool VersionRange::parse1(VersionRange &vr, const std::string &ts)
{
    bool ok = false;
	int cs = 0;
	auto p = ts.data();
    auto pe = p + ts.size();
    auto eof = pe;

    Version::Number n; // number
    auto sb = p; // string begin

    enum { ANY = -2, UNSET = -1, };

    VersionRange::RangePair rp;
    Version v;
    Version::Number *part;
    VersionRange::Range vr_and;
    VersionRange vr_or_neq;

    enum { LT, GT, LE, GE, EQ, NE, } sign;

    auto prepare_version = [](auto &ver, Version::Number val = 0, int level = Version::default_level)
    {
        auto e = &ver.major;
        for (int i = 0; i < 3; i++)
            *e++ = *e < 0 ? val : *e;
        *e++ = *e < 0 ? (level > 3 ? val : 0) : *e;
        return ver;
    };

    auto prepare_pair = [&prepare_version](auto &p)
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
    };

/*#define CHECK_IF_NOT_RANGE(v) \
    if ((v).major == ANY || (v).minor == ANY || (v).patch == ANY || (v).tweak == ANY) \
        return false*/

    %%{
        machine VersionRange;
		write data;

        action SB { sb = p; }
        action OK { ok = p == pe; }

        action RESET_VER
        {
            v.major = UNSET;
            v.minor = UNSET;
            v.patch = UNSET;
            v.tweak = UNSET;
            v.extra.clear();
            part = &v.major;
        }

        action SET_VER_LEVEL
        {
            if (v.tweak != UNSET)
                v.level = 4;
        }

        action RESET_PAIR
        {
            rp = VersionRange::RangePair();
            rp.first.patch = 0;
            rp.second.patch = 0;
            sign = LT;
            v.level = Version::default_level;
        }

        action RANGE_CMP
        {
            //CHECK_IF_NOT_RANGE(v);

            switch (sign)
            {
            case LT:
                rp.first = Version::min(v);
                rp.second = prepare_version(v).getPreviousVersion();
                break;
            case LE:
                rp.first = Version::min(v);
                rp.second = prepare_version(v);
                break;
            case GT:
                rp.first = prepare_version(v).getNextVersion();
                rp.second = Version::max(v);
                break;
            case GE:
                rp.first = prepare_version(v);
                rp.second = Version::max(v);
                break;
            case EQ:
                rp.first = prepare_version(v);
                rp.second = prepare_version(v);
                break;
            case NE:
                rp.first = Version::min(v);
                rp.second = prepare_version(v).getPreviousVersion();
                vr_or_neq |= rp;
                rp.first = prepare_version(v).getNextVersion();
                rp.second = Version::max(v);
                vr_or_neq |= rp;
                break;
            }
        }

        action RANGE_VER
        {
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
            //rp.second.extra = v.extra;
        }

        action TILDE_VER
        {
            // at the moment we do not allow ranges like *.2.* and similar
            // they must be filled only from the beginning

            rp.first = v;
            rp.second.level = v.level;

            if (v.major < 0)
                return false;
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
        }

        action CARET_VER
        {
            // at the moment we do not allow ranges like *.2.* and similar
            // they must be filled only from the beginning

            rp.first = v;
            rp.second.level = v.level;

            if (v.major < 0)
                return false;
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
        }

        action ADD_RANGE
        {
            VersionRange a;
            a.range.clear();
            if (!vr_and.empty())
            {
                a |= vr_and.back();
                vr_and.pop_back();
                for (auto &vr : vr_and)
                    a &= vr;
            }
            VersionRange r;
            r.range.clear();
            r |= a;
            r |= vr_or_neq;
            vr |= r;
        }

        alpha_ = alpha | '_';
        alnum_ = alnum | '_';
        logical_or = space* '||' space*;
        logical_and = space+ | space* (',' | '&&') space*;

        # from version - find a way to merge
        #
        number = digit+ >SB %{ n = std::stoll(std::string{sb, p}); };
        number_version_part = number %{ *part++ = n; } | [Xx*] %{ *part++ = ANY; };
        basic_version = number_version_part ('.' number_version_part){,3};
        extra_part = alnum_+ >SB %{ v.extra.emplace_back(sb, p); };
        extra = extra_part ('.' extra_part)*;
        version = (basic_version ('-' extra)?) >RESET_VER %SET_VER_LEVEL;
        #

        hyphen = version %{ rp.first = v; } space+ '-' space+ version %{ rp.second = v; if (!vr_and.empty()) vr_and.pop_back(); };
        tilde = '~' space* version %TILDE_VER;
        caret = '^' space* version %CARET_VER;
        cmp = (
            '<'  %{ sign = LT; } |
            '>'  %{ sign = GT; } |
            '<=' %{ sign = LE; } |
            '>=' %{ sign = GE; } |
            '='  %{ sign = EQ; } |
            '!=' %{ sign = NE; }
        ) space* version;

        range = (
            cmp %RANGE_CMP | caret | tilde | hyphen | version %RANGE_VER
        ) >RESET_PAIR %{ if (sign != NE) vr_and.push_back(prepare_pair(rp)); };

        range_set_and = (range (logical_and range)*) >{ vr_and = VersionRange::Range{}; vr_or_neq.range.clear(); } %ADD_RANGE;
        range_set_or = range_set_and (logical_or range_set_and)*;

        main := range_set_or %OK;

		write init;
		write exec;
    }%%

    return ok;
}

}
