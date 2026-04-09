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

using tokens_type = std::vector<token>;

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
    tokens_type tokens;
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

struct node {
    using string_type = std::string_view;

    enum class traverse_action { stop, continue_, skip_children };

    struct iterator {
        std::vector<const node *> nodes;
        std::vector<node>::iterator i;

        iterator() = default;
        iterator(const node &n) {
            nodes.push_back(&n);
        }
        const node &operator*() const {
            return *nodes.back();
        }
        auto &operator++() {

            return *this;
        }
        bool operator==(const iterator &) const = default;
    };

    string_type d;
    token_type type;
    std::map<string_type, string_type> attributes;
    std::vector<node> children;

    node() = default;
    node(std::string_view text, token_type type) : d{text}, type{type} {
    }
    node(std::string_view text, token_type type, auto) : type{type} {
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
    explicit operator bool() const {return !empty();}
    auto text() const {
        std::string t;
        traverse([&](auto &n) {
            if (n.type == token_type::text) {
                t += n.d;
            }
            return traverse_action::continue_;
        });
        return t;
    }
    traverse_action traverse(auto &&f) const {
        auto r = f(*this);
        if (r == traverse_action::stop) {
            return r;
        }
        if (r != traverse_action::skip_children)
        for (auto &&c : children) {
            if (auto r = c.traverse(f); r == traverse_action::stop) {
                return r;
            }
        }
        return traverse_action::continue_;
    }
    auto find(auto &&attrname, auto &&idname) {
        const node *r{};
        traverse([&](auto &n) {
            if (auto i = n.attributes.find(attrname); i != n.attributes.end() && i->second == idname) {
                r = &n;
                return traverse_action::stop;
            }
            return traverse_action::continue_;
        });
        return r;
    }
    auto begin() const {return iterator{*this};}
    auto end() const {return iterator{};}
};

std::vector<node> make_tree(auto &it, auto &&end, bool keep_whitespace = false) {
    std::vector<node> tags;
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
            tags.emplace_back(t, tt);
            break;
        case token_type::complete_tag:
            tags.emplace_back(t, tt, 0);
            break;
        case token_type::begin_tag:
            tags.emplace_back(t, tt, 0).children = make_tree(it, end, keep_whitespace || preserve_space(t));
            continue;
        case token_type::end_tag:
            return tags;
        }
    }
    return tags;
}
node make_tree(std::string_view page) {
    auto tokens = primitives::html::make_tokens(page);
    auto it = tokens.begin();
    auto tree = make_tree(it, tokens.end());
    for (auto &&t : tree) {
        if (t.d == "html"sv) {
            return std::move(t);
        }
    }
    return {};
}

} // namespace primitives::html
