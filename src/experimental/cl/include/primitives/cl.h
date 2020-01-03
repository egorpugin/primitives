// Copyright (C) 2020 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/string.h>

namespace primitives::cl
{

/// parse command line
/// argv[0] or program must be here
PRIMITIVES_EXPERIMENTAL_CL_API
void parse(const Strings &args);

PRIMITIVES_EXPERIMENTAL_CL_API
void parse(int argc, char **argv);

struct Option
{
    String value;
};

struct ProgramDescription
{
    Strings args;
    String description; // printed in -help
    std::map<String, Option> option_map;
    bool throw_on_missing_option = true;

    bool hasOption(const String &) const;
};

// main function
PRIMITIVES_EXPERIMENTAL_CL_API
void parse(const ProgramDescription &);

}
