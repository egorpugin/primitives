#pragma once

#if __has_include(<optional>)
#include <optional>
template <class T>
using optional = std::optional<T>;
#else
#include <boost/optional.hpp>
template <class T>
using optional = boost::optional<T>;
#endif
