// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#ifndef SW_MAIN_IMPL
#include <primitives/sw/sw.settings.program_name.h>
#endif

#include <primitives/filesystem.h>

int sw_main_internal(int argc, char *argv[]);
int SW_MAIN(int argc, char *argv[]);

#ifdef main
#define SW_OLD_MAIN1(x) x
#define SW_OLD_MAIN SW_OLD_MAIN1(main)
int main(int argc, char *argv[]) { return sw_main_internal(argc, argv); }
#undef main
#endif

#ifndef SW_MAIN_IMPL
#define main SW_MAIN
#endif

void sw_append_symbol_path(const path &in);
void sw_enable_crash_server(int level = 0); // send -1 to disable
