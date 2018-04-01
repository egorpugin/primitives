#pragma once

#if __has_include(<variant>)
#include <variant>
template <class ... Args>
using variant = std::variant<Args...>;
using std::get;
using std::visit;
#else
#include <mpark/variant.hpp>
template <class ... Args>
using variant = mpark::variant<Args...>;
using mpark::get;
using mpark::visit;
#endif
