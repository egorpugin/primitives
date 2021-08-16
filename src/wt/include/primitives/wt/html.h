// Copyright (C) 2021 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/exceptions.h>
#include <primitives/overload.h>
#include <primitives/wt.h>

#include <boost/hana/append.hpp>
#include <boost/hana/back.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/tuple.hpp>
#include <Wt/WContainerWidget.h>
#include <Wt/WVBoxLayout.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WWidget.h>

namespace primitives::wt::html {

using namespace ::Wt;
namespace hana = boost::hana;

template <typename T>
struct w {
    using widget_type = T;

    std::unique_ptr<widget_type> w_;
    widget_type *wref_ = nullptr;
    // settings
    int stretch_ = 0;
    WString text_;
    String url_;
    bool tab_ = false;

    w(auto && ... args)
        : w_{std::make_unique<widget_type>(std::forward<decltype(args)>(args)...)} {
    }
    w(std::unique_ptr<widget_type> in)
        : w_{std::move(in)} {
    }
    w(widget_type &in)
        : wref_{&in} {
    }

    auto get() {
        if (wref_)
            throw SW_LOGIC_ERROR("cannot get owning ptr from ref widget");
        return std::move(w_);
    }
    widget_type &get_ref() {
        return wref_ ? *wref_ : *w_;
    }
    widget_type *get_ptr() { return &get_ref(); }

    decltype(auto) ptr(widget_type **p) && {
        if (p)
            *p = get_ptr();
        return std::move(*this);
    }
    decltype(auto) stretch(int s) && {
        stretch_ = s;
        return std::move(*this);
    }
    decltype(auto) text(WString &&t) && {
        text_ = t;
        return std::move(*this);
    }
    decltype(auto) url(String &&t) && {
        url_ = t;
        return std::move(*this);
    }
    decltype(auto) id(String &&t) && {
        get_ref().setId(t);
        return std::move(*this);
    }
    decltype(auto) tab(WString &&t) && {
        text_ = t;
        tab_ = true;
        return std::move(*this);
    }
    decltype(auto) tab(WString &&t, String &&u) && {
        text_ = t;
        url_ = u;
        tab_ = true;
        return std::move(*this);
    }
    decltype(auto) style(auto &&s) && {
        addAttributeValue(get_ref(), "style", s);
        return std::move(*this);
    }
    decltype(auto) style_class(auto && ... s) && {
        (get_ref().addStyleClass(s), ...);
        return std::move(*this);
    }
    decltype(auto) setup(auto &&f) && {
        f(get_ref());
        return std::move(*this);
    }

    // for the last element in the chain
    decltype(auto) operator/(int stretch) && {
        stretch_ = stretch;
        return std::move(*this);
    }
    void add_members(auto &&args) {
        hana::for_each(std::move(args), [this](auto &&a) {
            auto &w = get_ref();
            if constexpr (std::is_same_v<widget_type, primitives::wt::right_menu_widget>) {
                auto t = a.url_.empty() ? w.addTab(a.text_, std::move(a.get())) : w.addTab(a.text_, a.url_, std::move(a.get()));
                if (a.tab_ && WApplication::instance()->internalPath() == a.url_)
                    w.menu->select(t);
            }
            else if constexpr (std::is_same_v<widget_type, WContainerWidget>) {
                auto v = w.addWidget(std::move(a.get()));
                //if (!text_.empty())
                //v->setText(text_);
            }
            else {
                auto v = w.addWidget(std::move(a.get()), a.stretch_);
                //if (!text_.empty())
                //v->setText(text_);
            }
        });
    }
    decltype(auto) operator[](auto &&args) & {
        add_members(std::move(args));
        return *this;
    }
    decltype(auto) operator[](auto &&args) && {
        add_members(std::move(args));
        return std::move(*this);
    }
};
template <typename T> w(T &)->w<T>;
//template <typename T> w(T &&)->w<T>;

decltype(auto) operator+(auto &&w) {
    if constexpr (hana::Foldable<decltype(w)>::value)
        return std::move(w);
    else
        return hana::make_tuple(std::move(w));
}
decltype(auto) operator+(auto &&a, auto &&b) {
    if constexpr (hana::Foldable<decltype(b)>::value)
        return hana::concat(std::move(a), std::move(b));
    else
        return hana::append(std::move(a), std::move(b));
}
decltype(auto) operator/(auto &&a, int stretch) {
    hana::back(a).stretch_ = stretch;
    return std::move(a);
}

// some predefined things, (re)move?

auto vl(auto && ... args) {
    auto v = w<layout>(layout::type::vertical, std::forward<decltype(args)>(args)...);
    return v;
}
auto hl(auto && ... args) {
    auto v = w<layout>(layout::type::horizontal, std::forward<decltype(args)>(args)...);
    return v;
}

auto br(auto && ... args) {
    return w<WBreak>(std::forward<decltype(args)>(args)...);
}

template <typename T>
auto buddy(w<T> &&a, auto &&b) {
    a.get_ref().setBuddy(b.get_ptr());
    return hana::make_tuple(std::move(a), std::move(b));
}
auto buddy(auto &&a, auto &&b) {
    return buddy(w<WLabel>(a), std::move(b));
}

inline auto stretch() {
    return w<WContainerWidget>() / 1;
}

} // namespace primitives::wt::html
