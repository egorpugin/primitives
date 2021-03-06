// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/patch.h>

#include <primitives/exceptions.h>

#include <boost/algorithm/string.hpp>

#include <regex>

// just skip fully empty lines
#define EMPTY_ACTION(x) if ((x).empty()) continue

namespace primitives::patch
{

static
void add_line(String &r, int &ni, const String &s)
{
    r += s + "\n";
    ni++;
}

static
std::pair<bool, String> patch_hunk(
    const Strings &lines, Strings::iterator &i, const Strings::iterator end, int options,
    size_t &oi, int &ni, int os, int ns)
{
    // we are in hunk right now

    String r;

    for (; i != end; i++)
    {
        EMPTY_ACTION(*i);

        if (os == 0 && ns == 0)
            return { true, r };
        if (os < 0 || ns < 0)
            return { false, "bad hunk info" };

        switch ((*i)[0])
        {
        case ' ':
            if (lines[oi] != i->substr(1))
                return { false, "context lines do not match" };
            add_line(r, ni, lines[oi]);
            os--;
            ns--;
            oi++;
            break;
        case '+':
            add_line(r, ni, i->substr(1));
            ns--;
            break;
        case '-':
            if (lines[oi] != i->substr(1))
                return { false, "context lines do not match" };
            os--;
            oi++;
            break;
        default:
            if (os == 0 && ns == 0)
                return { true, r };
            // actually this is ok, but we fall for now
            return { false, "bad symbol "s + (*i)[0] +
                ", os = " + std::to_string(os) +
                ", ns = " + std::to_string(ns) };
        }
    }

    if (os == 0 && ns == 0)
        return { true, r };
    return { false, "bad hunk info" };
}

static
std::pair<bool, String> patch_file(
    const Strings &lines, Strings::iterator &i, const Strings::iterator end, int options,
    size_t &oi)
{
    String r;

    int ni = 0; // new index

    for (; i != end; i++)
    {
        EMPTY_ACTION(*i);

        switch ((*i)[0])
        {
        case '+':
            if (i->find("+++") == 0)
                continue;
            return { false, "+ is out of hunk" };
        case '-':
            if (i->find("---") == 0)
                continue;
            return { false, "- is out of hunk" };
        case '@':
        {
            // @@ -1,3 +1,9 @@ some extra info here (git uses this)
            static const std::regex r_hunk("@@ -(\\d+),(\\d+) \\+(\\d+),(\\d+) @@.*");
            std::smatch m;
            if (!std::regex_match(*i, m, r_hunk))
                return { false, "bad hunk description" };

            auto ol = std::stoi(m[1].str()) - 1;
            auto os = std::stoi(m[2].str());
            auto nl = std::stoi(m[3].str()) - 1;
            auto ns = std::stoi(m[4].str());

            // copy lines up to hunk
            for (; oi < (size_t)ol; oi++)
                add_line(r, ni, lines[oi]);

            if (nl != ni)
                return { false, "nl is wrong: " + std::to_string(nl + 1) };

            auto r2 = patch_hunk(lines, ++i, end, options, oi, ni, os, ns);
            if (!r2.first)
                return { false, "bad hunk " + std::to_string(nl + 1) + ": " + r2.second };
            r += r2.second;
            --i;
        }
            break;
        // git stuff
        case 'd':
        case 'i':
            if (options & PatchOption::Git)
                return { true, r }; // handle outside
        default:
            return { false, "unknown action" };
        }
    }

    return { true, r };
}

static
auto prepare_lines(const String &s)
{
    Strings v;
    boost::split(v, s, boost::is_any_of("\n"));
    for (auto &l : v)
    {
        while (!l.empty() && l.back() == '\r')
            l.resize(l.size() - 1);
    }
    return v;
}

static
std::pair<bool, String> patch(const String &text, Strings::iterator &i, const Strings::iterator end, int options)
{
    auto lines = prepare_lines(text);

    size_t oi = 0; // old index
    auto r = patch_file(lines, i, end, options, oi);

    if (!r.first)
        return { false, "bad file: " + r.second }; // bad file

    // add rest of the file
    for (; oi < lines.size(); oi++)
        r.second += lines[oi] + "\n";

    if (!r.second.empty())
        r.second.resize(r.second.size() - 1); // remove last '\n'

    return r;
}

std::pair<bool, String> patch(const String &text, const String &unidiff, int options)
{
    auto diff = prepare_lines(unidiff);

    auto begin = diff.begin();
    return patch(text, begin, diff.end(), options);
}

/*static
bool patch(const path &root_dir, Strings::iterator &i, const Strings::iterator end, int options)
{
    for (; i != end; i++)
    {
        EMPTY_ACTION(*i);

        switch ((*i)[0])
        {
        // git stuff
        case 'd':
            if (options & PatchOption::Git)
            {
                // diff --git a/cppan.yml b/cppan.yml
                static const std::regex r_d("diff --git a/([^[[:space:]]]).*");
                std::smatch m;
                if (!std::regex_match(*i, m, r_d))
                    return {}; // bad diff description

                path fn = m[1].str();
                if (fn.is_absolute())
                    return {}; // not allowed

                auto text = read_file(root_dir / fn);
                auto r = patch(text, i, end, options);
                if (!r)
                    return {}; // cannot patch this file

                continue;
            }
            [[fallthrough]];
        case 'i':
            if (options & PatchOption::Git)
                continue;
            [[fallthrough]];
        default:
            return false;
        }
    }

    return true;
}*/

bool patch(const path &root_dir, const String &unidiff, int options)
{
    throw SW_RUNTIME_ERROR("not tested");

    /*auto diff = prepare_lines(unidiff);

    auto begin = diff.begin();
    return patch(root_dir, begin, diff.end(), options);*/
}

}
