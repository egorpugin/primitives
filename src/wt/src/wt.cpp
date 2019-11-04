// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/wt.h>

#include <Wt/WApplication.h>
#include <Wt/WLabel.h>
#include <Wt/WPushButton.h>

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

void showDialog(const String &title, const String &text)
{
    auto d = std::make_unique<Wt::WDialog>(title);
    auto c = d->contents();
    c->addWidget(std::make_unique<Wt::WText>(text));
    c->addWidget(std::make_unique<Wt::WBreak>());
    c->addWidget(std::make_unique<Wt::WBreak>());

    auto ok = c->addWidget(std::make_unique<Wt::WPushButton>("Ok"));
    ok->clicked().connect(d.get(), &Wt::WDialog::accept);

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

void showError(const String &text)
{
    showDialog("Error", text);
}

void showSuccess(const String &text)
{
    showDialog("Success", text);
}

}
