// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/version.h>

#include <primitives/exceptions.h>

#include <algorithm>
#include <charconv>

namespace primitives::version
{

bool Version::parse(Version &v, const std::string &s)
{
    if (s.size() > 8192)
        return false;

    bool ok = false;
	int cs = 0;
	auto p = s.data();
    auto pe = p + s.size();
    auto eof = pe;

    Version::Number n; // number
    auto sb = p; // string begin

    %%{
        machine Version;
	    write data;

        action SB { sb = p; }
        action OK { ok = p == pe; }

        action ADD_PART
        {
            v.numbers.push_back(n);
        }

        number = digit+ >SB %{ n = std::stoll(std::string{sb, p}); };
        number_version_part = number %ADD_PART;

        # to parse versions from git, we could try our best
        # this goes to coerce()
        #basic_version = [=]? [v]? number_version_part ('.' number_version_part)*;

        # we have limit - maximum 4 parts                             vvv
        #basic_version = number_version_part ('.' number_version_part){,3};
        # limit was disabled
        basic_version = number_version_part ('.' number_version_part)*;

        main := basic_version %OK;

		write init;
		write exec;
    }%%

    //SW_UNIMPLEMENTED;
    //v.setFirstVersion();

    return ok;
}

namespace detail
{

bool Extra::parse(const std::string &s)
{
    if (s.size() > 8192)
        return false;

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

        action ADD_EXTRA
        {
            primitives::version::detail::Number n;
            if (auto [ptr, ec] = std::from_chars(sb, p, n); ptr == p && ec == std::errc())
                value.emplace_back(n);
            else
                value.emplace_back(std::string{sb, p});
        }

        alnum_ = alnum | '_';

        number = digit+ >SB %{ n = std::stoll(std::string{sb, p}); };
        extra_part = alnum_+ >SB %ADD_EXTRA;
        extra = extra_part ('.' extra_part)*;

        main := extra %OK;

		write init;
		write exec;
    }%%

    return ok;
}

} // namespace detail

} // namespace primitives::version
