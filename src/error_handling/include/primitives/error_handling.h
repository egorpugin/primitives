// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <string>

PRIMITIVES_ERROR_HANDLING_API
void debug_break_if_debugger_attached();

PRIMITIVES_ERROR_HANDLING_API
void report_fatal_error(const std::string &);
