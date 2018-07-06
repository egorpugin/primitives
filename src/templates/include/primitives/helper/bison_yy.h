// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Second part of bison helper.
// It goes to %code provides section of parser file.

// set debug level
#if CONCATENATE(THIS_PARSER_UP, DEBUG)
#define SET_DEBUG_LEVEL set_debug_level(1)
#endif

// declare parser itself
DECLARE_PARSER;
