// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/db.h>

#include <primitives/exceptions.h>
#include <primitives/templates.h>

#include <pystring.h>

#include <regex>

#define version_column "version"
#define default_schema_filename "schema.sql"

#define default_diffs_spec "%06d"
#define default_diffs_prefix "patch_"
#define default_diffs_mask default_diffs_prefix default_diffs_spec ".sql"

namespace primitives::db
{

void LiteDatabase::checkKey(const String &key)
{
    static const std::regex r("[_a-zA-Z][_a-zA-Z0-9]*");
    if (!std::regex_match(key, r))
        throw SW_RUNTIME_ERROR("bad key");
}

void DatabaseSchemaManager::createOrUpdate() const
{
    auto v = database->getValue<int>(version_column);
    if (!v || v == -1)
        create();
    else if (getDiffSqlSize())
        update();
}

void DatabaseSchemaManager::create() const
{
    database->execute("BEGIN");
    database->execute(getLatestSchema());
    database->setValue(version_column, getDiffSqlSize());
    database->execute("COMMIT");
}

void DatabaseSchemaManager::update() const
{
    database->execute("BEGIN");
    auto scope = SCOPE_EXIT_NAMED
    {
        database->execute("ROLLBACK");
    };
    auto v = (size_t)database->getValue<int>(version_column).value();
    auto sz = getDiffSqlSize();
    if (v >= sz)
        return;
    for (auto i = v; i < sz; i++)
        database->execute(getDiffSql(i));
    database->setValue(version_column, getDiffSqlSize());
    database->execute("COMMIT");
    scope.dismiss();
}

String StringDatabaseSchemaManager::getLatestSchema() const
{
    return latestSchema;
}

String StringDatabaseSchemaManager::getDiffSql(size_t i) const
{
    return diffSqls[i];
}

size_t StringDatabaseSchemaManager::getDiffSqlSize() const
{
    return diffSqls.size();
}

FileDatabaseSchemaManager StringDatabaseSchemaManager::write(const path &dir) const
{
    if (fs::exists(dir))
        remove_all_from_dir(dir);
    else
        fs::create_directories(dir);

    FileDatabaseSchemaManager fm;
    fm.diffSqlsDir = dir;
    fm.latestSchemaFilename = dir / default_schema_filename;
    write_file(fm.latestSchemaFilename, latestSchema);
    int i = 0;
    for (auto &d : diffSqls)
        write_file(fm.getDiffSqlFilename(i++), d);
    return fm;
}

StringDatabaseSchemaManager FileDatabaseSchemaManager::read(const path &dir) const
{
    StringDatabaseSchemaManager sm;
    sm.latestSchema = read_file(latestSchemaFilename);
    auto n = getDiffSqlSize();
    for (size_t i = 0; i < n; i++)
        sm.diffSqls.push_back(getDiffSql(i));
    return sm;
}

String FileDatabaseSchemaManager::getLatestSchema() const
{
    return read_file(latestSchemaFilename);
}

path FileDatabaseSchemaManager::getDiffSqlFilename(size_t i) const
{
    const int sz = 100;
    char buf[sz] = { 0 };
    snprintf(buf, sz, diffSqlsFileMask.c_str(), i);
    return diffSqlsDir / buf;
}

String FileDatabaseSchemaManager::getDiffSql(size_t i) const
{
    return read_file(getDiffSqlFilename(i));
}

size_t FileDatabaseSchemaManager::getDiffSqlSize() const
{
    size_t n = 0;
    for (auto &f : fs::directory_iterator(diffSqlsDir))
    {
        if (fs::is_regular_file(f) && f.path().filename().string().find(default_diffs_prefix) == 0)
            n++;
    }
    return n;
}

String FileDatabaseSchemaManager::getDefaultDiffsMask()
{
    return default_diffs_mask;
}

void createOrUpdateSchema(LiteDatabase &database, const String &latestSchema, const Strings &diffSqls)
{
    StringDatabaseSchemaManager m;
    m.database = &database;
    m.latestSchema = latestSchema;
    m.diffSqls = diffSqls;
    m.createOrUpdate();
}

void createOrUpdateSchema(LiteDatabase &database, const path &latestSchemaFilename, const path &diffSqlsDir, const String &diffSqlsFileMask)
{
    FileDatabaseSchemaManager m;
    m.database = &database;
    m.latestSchemaFilename = latestSchemaFilename;
    m.diffSqlsDir = diffSqlsDir;
    m.diffSqlsFileMask = diffSqlsFileMask;
    m.createOrUpdate();
}

void createOrUpdateSchema(LiteDatabase &database, const String &latestSchema, bool split_schema_for_patches)
{
    StringDatabaseSchemaManager m;
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

void createOrUpdateSchema(LiteDatabase &database, const path &latestSchemaFilename, bool split_schema_for_patches)
{
    StringDatabaseSchemaManager m;
    m.database = &database;
    m.latestSchema = read_file(latestSchemaFilename);
    if (split_schema_for_patches)
        return createOrUpdateSchema(database, m.latestSchema, split_schema_for_patches);
    m.createOrUpdate();
}

}
