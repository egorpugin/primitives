// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/wt.h>

#include <Wt/WApplication.h>
#include <Wt/WDialog.h>
#include <Wt/WLabel.h>
#include <Wt/WMenu.h>
#include <Wt/WMenuItem.h>
#include <Wt/WPushButton.h>
#include <Wt/WStackedWidget.h>

namespace primitives::wt
{

void showDialog(Wt::WObject *parent, std::unique_ptr<Wt::WDialog> dialog)
{
    auto d = parent->addChild(std::move(dialog));
    //d->finished().connect([d](Wt::DialogCode code) { d->removeFromParent(); });
    d->show();

    // after show
    //getSwApplication()->navigation2->setAttributeValue("style", "z-index: 0;");
}

void showDialog(std::unique_ptr<Wt::WDialog> dialog)
{
    showDialog(Wt::WApplication::instance(), std::move(dialog));
}

void showDialog(const String &title, const String &text, std::function<void(void)> f)
{
    auto d = std::make_unique<Wt::WDialog>(title);
    auto c = d->contents();
    c->addWidget(std::make_unique<Wt::WText>(text));
    c->addWidget(std::make_unique<Wt::WBreak>());
    c->addWidget(std::make_unique<Wt::WBreak>());

    auto ok = c->addWidget(std::make_unique<Wt::WPushButton>("Ok"));
    ok->clicked().connect([d = d.get(), f]{ d->accept(); f(); });

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
    y->clicked().connect([d = d.get(), f]{ d->accept(); f(); });
    auto n = c->addWidget(std::make_unique<Wt::WPushButton>("No"));
    n->setAttributeValue("style", "width: 100px; margin-right: 10px;");
    n->clicked().connect(d.get(), &Wt::WDialog::reject);

    showDialog(std::move(d));
    return std::tuple{ y,n };
}

void showDialogYesNo(const String &title, const String &text, std::function<void(void)> f)
{
    auto [y, n] = showDialogYesNo1(title, text, f);
    y->addStyleClass("btn-success");
    n->addStyleClass("btn-danger");
}

void showDialogNoYes(const String &title, const String &text, std::function<void(void)> f)
{
    auto [n, y] = showDialogYesNo1(title, text, f);
    y->addStyleClass("btn-success");
    n->addStyleClass("btn-danger");
}

void showError(const String &text, std::function<void(void)> f)
{
    showDialog("Error", text, f);
}

void showSuccess(const String &text, std::function<void(void)> f)
{
    showDialog("Success", text, f);
}

right_menu_widget::right_menu_widget() {
    // setAttributeValue("style", "display: flex; padding-top: 10px;");
    setAttributeValue("style", "display: flex;");
    // auto L = std::make_unique<Wt::WHBoxLayout>();

    // Create a stack where the contents will be located.
    auto contents = std::make_unique<Wt::WStackedWidget>();
    contents->setAttributeValue("style", "width: 100%; padding-left: 10px;");

    menu = addWidget(std::make_unique<Wt::WMenu>(contents.get()));
    menu->setStyleClass("nav nav-pills nav-stacked");
    // menu->setInternalPathEnabled("/" + (unres_pkg.empty() ? p.toString("/") : unres_pkg));
    // menu->setWidth(150);

    addWidget(std::move(contents));
}

Wt::WMenuItem *right_menu_widget::addTab(const Wt::WString &name, std::unique_ptr<Wt::WWidget> w,
                                         Wt::ContentLoading policy) {
    auto mi = menu->addItem(name, std::move(w), policy);
    return mi;
}

Wt::WMenuItem *right_menu_widget::addTab(const Wt::WString &name, const String &url,
    std::unique_ptr<Wt::WWidget> w, Wt::ContentLoading policy) {
    auto mi = addTab(name, std::move(w), policy);
    mi->setPathComponent(url);
    mi->clicked().connect([mi]() {
        Wt::WApplication::instance()->setInternalPath(mi->pathComponent(), false);
    });
    return mi;
}

Wt::WMenuItem *right_menu_widget::addTab(const Wt::WString &name, const String &base_url, const String &url,
                                         std::unique_ptr<Wt::WWidget> w, Wt::ContentLoading policy) {
    auto mi = addTab(name, std::move(w), policy);
    mi->setPathComponent(url);
    mi->clicked().connect([base_url, mi]() {
        Wt::WApplication::instance()->setInternalPath(base_url + "/" + mi->pathComponent(), false);
    });
    return mi;
}

}
