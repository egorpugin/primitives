// Copyright (C) 2020 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/cl.h>

#include "driver.h"

// reference libraries
//  - https://github.com/CLIUtils/CLI11
//  - llvm's CommandLine

namespace primitives::cl
{

void parse(const Strings &args)
{
    ProgramDescription pd;
    pd.args = args;
    parse(pd);
}

void parse(int argc, char **argv)
{
    Strings args;
    for (int i = 0; i < argc; i++)
        args.push_back(argv[i]);
    parse(args);
}

void parse(const ProgramDescription &pd)
{
    if (pd.args.empty())
        throw SW_RUNTIME_ERROR("Arguments must not be empty. Program is missing.");

    MY_PARSER driver(pd);
    if (driver.parse())
        throw SW_RUNTIME_ERROR("Error during parsing: " + driver.bb.error_msg);
}

bool ProgramDescription::hasOption(const String &o) const
{
    return option_map.find(o) != option_map.end();
}

}
