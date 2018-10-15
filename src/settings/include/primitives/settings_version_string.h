// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Some parts of this code are taken from LLVM.Support.CommandLine.

#pragma once

#include <string>

#ifdef _WIN32
#if defined(CPPAN_EXECUTABLE)
#define EXPORT_FROM_EXECUTABLE __declspec(dllexport)
#endif
#else
#define EXPORT_FROM_EXECUTABLE __attribute__((visibility ("default")))
#endif

#ifndef EXPORT_FROM_EXECUTABLE
#define EXPORT_FROM_EXECUTABLE
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4190) // function with C-linkage returns UDT
#endif

namespace primitives
{

EXPORT_FROM_EXECUTABLE
std::string getVersionString();

}
