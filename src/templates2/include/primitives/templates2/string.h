#pragma once

#include <string>
#include <vector>

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