// Copyright (C) 2016-2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>
#include <string>
#include <time.h>

int main()
{
    time_t v;
    time(&v);
    std::cout << "\"" <<
#ifdef NDEBUG
        std::to_string(v)
#else
        "0"
#endif
        << "\"";

    return 0;
}
