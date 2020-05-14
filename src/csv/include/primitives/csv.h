// Copyright (C) 2020 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace primitives::csv
{

using Columns = std::vector<std::optional<std::string>>;

// add delim, quote options?
PRIMITIVES_CSV_API
Columns parse_line(const std::string &,
    char delimeter = ',', char quote = '"', char escape = '\\');

}
