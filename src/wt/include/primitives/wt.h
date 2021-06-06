// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/string.h>

#include <Wt/WBreak.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WLabel.h>

namespace Wt
{
class WMenu;
class WMenuItem;
class WDialog;
}

namespace primitives::wt
{

PRIMITIVES_WT_API
void showDialog(Wt::WObject *parent, std::unique_ptr<Wt::WDialog> dialog);

PRIMITIVES_WT_API
void showDialog(std::unique_ptr<Wt::WDialog> dialog);

PRIMITIVES_WT_API
void showDialog(const String &title, const String &text, std::function<void(void)> f = {});

PRIMITIVES_WT_API
void showDialogYesNo(const String &title, const String &text, std::function<void(void)> f = {});

PRIMITIVES_WT_API
void showDialogNoYes(const String &title, const String &text, std::function<void(void)> f = {});

PRIMITIVES_WT_API
void showError(const String &text, std::function<void(void)> f = {});

PRIMITIVES_WT_API
void showSuccess(const String &text, std::function<void(void)> f = {});

// deprecated, for compat
using center_content_widget = Wt::WContainerWidget;

struct PRIMITIVES_WT_API right_menu_widget : Wt::WContainerWidget {
    right_menu_widget();

    Wt::WMenuItem *addTab(const Wt::WString &name, std::unique_ptr<Wt::WWidget> w,
                          Wt::ContentLoading policy = Wt::ContentLoading::Lazy);
    Wt::WMenuItem *addTab(const Wt::WString &name, const String &url, std::unique_ptr<Wt::WWidget> w,
                          Wt::ContentLoading policy = Wt::ContentLoading::Lazy);
    Wt::WMenuItem *addTab(const Wt::WString &name, const String &base_url, const String &url, std::unique_ptr<Wt::WWidget> w,
                          Wt::ContentLoading policy = Wt::ContentLoading::Lazy);

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
        addAttributeValue(*this, "style", "display: flex;");
        if (t == type::horizontal)
            addAttributeValue(*this, "style", "flex-flow: row;");
        else if (t == type::vertical)
            addAttributeValue(*this, "style", "flex-flow: column;");
    }
    template <typename T>
    T *addWidget(std::unique_ptr<T> w, int stretch = 0) {
        bool add_space = count() && spacing;

        auto v = base::addWidget(std::move(w));
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
