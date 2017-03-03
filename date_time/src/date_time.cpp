#include <primitives/date_time.h>

TimePoint getUtc()
{
    boost::posix_time::ptime t(
        boost::gregorian::day_clock::universal_day(),
        boost::posix_time::second_clock::universal_time().time_of_day());
    return std::chrono::system_clock::from_time_t(boost::posix_time::to_time_t(t));
}

TimePoint string2timepoint(const String &s)
{
    auto t = boost::posix_time::time_from_string(s);
    return std::chrono::system_clock::from_time_t(boost::posix_time::to_time_t(t));
}

time_t string2time_t(const String &s)
{
    return std::chrono::system_clock::to_time_t(string2timepoint(s));
}

String local_time()
{
    auto t = boost::posix_time::second_clock::local_time();
    return boost::posix_time::to_simple_string(t);
}
