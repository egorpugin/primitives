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
        int quotes{};
        size_t e;
        for (e = b; e < d.size(); ++e) {
            if (d[e] == '\"') ++quotes;
            if (d[e] == '>' && quotes % 2 == 0) break;
        }
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
        using value_type = node;
        using difference_type = std::ptrdiff_t;

        const node *root;
        const node *n;

        iterator() = default;
        iterator(const node &n) : root{&n}, n{&n} {
            this->n = root->first_child; // step into children
        }
        const node &operator*() const {
            return *n;
        }
        iterator &operator++() {
            if (n->first_child) {
                n = n->first_child;
            } else if (n->next_sibling) {
                n = n->next_sibling;
            } else {
                while (n && n != root && !n->next_sibling) {
                    n = n->parent;
                }
                if (n && n != root) {
                    n = n->next_sibling;
                } else {
                    n = nullptr;
                }
            }
            return *this;
        }
        auto operator++(int) {
            auto temp = *this;
            operator++();
            return temp;
        }
        bool operator==(auto) const {return n == nullptr;}
    };

    string_type d;
    token_type type;
    std::map<string_type, string_type> attributes;
    std::vector<node*> children;

    node *parent{};
    node *first_child{};
    node *next_sibling{};

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

    bool empty() const { return d.empty(); }
    explicit operator bool() const { return !empty(); }
    bool is(std::string_view tag) const {return (type == token_type::begin_tag || type == token_type::complete_tag) && d == tag || tag.empty(); }
    bool has(std::string_view attr) const {return attributes.contains(attr); }
    const auto *attribute(auto &&attr) const {auto a = attributes.find(attr); return a == attributes.end() ? nullptr : &a->second; }
    std::string_view attribute(auto &&attr, std::string_view default_) const {auto a = attribute(attr); return a ? *a : default_; }
    auto attribute_or_default(auto &&attr, std::string_view default_ = ""sv) const {return attribute(attr, default_); }
    auto name() const {return type == token_type::text ? std::string{} : std::string{d};}
    auto &tag() const {return d;}
    auto value() const {
        static const std::unordered_map<std::string, std::string> m{
            {"&lt;", "<"},
            {"&gt;", ">"},
            {"&amp;", "&"},
            {"&quot;", "\""},
            {"&apos;", "'"},
            {"&nbsp;", " "},
        };

        std::string s{d};
        size_t p{};
        while (1) {
            p = s.find('&', p);
            if (p == -1) {
                break;
            }
            auto e = s.find(';', p);
            if (e == -1) {
                break;
            }
            ++e;
            if (s[p + 1] == '#') {
                int base = 10;
                int off = 2;
                if (s[p + 2] == 'x') {
                    base = 16;
                    off = 3;
                }
                int i;
                if (auto [_,ec] = std::from_chars(s.data() + p + off, s.data() + e, i, base); ec != std::errc{}) {
                    throw;
                }
                if (i == 160) {
                    i = ' ';
                }
                std::string t;
                if (i > 127) {
                    t = std::format("xxx unicode symbol {} xxx", i);
                } else {
                    t += (char)i;
                }
                s.replace(p, e - p, t);
                continue;
            }
            auto it = m.find(s.substr(p, e-p));
            if (it == m.end()) {
                throw;
            }
            s.replace(p, e-p, it->second);
            ++p;
        }
        return s;
    }

    auto text() const {
        std::string t;
        if (type == token_type::text) {
            t += value();
        }
        else {
            for (auto &&n : *this) {
                if (n.type == token_type::text) {
                    t += n.value();
                }
            }
        }
        return t;
    }
    std::string tag_raw() const {
        std::string t;
        if (type == token_type::text) {
            throw;
        } else {
            t += std::format("<{}", tag());
            if (!attributes.empty()) {
                t += " ";
            }
            for (auto &&[k, v] : attributes) {
                t += std::format("{}=\"{}\"", k, v);
            }
            t += ">";
        }
        return t;
    }
    std::string raw() const {
        std::string t;
        if (type == token_type::text) {
            t += value();
        } else {
            t += std::format("<{}", tag());
            if (!attributes.empty()) {
                t += " ";
            }
            for (auto &&[k,v] : attributes) {
                t += std::format("{}=\"{}\"", k, v);
            }
            t += ">";
            for (auto &&n : children) {
                t += n->raw();
            }
            t += std::format("</{}>", tag());
        }
        return t;
    }
    traverse_action traverse(auto &&f, int depth = -1) const {
        auto r = traverse_action::continue_;
        if (depth != -1) {
            if constexpr (requires {f(*this); }) {
                r = f(*this);
            } else {
                r = f(*this, depth);
            }
        }
        if (r == traverse_action::stop) {
            return r;
        }
        if (r != traverse_action::skip_children)
        for (auto &&c : children) {
            if (auto r = c->traverse(f, depth + 1); r == traverse_action::stop) {
                return r;
            }
        }
        ///r = f(*this); // end of tag?
        if (r == traverse_action::stop) {
            return r;
        }
        return traverse_action::continue_;
    }
    const node *find(auto &&attrname, auto &&idname) {
        for (auto &&n : *this) {
            if (auto i = n.attributes.find(attrname); i != n.attributes.end() && i->second == idname) {
                return &n;
            }
        }
        return nullptr;
    }
    iterator begin() const {return *this;}
    int end() const {return 0;}
};

struct root : node {
    std::vector<node> children;

    root(std::string_view page) {
        auto tokens = primitives::html::make_tokens(page);
        children.reserve(tokens.size());
        auto it = tokens.begin();
        auto tree = make_tree(it, tokens.end());
        for (auto &&t : tree) {
            if (t->d == "html"sv) {
                (node&)*this = *t;
                break;
            }
        }
    }
    std::vector<node*> make_tree(auto &it, auto &&end, bool keep_whitespace = false) {
        std::vector<node*> tags;
        while (it != end) {
            auto &[tt, t] = *it++;
            if (t.starts_with("<!DOCTYPE"sv) || tt == token_type::comment) {
                continue;
            }
            if (!keep_whitespace && std::ranges::all_of(t, [](char c) {return c == ' ' || c == '\r' || c == '\n' || c == '\t'; })) {
                continue;
            }
            switch (tt) {
            case token_type::text:
            case token_type::cdata:
                tags.emplace_back(&children.emplace_back(t, tt));
                break;
            case token_type::complete_tag:
                tags.emplace_back(&children.emplace_back(t, tt, 0));
                break;
            case token_type::begin_tag:
                tags.emplace_back(&children.emplace_back(t, tt, 0))->children = make_tree(it, end, keep_whitespace || preserve_space(t));
                continue;
            case token_type::end_tag:
                goto end;
            }
        }
    end:
        for (auto &&t : tags) {
            t->first_child = t->children.empty() ? nullptr : t->children[0];
            for (size_t i{}; auto &&c : t->children) {
                c->parent = t;
                c->next_sibling = i >= t->children.size() - 1 ? nullptr : t->children[i + 1];
                ++i;
            }
        }
        return tags;
    }
};


} // namespace primitives::html
