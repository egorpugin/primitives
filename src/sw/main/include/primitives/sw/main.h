// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>

int SW_MAIN(int argc, char *argv[]);

#ifndef SW_MAIN_IMPL
#define main SW_MAIN
#endif

void sw_append_symbol_path(const path &in);
