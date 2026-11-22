#pragma once

#ifndef FWD
#define FWD(x) std::forward<decltype(x)>(x)
#endif

#ifndef SW_OVERLOAD
#define SW_OVERLOAD

template<typename ... Ts>
struct overload : Ts... {
    overload(Ts && ... ts) : Ts(FWD(ts))... {}
    using Ts::operator()...;
};
//template<typename... Ts> overload(Ts...) -> overload<Ts...>;

#endif
