// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/git_rev.h>
#include <gitrev.h>

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

namespace primitives::git_rev
{

static auto get_time(const tm *t, const char *fmt)
{
    std::ostringstream ss;
    ss << std::put_time(t, fmt);
    return ss.str();
}

std::string getBuildTime()
{
    time_t t = SW_BUILD_TIME_T;
    std::ostringstream ss;
    ss << "assembled on\n";
    auto t1 = get_time(gmtime(&t), "%d.%m.%Y %H:%M:%S UTC");
    auto t2 = get_time(localtime(&t), "%d.%m.%Y %H:%M:%S %Z");
    ss << t1;
    if (t1 != t2)
        ss << "\n" << t2;
    return ss.str();
}

std::string getGitRevision()
{
    std::string gitrev = SW_GIT_REV;
    if (!gitrev.empty())
    {
        gitrev = "git revision " + gitrev;
        if (SW_GIT_CHANGED_FILES)
            gitrev += " plus " + std::to_string(SW_GIT_CHANGED_FILES) + " modified files";
    }
    return gitrev;
}

}
