// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/string.h>

#include <uv.h>

namespace primitives::detail
{

inline int uv_loop_close(uv_loop_t &loop, Strings &errors)
{
    int r;
    if (r = uv_loop_close(&loop); r)
    {
        if (r == UV_EBUSY)
        {
            auto wlk = [](uv_handle_t* handle, void* arg)
            {
                if (handle)
                    uv_close(handle, 0);
            };
            uv_walk(&loop, wlk, 0);
            while (uv_run(&loop, UV_RUN_DEFAULT) != 0)
                ;
            if (r = uv_loop_close(&loop); r)
            {
                if (r == UV_EBUSY)
                    errors.push_back("resources were not cleaned up: "s + uv_strerror(r));
                else
                    errors.push_back("uv_loop_close() error: "s + uv_strerror(r));
            }
        }
        else
            errors.push_back("uv_loop_close() error: "s + uv_strerror(r));
    }
    return r;
}

}
