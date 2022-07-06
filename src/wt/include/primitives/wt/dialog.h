// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/string.h>
#include <primitives/wt/html.h>

#include "headers.h"

namespace primitives::wt
{

inline void showDialog(Wt::WObject *parent, std::unique_ptr<Wt::WDialog> dialog) {
    auto d = parent->addChild(std::move(dialog));
    //d->finished().connect([d](Wt::DialogCode code) { d->removeFromParent(); });
    d->finished().connect([parent, d = dialog.get()](Wt::DialogCode) { parent->removeChild(d); });
    d->show();

    // after show
    //getSwApplication()->navigation2->setAttributeValue("style", "z-index: 0;");
}

inline void showDialog(std::unique_ptr<Wt::WDialog> dialog) {
    showDialog(Wt::WApplication::instance(), std::move(dialog));
}

inline auto make_default_dialog() {
    return std::make_unique<Wt::WDialog>();
}
inline auto make_default_dialog(const String &title) {
    auto d = make_default_dialog();
    d->setWindowTitle(title);
    return d;
}

std::unique_ptr<Wt::WDialog> make_default_dialog(const String &title, std::unique_ptr<Wt::WWidget> w){
    using namespace primitives::wt::html;

    auto d = make_default_dialog(title);
    auto c = d->contents();

    WPushButton *b;
    w<WContainerWidget> x(*c);
    x
    [
        + vl(6)
        [
            + w<WWidget>(std::move(in_w))
            + stretch()
            + hl(6)
            [
                + w<WPushButton>(d->tr("close")).ptr(&b)
                + stretch()
            ]
        ]
    ];
    c->addWidget(std::make_unique<Wt::WBreak>());
    b->clicked().connect([d = d.get()]() {
        d->accept();
    });
    return d;
}

inline auto showDialog(const String &title, std::unique_ptr<Wt::WWidget> w) {
    return showDialog(make_default_dialog(title, std::move(w)));
}
inline auto showDialog(std::unique_ptr<Wt::WWidget> w) {
    return showDialog({}, std::move(w));
}

void showDialog(const String &title, const String &text, std::function<void(void)> f = {})
{
    auto d = std::make_unique<Wt::WDialog>(title);
    auto c = d->contents();
    c->addWidget(std::make_unique<Wt::WText>(text));
    c->addWidget(std::make_unique<Wt::WBreak>());
    c->addWidget(std::make_unique<Wt::WBreak>());

    auto ok = c->addWidget(std::make_unique<Wt::WPushButton>("Ok"));
    ok->clicked().connect([d = d.get(), f]{ d->accept(); if (f) f(); });

    showDialog(std::move(d));
}

static auto showDialogYesNo1(const String &title, const String &text, std::function<void(void)> f)
{
    auto d = std::make_unique<Wt::WDialog>(title);
    auto c = d->contents();
    c->addWidget(std::make_unique<Wt::WLabel>(text));
    c->addWidget(std::make_unique<Wt::WBreak>());
    c->addWidget(std::make_unique<Wt::WBreak>());

    auto y = c->addWidget(std::make_unique<Wt::WPushButton>("Yes"));
    y->setAttributeValue("style", "width: 100px; margin-right: 10px;");
    y->clicked().connect([d = d.get(), f]{ d->accept(); if (f) f(); });
    auto n = c->addWidget(std::make_unique<Wt::WPushButton>("No"));
    n->setAttributeValue("style", "width: 100px; margin-right: 10px;");
    n->clicked().connect(d.get(), &Wt::WDialog::reject);

    showDialog(std::move(d));
    return std::tuple{ y,n };
}

void showDialogYesNo(const String &title, const String &text, std::function<void(void)> f = {})
{
    auto [y, n] = showDialogYesNo1(title, text, f);
    y->addStyleClass("btn-success");
    n->addStyleClass("btn-danger");
}

void showDialogNoYes(const String &title, const String &text, std::function<void(void)> f = {})
{
    auto [n, y] = showDialogYesNo1(title, text, f);
    y->addStyleClass("btn-success");
    n->addStyleClass("btn-danger");
}

void showError(const String &text, std::function<void(void)> f = {})
{
    showDialog("Error", text, f);
}

void showSuccess(const String &text, std::function<void(void)> f = {})
{
    showDialog("Success", text, f);
}

} // namespace primitives::wt
