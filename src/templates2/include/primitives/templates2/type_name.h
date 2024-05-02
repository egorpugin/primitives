#pragma once

#include <source_location>
#include <string_view>
using namespace std::literals;

template <typename T>
consteval auto type_name_with_namespaces() {
    std::string_view fn = std::source_location::current().function_name();
    auto structstr = "struct "sv;
    auto classstr = "class "sv;
#ifdef _MSC_VER
    fn = fn.substr(fn.rfind("<") + 1);
    if (fn.starts_with(structstr)) {
        fn = fn.substr(structstr.size());
    } else if (fn.starts_with(classstr)) {
        fn = fn.substr(classstr.size());
    } else {
        // builtin type
    }
    fn = fn.substr(0, fn.find(">"));
#else
    fn = fn.substr(fn.rfind("T = ") + 4);
    fn = fn.substr(0, fn.size() - 1);
#endif
    return fn;
}

template <typename T>
consteval auto type_name() {
    auto fn = type_name_with_namespaces<T>();
    fn = fn.substr(fn.rfind(":") + 1); // remove namespaces
    return fn;
}
