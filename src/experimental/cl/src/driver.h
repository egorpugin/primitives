// Copyright (C) 2020 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "location.h"
#include <cl_parser.h>
#include <cl.yy.hpp>

#include <primitives/cl.h>

#include <string>
#include <vector>

// does not validate input!!!
DECLARE_PARSER_BASE;
public:
    const ::primitives::cl::ProgramDescription &pd;

    MY_PARSER(const ::primitives::cl::ProgramDescription &);

    MY_LEXER_FUNCTION_SIGNATURE;
    int parse();

private:
    int argi = 0;
    bool print_argument_delimeter = false;

    void set_next_buf();
};
