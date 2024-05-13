#pragma once

#include <primitives/filesystem.h>
#include <primitives/templates.h>

struct makefile_am {
    using list = std::vector<std::string>;

    std::map<std::string, list> variables;
    //
    path current_fn;
    int if_ = 0;
    const char *p{}, *end{};
    enum record_type {
        keyword,
        comment,
        condition,
        empty_line,
        list_,
    };

    makefile_am(const path &fn) {
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

    void next_record() {
        eat_space();
        if (*p == '#') {
            eat_line();
            return comment;
        }
        if (*p == '\n') {
            eat_line();
            return empty_line;
        }
        auto line = eat_token();
        if (line.starts_with("if"sv)) {
            ++if_;
            eat_line();
            return;
        }
        if (line.starts_with("endif"sv)) {
            --if_;
            eat_line();
            return condition;
        }
        if (if_) {
            eat_line();
            return;
        }
        if (line.starts_with("include"sv)) {
            eat_space();
            auto file = eat_line();
            load(current_fn.parent_path() / file);
            return;
        }

        eat_space();
        if (*p == '=') {
            ++p;
            eat_space();
            variables[std::string{line}] = eat_list();
            return;
        }
        if (*p == ':') {
            variables[std::string{line}] = eat_commands();
            return;
        }
        if (*p == '+' && *(p+1) == '=') {
            ++p;
            ++p;
            eat_space();
            variables[std::string{line}].append_range(eat_list());
            return;
        }

        throw std::runtime_error{"unimplemented: unexpected symbol"};
    }
    list eat_commands() {
        eat_line();
        list l;
        while (*p == '\t') {
            ++p;
            l.push_back(std::string{eat_line()});
        }
        return l;
    }
    list eat_list() {
        auto start = p;
        while (1) {
            auto line = eat_line();
            if (line.empty()) {
                break;
            }
            if (line.back() != '\\') {
                break;
            }
        }
        return split_string(std::string(start,p), " \t\\");
    }
    void eat(char c) {
        if (*p != c) {
            throw std::runtime_error{"bad char"};
        }
        ++p;
    }
    void eat_space() {
        while (isspace(*p) && *p != '\n') {
            ++p;
        }
    }
    std::string_view eat_token() {
        auto start = p;
        while (*p != ':' && !isspace(*p) && *p != '+' && *p != '=') {
            ++p;
        }
        return {start, p};
    }
    std::string_view eat_until(char c) {
        auto start = p;
        while (*p != c) {
            ++p;
        }
        return {start, p};
    }
    std::string_view eat_line() {
        auto res = eat_until('\n');
        ++p;
        return res;
    }
    void load(const std::string &s) {
        p = s.data();
        end = s.data() + s.size();
        while (p < end) {
            next_record();
        }
    }
};
