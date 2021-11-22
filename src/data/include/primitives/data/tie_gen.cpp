// g++ tie_gen.cpp -std=c++23 && ./a.out 100 > tie_for_struct.inl

#include <algorithm>
#include <iostream>
#include <numeric>
#include <ranges>
#include <vector>

auto join(auto &&r, auto &&delim) {
    std::ranges::range_value_t<decltype(r)> s;
    auto it = std::begin(r);
    s += *it++;
    for (; it != std::end(r); ++it) {
        s += delim;
        s += *it;
    }
    return s;
}

auto to_vector(auto &&r) {
    std::vector<std::ranges::range_value_t<decltype(r)>> v;
    if constexpr(std::ranges::sized_range<decltype(r)>)
        v.reserve(std::ranges::size(r));
    std::ranges::copy(r, std::back_inserter(v));
    return v;
}

int main(int, char *argv[]) {
    using namespace std;
    using namespace std::ranges;
    auto v = to_vector(views::iota(1, stoi(argv[1]) + 1) | views::transform([](auto &&i) {
        auto v = to_vector(views::iota(1, i + 1) | views::transform([](auto &&i) { return "a" + to_string(i); }));
        auto joined = join(v, ",");
        return
            "else if constexpr (N == " + to_string(i) + ") {\n    auto &["
            + joined + "] = data;\n    return std::tie(" + joined + ");\n}";
    }));
    std::cout << join(v, " ");
}
