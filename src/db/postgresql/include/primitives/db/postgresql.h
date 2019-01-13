// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/db.h>

#include <pqxx/connection.hxx>

namespace primitives::db::postgresql
{

struct PRIMITIVES_DB_POSTGRESQL_API PostgresqlDatabase : primitives::db::LiteDatabase
{
    PostgresqlDatabase(const String &cs);
    PostgresqlDatabase(std::unique_ptr<pqxx::connection>);
    PostgresqlDatabase(PostgresqlDatabase &&);
    virtual ~PostgresqlDatabase();

    void execute(const String &q) override;

private:
    std::unique_ptr<pqxx::connection> c;

    static inline const String service_schema_name{ "service" };
    static inline const String service_table_name{ "kv_settings" };
    static inline const String service_table{ service_schema_name + "." + service_table_name };

    bool execute_and_check(const String &q, const String &allowed_error_code);

    std::optional<String> getValueByKey(const String &key) override;
    void setValueByKey(const String &key, const String &value) override;

    void createServiceTable();
    bool createServiceTableColumn(const String &col);
};

PRIMITIVES_DB_POSTGRESQL_API
void createDb(const String &root_cs, const String &dbname, const String &owner);

PRIMITIVES_DB_POSTGRESQL_API
void dropDb(const String &root_cs, const String &dbname);

}
