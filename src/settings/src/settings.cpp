// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/settings.h>

namespace primitives
{

void Settings::load(const path &fn, SettingsType type)
{

}

void Settings::save(const path &fn)
{

}

}

primitives::SettingStorage<primitives::Settings> &getSettings()
{
    static primitives::SettingStorage<primitives::Settings> settings;
    return settings;
}
