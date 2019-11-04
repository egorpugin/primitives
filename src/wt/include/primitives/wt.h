// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/string.h>
#include <Wt/WDialog.h>

namespace primitives::wt
{

PRIMITIVES_WT_API
void showDialog(Wt::WObject *parent, std::unique_ptr<Wt::WDialog> dialog);

PRIMITIVES_WT_API
void showDialog(std::unique_ptr<Wt::WDialog> dialog);

PRIMITIVES_WT_API
void showDialog(const String &title, const String &text);

PRIMITIVES_WT_API
void showDialogYesNo(const String &title, const String &text, std::function<void(void)> f);

PRIMITIVES_WT_API
void showDialogNoYes(const String &title, const String &text, std::function<void(void)> f);

PRIMITIVES_WT_API
void showError(const String &text);

PRIMITIVES_WT_API
void showSuccess(const String &text);

}
