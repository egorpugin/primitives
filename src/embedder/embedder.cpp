// Copyright (C) 2016-2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/filesystem.h>
#include <primitives/main.h>
#include <primitives/sw/settings.h>

#include <iostream>
#include <regex>

// TODO: we can zip required files (text)
// add an option for this

String preprocess_file(const String &s)
{
    String o;
    int i = 0;
    for (auto &c : s)
    {
        String h(2, 0);
        sprintf(&h[0], "%02x", c);
        o += "0x" + h + ",";
        if (++i % 25 == 0)
            o += "\n";
        else
            o += " ";
    }
    o += "0x00,";
    if (++i % 25 == 0)
        o += "\n";
    return o;
}

int main(int argc, char *argv[])
{
    cl::opt<std::string> InputFilename(cl::Positional, cl::desc("<input file>"), cl::Required);
    cl::opt<std::string> OutputFilename(cl::Positional, cl::desc("<input file>"), cl::Required);

    cl::parseCommandLineOptions(argc, argv);

    path p = argv[1];
    if (!fs::exists(p))
    {
        std::cerr << "no such file: " << p.string() << "\n";
        return 2;
    }

    auto s = read_file(p);
    std::regex r("EMBED<(.*?)>");
    std::smatch m;
    while (std::regex_search(s, m, r))
    {
        String str;
        str = m.prefix();
        path f = m[1].str();
        if (!fs::exists(f))
        {
            std::cerr << "no such include file: " << f.string() << "\n";
            return 3;
        }
        str += preprocess_file(read_file(f));
        str += m.suffix();
        s = str;
    }

    write_file(argv[2], s);

    return 0;
}
