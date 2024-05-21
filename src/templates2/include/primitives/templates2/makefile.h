#pragma once

#include <primitives/filesystem.h>
#include <primitives/templates.h>

struct makefile_am {
    using list = std::vector<std::string>;
    struct if_desc {
        bool if_current{true};
        bool if_branch{};
        bool else_branch{};

        void switch_to_else() {
            if_current = false;
        }
        explicit operator bool() const {
            return if_current ? if_branch : else_branch;
        }
    };

    std::map<std::string, list> variables;
    //
    path current_fn;
    std::vector<if_desc> if_stack;
    enum record_type {
        keyword,
        comment,
        condition,
        empty_line,
        list_,
    };

    makefile_am(const path &fn, auto &&...defs) {
        (load(std::string{defs} += "\n"), ...);
        resolve_variables();

        SwapAndRestore sr{current_fn, fn};
        load(fn);
    }

    void load(const path &fn) {
        load(read_file(fn));
        resolve_variables();
    }
    void resolve_variables() {
        for (auto &&[_,lst] : variables) {
            for (int i = 0; i < lst.size(); ++i) {
                auto &l = lst[i];
                if (l.starts_with("$("sv) && l.ends_with(")"sv)) {
                    auto k = l.substr(2, l.size() - 3);
                    for (auto &&l2 : variables[k]) {
                        lst.insert(lst.begin() + i + 1, l2);
                    }
                    lst.erase(lst.begin() + i);
                    --i;
                }
            }
        }
    }

    void next_record(auto &p) {
        eat_space(p);
        if (*p == '#') {
            eat_line(p);
            return;
        }
        if (*p == '\n') {
            eat_line(p);
            return;
        }
        auto line = eat_token(p);
        bool parse_cond = if_stack.empty() || if_stack.back();
        if (line.starts_with("if"sv)) {
            eat_space(p);
            if_desc d{};
            if (*p == '!') {
                ++p;
                auto cond = eat_line(p);
                d.if_branch = !variables.contains(std::string{cond}) && parse_cond;
            } else {
                auto cond = eat_line(p);
                d.if_branch = variables.contains(std::string{cond}) && parse_cond;
            }
            d.else_branch = !d.if_branch && parse_cond;
            if_stack.push_back(d);
            return;
        }
        if (line.starts_with("else"sv)) {
            if_stack.back().switch_to_else();
            eat_line(p);
            return;
        }
        if (line.starts_with("endif"sv)) {
            if_stack.pop_back();
            eat_line(p);
            return;
        }
        if (!if_stack.empty() && !if_stack.back()) {
            eat_line(p);
            return;
        }
        if (line.starts_with("include"sv)) {
            eat_space(p);
            auto file = eat_line(p);
            load(current_fn.parent_path() / file);
            return;
        }

        eat_space(p);
        if (*p == '=') {
            ++p;
            eat_space(p);
            variables[std::string{line}] = eat_list(p);
            return;
        }
        if (*p == ':') {
            variables[std::string{line}] = eat_commands(p);
            return;
        }
        if (*p == '+' && *(p+1) == '=') {
            ++p;
            ++p;
            eat_space(p);
            variables[std::string{line}].append_range(eat_list(p));
            return;
        }

        throw std::runtime_error{"unimplemented: unexpected symbol"};
    }
    list eat_commands(auto &p) {
        eat_line(p);
        list l;
        while (*p == '\t') {
            ++p;
            l.push_back(std::string{eat_line(p)});
        }
        return l;
    }
    list eat_list(auto &p) {
        auto start = p;
        while (1) {
            auto line = eat_line(p);
            if (line.empty()) {
                break;
            }
            if (line.back() != '\\') {
                break;
            }
        }
        return split_string(std::string(start,p), " \t\\");
    }
    void eat(auto &p, char c) {
        if (*p != c) {
            throw std::runtime_error{"bad char"};
        }
        ++p;
    }
    void eat_space(auto &p) {
        while (isspace(*p) && *p != '\n') {
            ++p;
        }
    }
    std::string_view eat_token(auto &p) {
        auto start = p;
        while (*p != ':' && !isspace(*p) && *p != '+' && *p != '=') {
            ++p;
        }
        return {start, p};
    }
    std::string_view eat_until(auto &p, char c) {
        auto start = p;
        while (*p != c) {
            ++p;
        }
        return {start, p};
    }
    std::string_view eat_line(auto &p) {
        auto res = eat_until(p, '\n');
        ++p;
        return res;
    }
    void load(const std::string &s) {
        auto p = s.data();
        auto end = s.data() + s.size();
        while (p < end) {
            next_record(p);
        }
    }
};
