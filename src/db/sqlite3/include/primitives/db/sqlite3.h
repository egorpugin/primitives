#pragma once

#include <primitives/db.h>

struct sqlite3;

namespace primitives::db::sqlite3
{

struct SqliteDatabase : primitives::db::LiteDatabase
{
    SqliteDatabase(const path &fn);
    SqliteDatabase(SqliteDatabase &&);
    virtual ~SqliteDatabase();

    void execute(const String &q) override;

private:
    ::sqlite3 *db = nullptr;
    static inline const String service_table{ "_svc_kv_settings" };

    void check_error(int r, int ec);

    optional<String> getValueByKey(const String &key) override;
    void setValueByKey(const String &key, const String &value) override;

    void createServiceTable();
    int createServiceTableColumn(const String &col);
};

}
