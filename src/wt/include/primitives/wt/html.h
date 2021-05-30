// Copyright (C) 2021 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/overload.h>

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
    int stretch = 0;

    w(auto && ... args)
        : w_{std::make_unique<widget_type>(std::forward<decltype(args)>(args)...)} {
    }
    w(widget_type **p) : w{} {
        if (p)
            *p = get_ptr();
    }

    auto get() { return std::move(w_); }
    widget_type &get_ref() { return *w_; }
    widget_type *get_ptr() { return &get_ref(); }

    // for the last element in the chain
    decltype(auto) operator/(int stretch) && {
        this->stretch = stretch;
        return std::move(*this);
    }
    decltype(auto) operator[](auto &&args) && {
        hana::for_each(std::move(args), [this](auto &&a) {
            if constexpr (std::is_base_of_v<WLayout, std::decay_t<decltype(a)>::widget_type>)
                get_ref().addLayout(std::move(a.get()), a.stretch);
            else
                get_ref().addWidget(std::move(a.get()), a.stretch);
        });
        return std::move(*this);
    }
};

decltype(auto) operator+(auto &&w) {
    return hana::make_tuple(std::move(w));
}
decltype(auto) operator+(auto &&a, auto &&b) {
    if constexpr (!hana::Foldable<decltype(b)>::value)
        return hana::append(std::move(a), std::move(b));
    else
        return hana::concat(std::move(a), std::move(b));
}
decltype(auto) operator/(auto &&a, int stretch) {
    hana::back(a).stretch = stretch;
    return std::move(a);
}

// some predefined things, (re)move?

auto vl(auto && ... args) {
    return w<WVBoxLayout>(std::forward<decltype(args)>(args)...);
}
auto hl(auto && ... args) {
    return w<WHBoxLayout>(std::forward<decltype(args)>(args)...);
}

auto br(auto && ... args) {
    return w<WBreak>(std::forward<decltype(args)>(args)...);
}

auto buddy(auto &&a, auto &&b) {
    a.get_ref().setBuddy(b.get_ptr());
    return hana::make_tuple(std::move(a), std::move(b));
}

} // namespace primitives::wt::html
