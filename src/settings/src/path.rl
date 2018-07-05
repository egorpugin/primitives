// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/settings.h>

namespace primitives
{

static String unescapeSettingPart(const String &s)
{
    if (s.empty())
        return s;

    String basic;

    bool ok = false;
	int cs = 0;
	auto p = s.data();
    auto pe = p + s.size();
    auto eof = pe;

    auto sb = p; // string begin
    const char *se = nullptr; // string end

    %%{
        machine EscapedSettingPart;
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

        basic_character = ('\\' escaped_character) |
            any %{ basic += *p; }
        ;

        main := basic_character %OK;

		write init;
		write exec;
    }%%

    if (!ok)
        throw std::runtime_error("Bad setting path part: " + s);

    return basic;
}

SettingPath parseSettingPath1(const String &s)
{
    SettingPath setting_path;

    bool ok = false;
	int cs = 0;
	auto p = s.data();
    auto pe = p + s.size();
    auto eof = pe;

    auto sb = p; // string begin
    const char *se = nullptr; // string end
    bool escaped = false;

    %%{
        machine SettingPath;
	    write data;

        action return { fret; }

        action SB { sb = p; }
        action SE { se = p; }
        action OK { ok = p == pe; }
        action ADD
        {
            setting_path.emplace_back(sb, sb > se ? p : se);
            if (escaped)
            {
                setting_path.back() = unescapeSettingPart(setting_path.back());
                escaped = false;
            }
        }

        escaped_character =
            'b'  |
            't'  |
            'n'  |
            'f'  |
            'r'  |
            '"'  |
            '\\' |
            'u' xdigit{4} |
            'U' xdigit{8}
        ;

        basic_character = ('\\' escaped_character %{ escaped = true; }) |
        (extend -- cntrl -- '\\');

        alnum_ = alnum | '_' | '-';
        bare = alnum_* >SB %SE;
        basic = '"' (extend -- cntrl)* >SB %SE :>> (^'\\' '"') '"';
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
