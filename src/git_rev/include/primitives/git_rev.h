// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <string>

namespace primitives::git_rev {

std::string getBuildTime();
std::string getGitRevision();

inline auto common_string() {
    return getGitRevision() + "\n" + getBuildTime();
}

}
