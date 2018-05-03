// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>

namespace primitives::db
{

struct DatabaseSchemaManager
{
    struct LiteDatabase *database = nullptr;

    virtual ~DatabaseSchemaManager() = default;

    void createOrUpdate() const;
    virtual size_t getDiffSqlSize() const = 0;

    static inline const String version_column{ "version" };
    static inline const String default_schema_filename{ "schema.sql" };
    static inline const String default_diffs_spec{ "%06d" };
    static inline const String default_diffs_prefix{ "patch_" };
    static inline const String default_diffs_mask{ default_diffs_prefix + default_diffs_spec + ".sql" };

private:
    void create() const;
    void update() const;

    virtual String getLatestSchema() const = 0;
    virtual String getDiffSql(size_t i) const = 0;
};

struct FileDatabaseSchemaManager;

struct StringDatabaseSchemaManager : DatabaseSchemaManager
{
    String latestSchema;
    Strings diffSqls;

    FileDatabaseSchemaManager write(const path &dir) const;
    size_t getDiffSqlSize() const override;

private:
    String getLatestSchema() const override;
    String getDiffSql(size_t i) const override;
};

struct FileDatabaseSchemaManager : DatabaseSchemaManager
{
    path latestSchemaFilename;
    path diffSqlsDir;
    String diffSqlsFileMask = default_diffs_mask;

    StringDatabaseSchemaManager read(const path &dir) const;
    path getDiffSqlFilename(size_t i) const;
    size_t getDiffSqlSize() const override;

private:
    String getLatestSchema() const override;
    String getDiffSql(size_t i) const override;
};

}
