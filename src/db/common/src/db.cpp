#include <primitives/db.h>

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
    return read_file(latestSchemaFilename, true);
}

path FileDatabaseSchemaManager::getDiffSqlFilename(int i) const
{
    const int sz = 100;
    char buf[sz] = { 0 };
    snprintf(buf, sz, diffSqlsFileMask.c_str(), i);
    return diffSqlsDir / buf;
}

String FileDatabaseSchemaManager::getDiffSql(int i) const
{
    return read_file(getDiffSqlFilename(i));
}

size_t FileDatabaseSchemaManager::getDiffSqlSize() const
{
    size_t n = 0;
    for (auto &f : boost::make_iterator_range(fs::directory_iterator(diffSqlsDir), {}))
    {
        if (fs::is_regular_file(f) && f.path().filename().string().find(default_diffs_prefix) == 0)
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
