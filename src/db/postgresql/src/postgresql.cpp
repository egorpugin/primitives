// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/db/postgresql.h>

#include <pqxx/pqxx>

namespace primitives::db::postgresql
{

PostgresqlDatabase::PostgresqlDatabase(const path &fn)
{
}

PostgresqlDatabase::PostgresqlDatabase(PostgresqlDatabase &&rhs)
{
}

PostgresqlDatabase::~PostgresqlDatabase()
{
}

void PostgresqlDatabase::execute(const String &q)
{
}

void PostgresqlDatabase::check_error(int r, int ec)
{
}

optional<String> PostgresqlDatabase::getValueByKey(const String &key)
{
    return "";
}

void PostgresqlDatabase::setValueByKey(const String &key, const String &value)
{
}

void PostgresqlDatabase::createServiceTable()
{
}

int PostgresqlDatabase::createServiceTableColumn(const String &col)
{
    return 0;
}

}
