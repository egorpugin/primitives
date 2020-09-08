// Copyright (C) 2020 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/sw/cl.h>

#include <primitives/exceptions.h>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace cl
{

bool ParseCommandLineOptions(int argc, char **argv,
    llvm::StringRef Overview,
    llvm::raw_ostream *Errs)
{
#ifdef _WIN32
    int argc2;
    auto argw = CommandLineToArgvW(GetCommandLineW(), &argc2);
    SCOPE_EXIT
    {
        LocalFree(argw);
    };
    if (argc != argc2)
        throw SW_RUNTIME_ERROR("bad cmd");
    Strings args;
    for (int i = 0; i < argc; i++)
        args.push_back(to_string(std::wstring(argw[i])));
    return ParseCommandLineOptions(args, Overview, Errs);
#else
    return ::primitives::cl::ParseCommandLineOptions(argc, argv, Overview, Errs);
#endif
}

}
