#pragma once

#include <primitives/db.h>

namespace primitives::db::postgresql
{

struct PostgresqlDatabase : primitives::db::LiteDatabase
{
    PostgresqlDatabase(const path &fn);
    PostgresqlDatabase(PostgresqlDatabase &&);
    virtual ~PostgresqlDatabase();

    void execute(const String &q) override;

private:
    static inline const String service_table{ "service.kv_settings" };

    void check_error(int r, int ec);

    optional<String> getValueByKey(const String &key) override;
    void setValueByKey(const String &key, const String &value) override;

    void createServiceTable();
    int createServiceTableColumn(const String &col);
};

}
