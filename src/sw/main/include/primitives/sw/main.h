#pragma once

#include <primitives/filesystem.h>

int SW_MAIN(int argc, char *argv[]);

#ifndef SW_MAIN_IMPL
#define main SW_MAIN
#endif

void sw_append_symbol_path(const path &in);
