// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>

namespace primitives::db
{

inline const auto version_column = "version"s;
inline const auto default_schema_filename = "schema.sql"s;

inline const auto default_diffs_spec = "%06d"s;
inline const auto default_diffs_prefix = "patch_"s;
inline const auto default_diffs_mask = default_diffs_prefix + default_diffs_spec + ".sql";

template <typename LiteDatabase>
struct DatabaseSchemaManager
{
    LiteDatabase *database = nullptr;

    virtual ~DatabaseSchemaManager() = default;

    void createOrUpdate() const
    {
        auto v = database->template getValue<int>(version_column);
        if (!v || v == -1)
            create();
        else if (getDiffSqlSize())
            update();
    }
    virtual size_t getDiffSqlSize() const = 0;

private:
    void create() const
    {
        database->execute("BEGIN");
        database->execute(getLatestSchema());
        database->setValue(version_column, getDiffSqlSize());
        database->execute("COMMIT");
    }
    void update() const
    {
        database->execute("BEGIN");
        auto scope = SCOPE_EXIT_NAMED
        {
            database->execute("ROLLBACK");
        };
        auto v = (size_t)database->template getValue<int>(version_column).value();
        auto sz = getDiffSqlSize();
        if (v >= sz)
            return;
        for (auto i = v; i < sz; i++)
            database->execute(getDiffSql(i));
        database->setValue(version_column, getDiffSqlSize());
        database->execute("COMMIT");
        scope.dismiss();
    }

    virtual String getLatestSchema() const = 0;
    virtual String getDiffSql(size_t i) const = 0;
};

template <typename LiteDatabase>
struct FileDatabaseSchemaManager;

template <typename LiteDatabase>
struct StringDatabaseSchemaManager : DatabaseSchemaManager<LiteDatabase>
{
    String latestSchema;
    Strings diffSqls;

    auto write(const path &dir) const
    {
        if (fs::exists(dir))
            remove_all_from_dir(dir);
        else
            fs::create_directories(dir);

        FileDatabaseSchemaManager<LiteDatabase> fm;
        fm.diffSqlsDir = dir;
        fm.latestSchemaFilename = dir / default_schema_filename;
        write_file(fm.latestSchemaFilename, latestSchema);
        int i = 0;
        for (auto &d : diffSqls)
            write_file(fm.getDiffSqlFilename(i++), d);
        return fm;
    }
    size_t getDiffSqlSize() const override
    {
        return diffSqls.size();
    }

private:
    String getLatestSchema() const override
    {
        return latestSchema;
    }
    String getDiffSql(size_t i) const override
    {
        return diffSqls[i];
    }
};

template <typename LiteDatabase>
struct FileDatabaseSchemaManager : DatabaseSchemaManager<LiteDatabase>
{
    path latestSchemaFilename;
    path diffSqlsDir;
    String diffSqlsFileMask = getDefaultDiffsMask();

    auto read(const path &dir) const
    {
        StringDatabaseSchemaManager<LiteDatabase> sm;
        sm.latestSchema = read_file(latestSchemaFilename);
        auto n = getDiffSqlSize();
        for (size_t i = 0; i < n; i++)
            sm.diffSqls.push_back(getDiffSql(i));
        return sm;
    }
    path getDiffSqlFilename(size_t i) const
    {
        const int sz = 100;
        char buf[sz] = { 0 };
        snprintf(buf, sz, diffSqlsFileMask.c_str(), i);
        return diffSqlsDir / buf;
    }
    size_t getDiffSqlSize() const override
    {
        size_t n = 0;
        for (auto &f : fs::directory_iterator(diffSqlsDir))
        {
            if (fs::is_regular_file(f) && f.path().filename().string().find(default_diffs_prefix) == 0)
                n++;
        }
        return n;
    }

    static String getDefaultDiffsMask()
    {
        return default_diffs_mask;
    }

private:
    String getLatestSchema() const override
    {
        return read_file(latestSchemaFilename);
    }
    String getDiffSql(size_t i) const override
    {
        return read_file(getDiffSqlFilename(i));
    }
};

}
