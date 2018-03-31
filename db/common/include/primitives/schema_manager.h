#pragma once

#include <primitives/string.h>

namespace primitives::db
{

struct DatabaseSchemaManager
{
    struct LiteDatabase *database = nullptr;

    virtual ~DatabaseSchemaManager() = default;

    void createOrUpdate() const;

    static inline const String version_column{ "version" };

private:
    void create() const;
    void update() const;

    virtual String getLatestSchema() const = 0;
    virtual String getDiffSql(int i) const = 0;
    virtual size_t getDiffSqlSize() const = 0;
};

struct StringDatabaseSchemaManager : DatabaseSchemaManager
{
    String latestSchema;
    Strings diffSqls;

private:
    String getLatestSchema() const override;
    String getDiffSql(int i) const override;
    size_t getDiffSqlSize() const override;
};

struct FileDatabaseSchemaManager : DatabaseSchemaManager
{
    path latestSchemaFilename;
    path diffSqlsDir;
    String diffSqlsFileMask;

private:
    String getLatestSchema() const override;
    String getDiffSql(int i) const override;
    size_t getDiffSqlSize() const override;
};

}
