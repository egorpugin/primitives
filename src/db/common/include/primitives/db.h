// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/schema_manager.h>
#include <primitives/stdcompat/optional.h>

namespace primitives::db
{

/// lite db: has execute() call
/// kv db accessor in column manner: has getValue()/setValue() calls
struct PRIMITIVES_DB_COMMON_API LiteDatabase
{
    virtual ~LiteDatabase() = default;

    virtual void execute(const String &) = 0;

    template <typename T>
    optional<T> getValue(const String &key)
    {
        checkKey(key);
        auto v = getValueByKey(key);
        if (!v)
            return {};
        if constexpr (std::is_same_v<T, int>)
            return std::stoi(v.value());
        else if constexpr (std::is_same_v<T, double>)
            return std::stod(v.value());
        else
            return v;
    }

    template <typename T>
    T getValue(const String &key, const T &default_)
    {
        checkKey(key);
        auto v = getValueByKey(key);
        if (!v)
        {
            setValue(key, default_);
            return default_;
        }
        if constexpr (std::is_same_v<T, int>)
            return std::stoi(v.value());
        else if constexpr (std::is_same_v<T, double>)
            return std::stod(v.value());
        else
            return v.value();
    }

    template <typename T>
    void setValue(const String &key, const T &v)
    {
        checkKey(key);
        if constexpr (std::is_convertible_v<T, String>)
            setValueByKey(key, v);
        else
            setValueByKey(key, std::to_string(v));
    }

private:
    virtual optional<String> getValueByKey(const String &key) = 0;
    virtual void setValueByKey(const String &key, const String &value) = 0;
    void checkKey(const String &key);
};

PRIMITIVES_DB_COMMON_API
void createOrUpdateSchema(LiteDatabase &database, const String &latestSchema, const Strings &diffSqls);

PRIMITIVES_DB_COMMON_API
void createOrUpdateSchema(LiteDatabase &database, const path &latestSchemaFilename, const path &diffSqlsDir, const String &diffSqlsFileMask = FileDatabaseSchemaManager::default_diffs_mask);

PRIMITIVES_DB_COMMON_API
void createOrUpdateSchema(LiteDatabase &database, const String &latestSchema, bool split_schema_for_patches = false);

PRIMITIVES_DB_COMMON_API
void createOrUpdateSchema(LiteDatabase &database, const path &latestSchemaFilename, bool split_schema_for_patches = false);

}
