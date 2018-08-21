// Copyright (C) 2016-2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/filesystem.h>
#include <primitives/sw/main.h>
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
    cl::opt<path> InputFilename(cl::Positional, cl::desc("<input file>"), cl::Required);
    cl::opt<path> OutputFilename(cl::Positional, cl::desc("<output file>"), cl::Required);

    cl::ParseCommandLineOptions(argc, argv);

    auto s = read_file(InputFilename);
    std::regex r("EMBED<(.*?)>");
    std::smatch m;
    while (std::regex_search(s, m, r))
    {
        String str;
        str = m.prefix();
        str += preprocess_file(read_file(m[1].str()));
        str += m.suffix();
        s = str;
    }

    write_file(OutputFilename, s);

    return 0;
}