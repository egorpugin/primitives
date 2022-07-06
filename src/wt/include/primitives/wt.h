// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/string.h>

#include "wt/headers.h"

namespace primitives::wt
{

// deprecated, for compat
using center_content_widget = Wt::WContainerWidget;

struct right_menu_widget : Wt::WContainerWidget {
    right_menu_widget(){
        // setAttributeValue("style", "display: flex; padding-top: 10px;");
        setAttributeValue("style", "display: flex;");
        // auto L = std::make_unique<Wt::WHBoxLayout>();

        // Create a stack where the contents will be located.
        auto contents = std::make_unique<Wt::WStackedWidget>();
        contents->setAttributeValue("style", "width: 100%; padding-left: 10px;");

        menu = addWidget(std::make_unique<Wt::WMenu>(contents.get()));
        menu->setStyleClass("nav nav-pills nav-stacked flex-column");
        // menu->setInternalPathEnabled("/" + (unres_pkg.empty() ? p.toString("/") : unres_pkg));
        // menu->setWidth(150);

        addWidget(std::move(contents));
    }

    Wt::WMenuItem *addTab(const Wt::WString &name, std::unique_ptr<Wt::WWidget> w,
                          Wt::ContentLoading policy = Wt::ContentLoading::Lazy){
        auto mi = menu->addItem(name, std::move(w), policy);
        return mi;
    }
    Wt::WMenuItem *addTab(const Wt::WString &name, const String &url, std::unique_ptr<Wt::WWidget> w,
                          Wt::ContentLoading policy = Wt::ContentLoading::Lazy){
        auto mi = addTab(name, std::move(w), policy);
        mi->setPathComponent(url);
        mi->clicked().connect([mi]() {
            Wt::WApplication::instance()->setInternalPath(mi->pathComponent(), false);
        });
        return mi;
    }
    Wt::WMenuItem *addTab(const Wt::WString &name, const String &base_url, const String &url, std::unique_ptr<Wt::WWidget> w,
                          Wt::ContentLoading policy = Wt::ContentLoading::Lazy){
        auto mi = addTab(name, std::move(w), policy);
        mi->setPathComponent(url);
        mi->clicked().connect([base_url, mi]() {
            Wt::WApplication::instance()->setInternalPath(base_url + "/" + mi->pathComponent(), false);
        });
        return mi;
    }

//private:
    Wt::WMenu *menu;
};

template <typename Widget>
struct labeled_widget : public Wt::WContainerWidget {
    Wt::WLabel *label = nullptr;
    Widget *widget = nullptr;
    Wt::WLabel *underline_text = nullptr;

    labeled_widget(const Wt::WString &label, const std::string &underline_text = {}) {
        constexpr auto fw = std::is_base_of_v<Wt::WFormWidget, Widget>;

        this->label = addWidget(std::make_unique<Wt::WLabel>(label));
        if constexpr (!fw) {
            //addWidget(std::make_unique<Wt::WBreak>());
            //addWidget(std::make_unique<Wt::WBreak>());
            this->label->setAttributeValue("style", "font-weight: bold; margin-bottom: 5px;");
        }
        widget = addWidget(std::make_unique<Widget>());
        if constexpr (fw)
            this->label->setBuddy(widget);
        if (!underline_text.empty()) {
            this->underline_text = addWidget(std::make_unique<Wt::WLabel>(underline_text));
            this->underline_text->setAttributeValue("style", "font-size: 10px; color: gray;");
            setAttributeValue("style", "margin-bottom: 10px;");
        }
        addWidget(std::make_unique<Wt::WBreak>());
    }
};

template <typename W>
void addAttributeValue(W &w, const std::string &key, const std::string &value) {
    auto s = w.attributeValue(key);
    s += value;
    w.setAttributeValue(key, s);
}

// flex layouts
struct layout : Wt::WContainerWidget {
    enum type {
        horizontal,
        vertical,
    };

    type t;
    int spacing;

    using base = Wt::WContainerWidget;

    layout(type t, int spacing = 0) : t(t), spacing(spacing) {
        addStyleClass("flex");
        if (t == type::horizontal)
            addAttributeValue(*this, "style", "flex-flow: row;");
        else if (t == type::vertical)
            addAttributeValue(*this, "style", "flex-flow: column; height: 100%;");
    }
    template <typename T>
    T *addWidget(std::unique_ptr<T> w, int stretch = 0) {
        bool add_space = count() && spacing;

        auto v = base::addWidget<T>(std::move(w));
        auto flex = std::to_string(stretch) + " " + std::to_string(stretch) + " auto;";
        addAttributeValue(*v, "style", "flex: " + flex);
        if (add_space) {
            if (t == type::horizontal)
                addAttributeValue(*v, "style", "margin-left: " + std::to_string(spacing) + "px;");
            else if (t == type::vertical)
                addAttributeValue(*v, "style", "margin-top: " + std::to_string(spacing) + "px;");
        }
        return v;
    }
};

}
