// Copyright (C) 2016-2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/filesystem.h>
#include <primitives/sw/main.h>
#include <primitives/sw/settings.h>
#include <primitives/sw/cl.h>

#include <iostream>
#include <regex>

// TODO: we can zip required files (text)
// add an option for this

String preprocess_file(const String &s)
{
    String o;
    int i = 0;
    for (uint8_t c : s)
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
    cl::opt<path> deps_file("-d"); // gnu style

    cl::ParseCommandLineOptions(argc, argv);

    String d;
    d += to_string(OutputFilename.u8string()) + ": \\\n";

    auto s = read_file(InputFilename);
    std::regex r("EMBED<(.*?)>");
    std::smatch m;
    while (std::regex_search(s, m, r))
    {
        String str;
        str = m.prefix();
        // make a copy here, because when str is changed, m[1].str() will point to new string
        String fn = m[1].str();
        str += preprocess_file(read_file(fn));
        str += m.suffix();
        s = str;
        // print inputs for consumers
        d += to_string((fs::current_path() / fn).u8string()) + " \\\n";
    }

    write_file(OutputFilename, s);
    if (!deps_file.empty())
        write_file(deps_file, d);

    return 0;
}
