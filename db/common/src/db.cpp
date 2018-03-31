#include <primitives/db.h>

#include <primitives/schema_manager.h>

#include <primitives/templates.h>

#include <regex>

namespace primitives::db
{

void LiteDatabase::checkKey(const String &key)
{
    static const std::regex r("[_a-zA-Z][_a-zA-Z0-9]*");
    if (!std::regex_match(key, r))
        throw std::runtime_error("bad key");
}

void DatabaseSchemaManager::createOrUpdate() const
{
    if (!database->getValue<int>(version_column))
        create();
    else
        update();
}

void DatabaseSchemaManager::create() const
{
    database->execute("BEGIN");
    database->execute(getLatestSchema());
    database->setValue(version_column, getDiffSqlSize());
    database->execute("END");
}

void DatabaseSchemaManager::update() const
{
    if (getDiffSqlSize() == 0)
        return;
    database->execute("BEGIN");
    auto scope = SCOPE_EXIT_NAMED
    {
        database->execute("ROLLBACK");
    };
    auto v = (size_t)database->getValue<int>(version_column).value();
    auto sz = getDiffSqlSize();
    if (v == sz)
        return;
    for (auto i = v; i < sz; i++)
        database->execute(getDiffSql(i));
    database->setValue(version_column, getDiffSqlSize());
    database->execute("END");
    scope.dismiss();
}

String StringDatabaseSchemaManager::getLatestSchema() const
{
    return latestSchema;
}

String StringDatabaseSchemaManager::getDiffSql(int i) const
{
    return diffSqls[i];
}

size_t StringDatabaseSchemaManager::getDiffSqlSize() const
{
    return diffSqls.size();
}

String FileDatabaseSchemaManager::getLatestSchema() const
{
    return read_file(latestSchemaFilename, true);
}

String FileDatabaseSchemaManager::getDiffSql(int i) const
{
    path fn = diffSqlsDir;
    const int sz = 100;
    char buf[sz] = { 0 };
    snprintf(buf, sz, diffSqlsFileMask.c_str(), i);
    return read_file(fn / buf);
}

size_t FileDatabaseSchemaManager::getDiffSqlSize() const
{
    size_t n = 0;
    for (auto &f : boost::make_iterator_range(fs::directory_iterator(diffSqlsDir), {}))
    {
        if (fs::is_regular_file(f))
            n++;
    }
    return n;
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

}
