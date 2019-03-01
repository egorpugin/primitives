// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Some parts of this code are taken from LLVM.Support.CommandLine.

#pragma once

#include <primitives/CommandLine.h>

namespace cl
{

using namespace ::primitives::cl;

using ::primitives::cl::ParseCommandLineOptions;

inline bool ParseCommandLineOptions(const Strings &args,
    llvm::StringRef Overview = "",
    llvm::raw_ostream *Errs = nullptr)
{
    std::vector<const char *> argv;
    for (auto &a : args)
        argv.push_back(a.data());
    return ParseCommandLineOptions(argv.size(), argv.data(), Overview, Errs);
}

}
