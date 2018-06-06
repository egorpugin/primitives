// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/db/postgresql.h>

#include <pqxx/pqxx>

namespace primitives::db::postgresql
{

PostgresqlDatabase::PostgresqlDatabase(const String &cs)
    : c(std::make_unique<pqxx::connection>(cs))
{
}

PostgresqlDatabase::PostgresqlDatabase(std::unique_ptr<pqxx::connection> conn)
    : c(std::move(conn))
{
}

PostgresqlDatabase::PostgresqlDatabase(PostgresqlDatabase &&rhs)
{
    c = std::move(rhs.c);
}

PostgresqlDatabase::~PostgresqlDatabase()
{
}

void PostgresqlDatabase::execute(const String &q)
{
    pqxx::nontransaction txn(*c);
    txn.exec(q);
}

optional<String> PostgresqlDatabase::getValueByKey(const String &key)
{
    createServiceTable();
    if (createServiceTableColumn(key))
        return {};

    pqxx::nontransaction txn(*c);
    auto r = txn.exec("SELECT " + key + " FROM " + service_table);
    return r[0][0].c_str();
}

void PostgresqlDatabase::setValueByKey(const String &key, const String &value)
{
    createServiceTable();
    createServiceTableColumn(key);

    pqxx::nontransaction txn(*c);
    txn.exec_params("UPDATE " + service_table + " SET " + key + " = $1", value);
}

void PostgresqlDatabase::createServiceTable()
{
    execute_and_check("CREATE SCHEMA " + service_schema_name, "42P06");
    execute_and_check("CREATE TABLE " + service_table + " (version INT NOT NULL DEFAULT -1)", "42P07");

    pqxx::nontransaction txn(*c);
    auto r = txn.exec("SELECT * FROM " + service_table);
    if (r.empty())
        txn.exec("INSERT INTO " + service_table + " (version) values (-1)");
    else if (r.size() > 1)
        throw std::runtime_error("db error: more than one row is present"s);
}

bool PostgresqlDatabase::createServiceTableColumn(const String &col)
{
    return execute_and_check("ALTER TABLE " + service_table + " ADD COLUMN " + col + " VARCHAR NOT NULL DEFAULT ''", "42701");
}

bool PostgresqlDatabase::execute_and_check(const String &q, const String &allowed_error_code)
{
    auto exec_and_check = [this](const String &q, const String &allowed_error_code)
    {
        try
        {
            execute(q);
            return true;
        }
        catch (pqxx::sql_error &e)
        {
            if (e.sqlstate() != allowed_error_code)
                throw;
        }
        return false;
    };

    auto exec_and_check_with_rollback = [&](const String &q, const String &allowed_error_code)
    {
        try
        {
            execute(q);
            return true;
        }
        catch (pqxx::sql_error &e)
        {
            if (e.sqlstate() != allowed_error_code)
                throw;
            exec_and_check("ROLLBACK TO sp", "25P01");
        }
        return false;
    };

    exec_and_check_with_rollback("SAVEPOINT sp", "25P01");
    return exec_and_check_with_rollback(q, allowed_error_code);
}

void createDb(const String &root_cs, const String &dbname, const String &owner)
{
    PostgresqlDatabase db(root_cs);
    db.execute("CREATE DATABASE " + dbname + " OWNER " + owner);
}

void dropDb(const String &root_cs, const String &dbname)
{
    PostgresqlDatabase db(root_cs);
    db.execute("DROP DATABASE " + dbname);
}

}
