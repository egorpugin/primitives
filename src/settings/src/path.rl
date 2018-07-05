// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/settings.h>

namespace primitives
{

SettingPath parseSettingPath1(const String &s)
{
    SettingPath setting_path;

    bool ok = false;
	int cs = 0;
	auto p = s.data();
    auto pe = p + s.size();
    auto eof = pe;
    //int stack[100];
    //int top;

    auto sb = p; // string begin
    const char *se = nullptr; // string end
    String basic;

    %%{
        machine VersionExtra;
	    write data;

        action return { fret; }

        action SB { sb = p; }
        action SE { se = p; }
        action OK { ok = p == pe; }
        action ADD
        {
            if (!basic.empty())
                setting_path.emplace_back(std::move(basic));
            else
                setting_path.emplace_back(sb, sb > se ? p : se);
        }

        escaped_character =
            'b' %{ basic += '\b'; } |
            't' %{ basic += '\t'; } |
            'n' %{ basic += '\n'; } |
            'f' %{ basic += '\f'; } |
            'r' %{ basic += '\r'; } |
            '"' %{ basic += '\"'; } |
            '\\' %{ basic += '\\'; } |
            'u' xdigit{4} %{ basic += *p; } |
            'U' xdigit{8} %{ basic += *p; }
        ;

        basic_character =
            ('\\' escaped_character) |
            (extend -- cntrl) %{ basic += *p; }
        ;

        # basic2 := basic_character* | '"' @return;
        # basic = '"' @{ fcall basic2; };
        basic = '"' ((basic_character* - (^'\\' '"')) >SB %SE) '"';

        alnum_ = alnum | '_' | '-';
        bare = alnum_* >SB %SE;
        literal = '\'' [^']* >SB %SE '\'';
        part = space* (bare | basic | literal) space* %ADD;
        main := (part ('.' part)*)? %OK;

		write init;
		write exec;
    }%%

    if (!ok)
        throw std::runtime_error("Bad setting path: " + s);

    if (setting_path.empty())
        setting_path.push_back("");

    return setting_path;
}

}
