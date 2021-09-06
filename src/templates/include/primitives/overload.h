#pragma once

template<typename ... Ts>
struct overload : Ts... {
    overload(Ts ... ts) : Ts(std::forward<Ts>(ts))... {}
    using Ts::operator()...;
};
//template<class... Ts> overload(Ts...) -> overload<Ts...>;
