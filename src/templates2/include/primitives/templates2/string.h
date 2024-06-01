#pragma once

#include <string>
#include <vector>

template<std::size_t N>
struct static_string {
    char p[N]{};

    constexpr static_string(char const(&pp)[N]) {
        std::ranges::copy(pp, p);
    }
    //operator auto() const { return &p[0]; }
    constexpr auto begin() const {return p;}
    constexpr auto end() const {return p+size();}
    constexpr auto operator[](int i) const {return p[i];}
    static constexpr auto size() {return N-1;}
};
template<static_string s>
constexpr auto operator "" _s() { return s; }

auto find_text_between(auto &&text, auto &&from, auto &&to) {
    std::vector<std::string> fragments;
    auto pos = text.find(from);
    while (pos != -1) {
        auto pose = text.find(to, pos);
        if (pose == -1) {
            break;
        }
        fragments.emplace_back(text.substr(pos + from.size(), pose - (pos + from.size())));
        pos = text.find(from, pos + 1);
    }
    return fragments;
}