// Copyright (C) 2021 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/overload.h>

#include <variant>

namespace primitives::wt::html {

using namespace ::Wt;

using list_variant = std::variant<std::unique_ptr<WWidget>, std::unique_ptr<WLayout>>;
using wlist = std::vector<list_variant>;

struct div {
    std::unique_ptr<WContainerWidget> c{ std::make_unique<WContainerWidget>() };
    bool layout_set{ false };

    div() = default;
    div(WContainerWidget *&v) { v = c.get(); }

    decltype(auto) operator[](div &&w) && {
        add_widget(std::move(w.c));
        return std::move(*this);
    }

    decltype(auto) operator[](wlist &&w) && {
        for (auto &&e : w) {
            std::visit(overload(
                [this](std::unique_ptr<WWidget> &&e) {
                add_widget(std::move(e));
            },
                [this](std::unique_ptr<WLayout> &&e) {
                throw SW_LOGIC_ERROR("cannot add layout to container");
            }),
                std::move(e));
        }
        return std::move(*this);
    }

    void add_widget(auto &&w) {
        if (layout_set)
            throw SW_LOGIC_ERROR("cannot add widgets after layout is set");
        c->addWidget(std::move(w));
    }

    void set_layout(auto &&l) {
        if (layout_set)
            throw SW_LOGIC_ERROR("cannot set layout twice");
        c->setLayout(std::move(l));
        layout_set = true;
    }
};

template <typename Layout>
struct l {
    std::unique_ptr<Layout> c{ std::make_unique<Layout>() };

    //operator std::unique_ptr<Layout>() { std::move(c); }

    l() = default;
    l(Layout *&v) { v = c.get(); }

    template <typename Widget>
    decltype(auto) operator[](Widget &&w) && {
        add_widget(std::move(w));
        return std::move(*this);
    }

    decltype(auto) operator[](wlist &&w) && {
        for (auto &&e : w) {
            std::visit(overload(
                [this](std::unique_ptr<WWidget> &&e) {
                add_widget(std::move(e));
            },
                [this](std::unique_ptr<WLayout> &&e) {
                add_layout(std::move(e));
            }),
                std::move(e));
        }
        return std::move(*this);
    }

    void add_widget(auto &&w) {
        c->addWidget(std::move(w));
    }

    void add_layout(auto &&w) {
        c->addLayout(std::move(w));
    }
};
using vl = l<WVBoxLayout>;
using hl = l<WHBoxLayout>;
// grid layout
// border layout

template <typename T>
concept is_layout = std::is_same_v<T, vl> || std::is_same_v<T, hl>;
template <typename T>
concept html_element = std::is_same_v<T, div> || is_layout<T>;

// mkw? mw?
template <typename Widget>
auto w() {
    return std::make_unique<Widget>();
}

auto br() {
    return w<WBreak>();
}

template <typename T>
decltype(auto) operator+(html_element auto &&e, T &&w) {
    if constexpr (std::is_same_v<T, div>)
        e.add_widget(std::move(w.c));
    else if constexpr (is_layout<T>) {
        if constexpr (std::is_same_v<std::decay_t<decltype(e)>, div>)
            e.set_layout(std::move(w.c));
        else
            e.add_layout(std::move(w.c));
    }
    else
        e.add_widget(std::move(w));
    return std::move(e);
}

template <typename T>
decltype(auto) operator+(T &&w) {
    wlist l;
    l.emplace_back(std::move(w));
    return l;
}

template <typename T>
decltype(auto) operator+(wlist &&l, T &&w) {
    if constexpr (is_layout<T>) {
        l.emplace_back(std::move(w.c));
    }
    else
        l.emplace_back(std::move(w));
    return std::move(l);
}

}

/*
*

#include <primitives/wt/html.h>

    namespace html = primitives::wt::html;
    using namespace html;
    using html::div;

    Wt::WContainerWidget *test;
    Wt::WHBoxLayout *l;
    auto c =
        div(test)
        + hl(l)
        [
            + w<WTreeView>()
            + w<WTreeView>()
            + vl()
            [
                + w<WTreeView>()
                + br()
                + br()
                + br()
                + w<WTreeView>()
            ]
            + hl()
            [
                + w<WTreeView>()
                + br()
                + w<WTreeView>()
            ]
        ]
    ;

*/
