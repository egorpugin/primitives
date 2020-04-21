#pragma once

#include "templates.h"

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/connection.h>

/*[[nodiscard]]
inline std::unique_ptr<primitives::ExtendedScopeExit> sqlpp11_transaction(::sqlpp::sqlite3::connection &c)
{
    auto ese = std::make_unique<primitives::ExtendedScopeExit>([&c] {c.start_transaction(); });
    ese->on_error = [&c] {c.rollback_transaction(false); };
    ese->on_success = [&c] {c.commit_transaction(); };
    return ese;
}*/

[[nodiscard]]
inline std::unique_ptr<primitives::ExtendedScopeExit> sqlpp11_transaction_manual(::sqlpp::sqlite3::connection &c)
{
    // start IMMEDIATE instead of default DEFERRED?
    // "BEGIN IMMEDIATE"
    auto ese = std::make_unique<primitives::ExtendedScopeExit>([&c] {c.execute("BEGIN"); });
    // sqlite3_exec to prevent throwing from dtor
    ese->on_error = [&c] {sqlite3_exec(c.native_handle(), "ROLLBACK", 0, 0, 0); };
    ese->on_success = [&c] {sqlite3_exec(c.native_handle(), "COMMIT", 0, 0, 0); };
    return ese;
}
