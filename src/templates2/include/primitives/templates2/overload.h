#pragma once

#ifndef FWD
#define FWD(x) std::forward<decltype(x)>(x)
#endif

template<typename ... Ts>
struct overload : Ts... {
    overload(Ts && ... ts) : Ts(FWD(ts))... {}
    using Ts::operator()...;
};
//template<typename... Ts> overload(Ts...) -> overload<Ts...>;
