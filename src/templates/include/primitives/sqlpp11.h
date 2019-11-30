#pragma once

#include "templates.h"

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/connection.h>

[[nodiscard]]
primitives::ExtendedScopeExit sqlpp11_transaction(::sqlpp::sqlite3::connection &c)
{
    primitives::ExtendedScopeExit ese([&c] {c.start_transaction(); });
    ese.on_error = [&c] {c.rollback_transaction(false); };
    ese.on_success = [&c] {c.commit_transaction(); };
    return std::move(ese); // move is not needed for cpp17
}

[[nodiscard]]
primitives::ExtendedScopeExit sqlpp11_transaction_manual(::sqlpp::sqlite3::connection &c)
{
    primitives::ExtendedScopeExit ese([&c] {c.execute("BEGIN"); });
    ese.on_error = [&c] {c.execute("ROLLBACK"); };
    ese.on_success = [&c] {c.execute("COMMIT"); };
    return std::move(ese); // move is not needed for cpp17
}
