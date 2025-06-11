#pragma once

#include "mmap2.h"

#include <primitives/filesystem.h>

namespace primitives::templates2 {

struct pdf {
    struct xref {
        struct record {
            size_t offset;
            int generation;
            bool valid;

            std::string_view data;
        };
        std::map<int, xref::record> data;
    };
    struct reference {
        int id;
        int generation;
    };

    mmap_file<const char> m;
    xref r;

    pdf(const path &p) : m{ p } {
        read_trailer();
        std::string_view v{m.p, m.sz};
        auto startxref = "startxref"sv;
        auto pos = v.rfind(startxref);
        if (pos == -1) {
            throw std::runtime_error{"bad pdf"};
        }
        auto i = parse_pos(pos + startxref.size());
        auto xrefsv = "xref"sv;
        auto xref_data = v.substr(i);
        if (!xref_data.starts_with(xrefsv)) {
            throw std::runtime_error{ "bad pdf" };
        }
        xref_data.remove_prefix(xrefsv.size());
        eat_space(xref_data);
        int sz{299};
        pos = xref_data.data() - m.p;
        while (r.data.empty() || r.data.rbegin()->first + 1 != sz) {
            auto start = parse_int(pos);
            auto count = parse_int(pos);
            auto n = count;
            while (n--) {
                auto &rec = r.data[start + count - n - 1];
                rec.offset = parse_int(pos);
                rec.generation = parse_int(pos);
                rec.valid = m.p[pos += 1] == 'n';
                constexpr auto endo = "endobj"sv;
                rec.data = v.substr(rec.offset);
                rec.data = rec.data.substr(0, rec.data.find(endo));
                ++pos;
                ;
            }
        }
    }
    std::string_view rfind_block(auto &&name) {
        std::string_view v{ m.p, m.sz };
        auto pos = v.rfind(name);
        if (pos == -1) {
            throw std::runtime_error{ "bad pdf" };
        }
        return v.substr(pos + name.size());
    }
    void read_trailer() {
        auto t = rfind_block("trailer"sv);
        read_map(t);
    }
    void read_map(auto &&from) {
        eat_space(from);
        auto ll = "<<"sv;
        if (!from.starts_with(ll)) {
            throw std::runtime_error{ "bad pdf" };
        }
        from.remove_prefix(ll.size());
        auto key = read_map_key(from);
        auto value = read_token(from);
    }
    std::string_view read_map_key(auto &from) {
        eat_space(from);
        if (!from.starts_with('/')) {
            throw std::runtime_error{ "bad pdf" };
        }
        auto p = from.data();
        while (*p && !isspace(*p)) ++p;
        std::string_view r{ from.data(), p };
        from.remove_prefix(p - from.data());
        return r;
    }
    std::string_view read_token(auto &from) {
        eat_space(from);
        //
        //return r;
        return {};
    }
    int parse_pos(size_t p) {
        auto i = parse_int(p);
        if (i >= m.sz) {
            throw std::runtime_error{ "bad pos" };
        }
        return i;
    }
    int parse_int(auto &p) {
        eat_space(p);
        std::string_view v{ m.p, m.sz };
        int i;
        if (auto [ptr, ec] = std::from_chars(v.data() + p, v.data() + v.size(), i, 10); ec != std::errc{}) {
            throw std::runtime_error{ "bad pdf" };
        } else {
            p += ptr - m.p - p;
        }
        return i;
    }
    void eat_space(auto &p) {
        while (p < m.sz && isspace(m[p])) {
            ++p;
        }
    }
    void eat_space(auto &p) requires requires {p.data();} {
        while (!p.empty() && isspace(p[0])) {
            p.remove_prefix(1);
        }
    }
};

}
