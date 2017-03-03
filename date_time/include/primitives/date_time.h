#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <primitives/string.h>

#include <chrono>

using namespace std::literals;

using Clock = std::chrono::system_clock;
using TimePoint = Clock::time_point;

using ptime = boost::posix_time::ptime;

TimePoint getUtc();
TimePoint string2timepoint(const String &s);
time_t string2time_t(const String &s);
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
