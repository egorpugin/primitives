#pragma once

template <typename T>
consteval auto type_name() {
    std::string_view fn = std::source_location::current().function_name();
    auto structstr = "struct "sv;
    auto classstr = "class "sv;
#ifdef _MSC_VER
    fn = fn.substr(fn.rfind("<") + 1);
    fn = fn.substr(fn.rfind(":") + 1);
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
    fn = fn.substr(fn.rfind(":") + 1); // remove namespaces
#endif
    return fn;
}
