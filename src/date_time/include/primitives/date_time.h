// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <primitives/string.h>

#include <chrono>

using namespace std::literals;

using Clock = std::chrono::system_clock;
using TimePoint = Clock::time_point;

using ptime = boost::posix_time::ptime;

PRIMITIVES_DATE_TIME_API
TimePoint getUtc();

PRIMITIVES_DATE_TIME_API
TimePoint string2timepoint(const String &s);

PRIMITIVES_DATE_TIME_API
time_t string2time_t(const String &s);

PRIMITIVES_DATE_TIME_API
String local_time();

// time of operation
template <typename F, typename ... Args>
auto get_time(F &&f, Args && ... args)
{
    using namespace std::chrono;

    auto t0 = high_resolution_clock::now();
    std::forward<F>(f)(std::forward<Args>(args)...);
    auto t1 = high_resolution_clock::now();
    return t1 - t0;
}

template <typename T, typename F, typename ... Args>
auto get_time(F &&f, Args && ... args)
{
    using namespace std::chrono;

    auto t = get_time(std::forward<F>(f), std::forward<Args>(args)...);
    return std::chrono::duration_cast<T>(t).count();
}

template <typename T, typename F, typename ... Args>
auto get_time_custom(F &&f, Args && ... args)
{
    using namespace std::chrono;

    auto t = get_time(std::forward<F>(f), std::forward<Args>(args)...);
    return std::chrono::duration_cast<std::chrono::duration<T>>(t).count();
}

struct PRIMITIVES_DATE_TIME_API ScopedTime
{
    using clock = std::chrono::high_resolution_clock;

    clock::time_point t0;

    ScopedTime()
    {
        t0 = clock::now();
    }

    auto getTime() const
    {
        return clock::now() - t0;
    }

    template <typename T>
    auto getTimeCustom() const
    {
        return std::chrono::duration_cast<std::chrono::duration<T>>(getTime()).count();
    }

    auto getTimeFloat() const
    {
        return getTimeCustom<float>();
    }
};
