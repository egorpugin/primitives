#pragma once

#include <string_view>
#include <vector>

namespace primitives::html {

enum class token_type {
    comment,
    text,
    cdata, // TODO: also handle pcdata
    begin_tag,
    end_tag,
    complete_tag,
};
struct token {
    token_type t;
    std::string_view d;
};

using html_tokens = std::vector<token>;

bool is_void_element(auto sv) {
    sv.remove_prefix(1);
    return false
        || sv.starts_with("area"sv)
        || sv.starts_with("base"sv)
        || sv.starts_with("br"sv)
        || sv.starts_with("col"sv)
        || sv.starts_with("embed"sv)
        || sv.starts_with("hr"sv)
        || sv.starts_with("img"sv)
        || sv.starts_with("input"sv)
        || sv.starts_with("link"sv)
        || sv.starts_with("meta"sv)
        || sv.starts_with("param"sv)
        || sv.starts_with("source"sv)
        || sv.starts_with("track"sv)
        || sv.starts_with("wbr"sv)
        ;
}
bool preserve_space(auto sv) {
    sv.remove_prefix(1);
    return false
        || sv.starts_with("pre"sv)
        ;
}

auto make_tokens(std::string_view d) {
    html_tokens tokens;
    tokens.reserve(100000);
    size_t p{};
    while (1) {
        auto b = d.find('<', p);
        if (b == -1) {
            if (b != d.size()) {
                b = d.size();
                tokens.emplace_back(token_type::text, d.substr(p, b - p));
            }
            break;
        }
        if (b != p) {
            tokens.emplace_back(token_type::text, d.substr(p, b - p));
            p = b;
            continue;
        }
        auto sv = d.substr(b);
        if (sv.starts_with("<!--")) {
            constexpr auto etag = "-->"sv;
            auto e = d.find(etag, b);
            e += etag.size();
            tokens.emplace_back(token_type::comment, d.substr(b, e - b));
            p = e;
            continue;
        }
        constexpr auto cdata = "<![CDATA["sv;
        if (sv.starts_with(cdata)) {
            constexpr auto etag = "]]>"sv;
            auto e = d.find(etag, b);
            b += cdata.size();
            tokens.emplace_back(token_type::cdata, d.substr(b, e - b));
            e += etag.size();
            p = e;
            continue;
        }
        auto e = d.find('>', b);
        ++e;
        auto t = d.substr(b, e - b);
        tokens.emplace_back(t.starts_with("</"sv) ? token_type::end_tag : (t.ends_with("/>"sv) || is_void_element(t) ? token_type::complete_tag : token_type::begin_tag), t);
        p = e;
    }
    return tokens;
}

struct html_node {
    using string_type = std::string_view;

    string_type d;
    std::map<string_type, string_type> attributes;
    std::vector<html_node> children;

    html_node() = default;
    html_node(std::string_view text) : d{text} {
    }
    html_node(std::string_view text, auto) {
        constexpr auto end = " />"sv;
        constexpr auto attrval = " =/>"sv;
        constexpr auto quotes = "'\""sv; // some quotes can be '
        auto p = text.find_first_of(end);
        d = text.substr(1, p - 1);
        string_type k, v;
        while (1) {
            p = text.find_first_not_of(end, p);
            if (p == -1) {
                break;
            }
            auto e = text.find_first_of(attrval, p);
            k = text.substr(p, e-p);
            if (text[e] == '=') {
                p = text.find_first_of(quotes, p);
                e = text.find(text[p] == '\'' ? '\'' : '\"', p + 1);
                ++p;
                v = text.substr(p, e-p);
                ++e;
            }
            attributes.emplace(k, v);
            p = e;
        }
    }
    bool empty() const {return d.empty();}
};

std::vector<html_node> make_tree(auto &it, auto &&end, bool keep_whitespace = false) {
    std::vector<html_node> tags;
    while (it != end) {
        auto &[tt,t] = *it++;
        if (t.starts_with("<!DOCTYPE"sv) || tt == token_type::comment) {
            continue;
        }
        if (!keep_whitespace && std::ranges::all_of(t, [](char c){return c == ' ' || c == '\r' || c == '\n' || c == '\t';})) {
            continue;
        }
        switch (tt) {
        case token_type::text:
        case token_type::cdata:
            tags.emplace_back(t);
            break;
        case token_type::complete_tag:
            tags.emplace_back(t, token_type::complete_tag);
            break;
        case token_type::begin_tag:
            tags.emplace_back(t, token_type::begin_tag).children = make_tree(it, end, keep_whitespace || preserve_space(t));
            continue;
        case token_type::end_tag:
            return tags;
        }
    }
    return tags;
}
html_node make_tree(const html_tokens &tokens) {
    auto it = tokens.begin();
    auto tree = make_tree(it, tokens.end());
    for (auto &&t : tree) {
        if (t.d.starts_with("<html"sv)) {
            return std::move(t);
        }
    }
    return {};
}

} // namespace primitives::html
