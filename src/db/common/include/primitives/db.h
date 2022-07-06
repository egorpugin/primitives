// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/schema_manager.h>
#include <primitives/exceptions.h>
#include <primitives/templates.h>

#include <pystring.h>

#include <optional>

namespace primitives::db
{

/// lite db: has execute() call
/// kv db accessor in column manner: has getValue()/setValue() calls
struct LiteDatabase
{
    virtual ~LiteDatabase() = default;

    virtual void execute(const String &) = 0;

    template <typename T>
    std::optional<T> getValue(const String &key)
    {
        checkKey(key);
        auto v = getValueByKey(key);
        if (!v)
            return {};
        if constexpr (std::is_same_v<T, int>)
            return std::stoi(v.value());
        else if constexpr (std::is_same_v<T, long long int>)
            return std::stoll(v.value());
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
        else if constexpr (std::is_same_v<T, long long int>)
            return std::stoll(v.value());
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
    virtual std::optional<String> getValueByKey(const String &key) = 0;
    virtual void setValueByKey(const String &key, const String &value) = 0;
    void checkKey(const String &key)
    {
        static const std::regex r("[_a-zA-Z][_a-zA-Z0-9]*");
        if (!std::regex_match(key, r))
            throw SW_RUNTIME_ERROR("bad key");
    }
};

inline void createOrUpdateSchema(LiteDatabase &database, const String &latestSchema, const Strings &diffSqls)
{
    StringDatabaseSchemaManager<LiteDatabase> m;
    m.database = &database;
    m.latestSchema = latestSchema;
    m.diffSqls = diffSqls;
    m.createOrUpdate();
}

inline void createOrUpdateSchema(LiteDatabase &database, const path &latestSchemaFilename, const path &diffSqlsDir,
                          const String &diffSqlsFileMask = FileDatabaseSchemaManager<LiteDatabase>::getDefaultDiffsMask())
{
    FileDatabaseSchemaManager<LiteDatabase> m;
    m.database = &database;
    m.latestSchemaFilename = latestSchemaFilename;
    m.diffSqlsDir = diffSqlsDir;
    m.diffSqlsFileMask = diffSqlsFileMask;
    m.createOrUpdate();
}

inline void createOrUpdateSchema(LiteDatabase &database, const String &latestSchema, bool split_schema_for_patches = false)
{
    StringDatabaseSchemaManager<LiteDatabase> m;
    m.database = &database;
    if (split_schema_for_patches)
    {
        Strings v;
        pystring::split(latestSchema, v, "%split");
        m.latestSchema = v[0];
        m.diffSqls.assign(v.begin() + 1, v.end());
    }
    else
        m.latestSchema = latestSchema;
    m.createOrUpdate();
}

inline void createOrUpdateSchema(LiteDatabase &database, const path &latestSchemaFilename,
                          bool split_schema_for_patches = false)
{
    StringDatabaseSchemaManager<LiteDatabase> m;
    m.database = &database;
    m.latestSchema = read_file(latestSchemaFilename);
    if (split_schema_for_patches)
        return createOrUpdateSchema(database, m.latestSchema, split_schema_for_patches);
    m.createOrUpdate();
}

}
