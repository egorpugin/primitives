// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/patch.h>

#include <boost/algorithm/string.hpp>

#include <regex>

namespace primitives::patch
{

static std::optional<String> patch_hunk(const Strings &lines, Strings::iterator &i, Strings::iterator end, int options)
{
    return {};
}

static std::optional<String> patch_file(const Strings &lines, Strings::iterator &i, Strings::iterator end, int options)
{
    String r;
    bool in_hunk = false;
    int oi = 0; // old index
    int ni = 0; // new index
    int os = 0; // old changes
    int ns = 0; // new changes

    auto add_line = [&r, &ni](const String &s)
    {
        r += s + "\n";
        ni++;
    };

    auto check_hunk = [&in_hunk, &os, &ns]()
    {
        if (os == 0 && ns == 0)
            in_hunk = false;
        if (os < 0 || ns < 0)
            return false;
        return true;
    };

    for (; i != end; i++)
    {
        if (i->empty())
            return {}; // empty lines not allowed

        if (!check_hunk())
            return {}; // hunk info is bad

        switch ((*i)[0])
        {
        case ' ':
            if (lines[oi] != i->substr(1))
                return {}; // context lines do not match
            add_line(lines[oi]);
            os--;
            ns--;
            oi++;
            break;
        case '+':
            if (!in_hunk)
            {
                if (i->find("+++") == 0)
                    continue;
                return {}; // + is out of hunk
            }
            add_line(i->substr(1));
            ns--;
            break;
        case '-':
            if (!in_hunk)
            {
                if (i->find("---") == 0)
                    continue;
                return {}; // - is out of hunk
            }
            os--;
            oi++;
            break;
            // git stuff
        case 'd':
        case 'i':
            if (options & PatchOption::Git)
            {
                if (in_hunk)
                    return {}; // symbols are in hunk
            }
            else
                return {}; // not git format
            break;
        case '@':
        {
            // @@ -1,3 +1,9 @@
            static const std::regex r_hunk("@@ -(\\d+),(\\d+) \\+(\\d+),(\\d+) @@");
            std::smatch m;
            if (!std::regex_match(*i, m, r_hunk))
                return {}; // bad hunk description

            in_hunk = true;

            auto ol = std::stoi(m[1].str()) - 1;
            os = std::stoi(m[2].str());
            auto nl = std::stoi(m[3].str()) - 1;
            ns = std::stoi(m[4].str());

            // copy lines up to hunk
            for (; oi < ol; oi++)
                add_line(lines[oi]);

            if (nl != ni)
                return {}; // nl is wrong
        }
        break;
        default:
            return {};
        }
    }

    if (!check_hunk() || in_hunk)
        return {}; // hunk info is bad

    return r;
}

std::optional<String> patch(const String &text, const String &unidiff, int options)
{
    auto prepare_lines = [](const auto &s)
    {
        Strings v;
        boost::split(v, s, boost::is_any_of("\n"));
        for (auto &l : v)
        {
            while (!l.empty() && l.back() == '\r')
                l.resize(l.size() - 1);
        }
        return v;
    };

    auto lines = prepare_lines(text);
    auto diff = prepare_lines(unidiff);

    auto begin = diff.begin();
    auto r = patch_file(lines, begin, diff.end(), options);

    if (!r)
        return {};

    if (!r->empty())
        r->resize(r->size() - 1); // remove last '\n'

    return r;
}

bool patch(const path &root_dir, const String &unidiff, int options)
{
    return true;
}

}
