#pragma once

#include "type_name.h"
#include "overload.h"

#include <boost/pfr.hpp>
#include <sqlite3.h>

#include <tuple>

namespace primitives::sqlite {

namespace db {

struct autoincrement {};
struct primary_key {};
struct unique {};

struct or_ignore {};
struct or_replace {};

template <typename T, auto... Attributes>
struct type {
    T value;

    void for_attributes(auto &&f) const {
        (f(Attributes), ...);
    }
    bool has_attribute(auto a) const {
        return (false || ... || std::is_same_v<decltype(a), decltype(Attributes)>);
    }

    operator auto &&() const {
        return value;
    }
};

struct contraint_unique_base {};
template <auto Field, auto... Fields>
struct contraint_unique : contraint_unique_base {
    void for_fields(auto &&f) {
        f(Field);
        (f(Fields), ...);
    }
};

struct foreign_key_base {};
template <typename Table, auto Field, auto... Attributes>
struct foreign_key : foreign_key_base, type<decltype((std::declval<Table>().*Field).value), Attributes...> {
    using type_base_ = type<decltype((std::declval<Table>().*Field).value), Attributes...>;
    using table_name = Table;

    foreign_key() = default;
    foreign_key(auto &&v) {
        type_base_::value = v;
    }
    foreign_key &operator=(auto &&v) {
        type_base_::value = v;
        return *this;
    }

    void for_fields(auto &&f) {
        f(Field);
    }
};

} // namespace db

struct sqlitemgr {
    sqlite3 *db;

#define sqlite_check2(x, y)                                                                                            \
    if ((x) != y)                                                                                                      \
    throw SW_RUNTIME_ERROR("sqlite error: "s + sqlite3_errmsg(db))
#define sqlite_check(x) sqlite_check2(x, SQLITE_OK)
    sqlitemgr(const path &p) {
        sqlite_check(sqlite3_open((const char *)p.u8string().c_str(), &db));
    }
    ~sqlitemgr() noexcept(false) {
        sqlite_check(sqlite3_close(db));
    }

    void create_tables(auto db) {
        boost::pfr::for_each_field(db.tables, [&]<auto I>(auto &&field, std::integral_constant<size_t, I>) {
            create_table<std::decay_t<decltype(field)>>();
        });
    }
    template <typename TableName, typename T>
    void create_table_with_name() {
        // std::print("{}\n", std::source_location::current().function_name());
        std::string query;
        query += std::format("create table if not exists {}", type_name<TableName>());
        if (boost::pfr::tuple_size<T>()) {
            query += " (\n";
            boost::pfr::for_each_field(T{}, [&]<auto I>(auto &&field, std::integral_constant<size_t, I>) {
                using type = std::decay_t<decltype(field)>;

                if constexpr (std::is_base_of_v<db::contraint_unique_base, type>) {
                    query += "  unique (";
                    field.for_fields([&](auto &&f) {
                        T vv{};
                        boost::pfr::for_each_field(vv, [&]<auto I2>(auto &&field, std::integral_constant<size_t, I2>) {
                            if ((void *)&(vv.*f) == (void *)&field) {
                                query += std::format("{}, ", boost::pfr::get_name<I2, T>());
                            }
                        });
                    });
                    query.resize(query.size() - 2);
                    query += ")";
                } else if constexpr (requires { field.value; } || std::is_base_of_v<db::foreign_key_base, type>) {
                    query += std::format("  {} {}", boost::pfr::get_name<I, T>(),
                                         sqlite_type<std::decay_t<decltype(field.value)>>());
                    field.for_attributes(overload{[&](db::autoincrement) {
                                                      query += " autoincrement";
                                                  },
                                                  [&](db::primary_key) {
                                                      query += " primary key";
                                                  },
                                                  [&](db::unique) {
                                                      query += " unique";
                                                  }});
                } else {
                    query += std::format("  {} {}", boost::pfr::get_name<I, T>(),
                                         sqlite_type<std::decay_t<decltype(field)>>());
                }
                query += ",\n";
            });
            // constraints
            boost::pfr::for_each_field(T{}, [&]<auto I>(auto &&field, std::integral_constant<size_t, I>) {
                using type = std::decay_t<decltype(field)>;

                if constexpr (std::is_base_of_v<db::foreign_key_base, type>) {
                    field.for_fields([&](auto &&f) {
                        typename type::table_name vv{};
                        boost::pfr::for_each_field(vv, [&]<auto I2>(auto &&field, std::integral_constant<size_t, I2>) {
                            if ((void *)&(vv.*f) == (void *)&field) {
                                query +=
                                    std::format("  FOREIGN KEY({}) REFERENCES {}({})", boost::pfr::get_name<I, T>(),
                                                type_name<typename type::table_name>(), boost::pfr::get_name<I2, typename type::table_name>());
                            }
                        });
                    });
                    query += ",\n";
                }
                /*if constexpr (std::is_base_of_v<db::contraint_unique_base, type>) {
                    query += std::format("  UNIQUE (");
                    field.for_fields([&](auto &&f) {
                        T vv{};
                        boost::pfr::for_each_field(vv, [&]<auto I2>(auto &&field, std::integral_constant<size_t, I2>) {
                            if ((void *)&(vv.*f) == (void *)&field) {
                                query += std::format("{}, ", boost::pfr::get_name<I2, T>());
                            }
                        });
                    });
                    query.resize(query.size() - 2);
                    query += std::format(")");
                    query += ",\n";
                }*/
            });
            query.resize(query.size() - 2);
            query += "\n)";
        }
        query += ";";
        this->query(query);
    }
    template <typename T>
    void create_table() {
        create_table_with_name<T,T>();
    }

    template <typename T>
    void insert(T &&value) {
    }
    struct sc_tr {
        sqlitemgr *mgr;
        sc_tr(auto *mgr) : mgr{mgr} {
            mgr->begin();
        }
        ~sc_tr() noexcept(false) {
            if (std::uncaught_exceptions()) {
                mgr->rollback();
            } else {
                mgr->commit();
            }
        }
    };
    auto scoped_transaction() {
        return sc_tr{this};
    }
    void begin() {
        query("begin transaction;");
    }
    void commit() {
        query("commit;");
    }
    void rollback() {
        query("rollback;");
    }
    void query(const std::string &query) {
        // std::print("{}\n\n", query);
        sqlite_check(sqlite3_exec(db, query.c_str(), 0, 0, 0));
    }
    template <typename TableName, typename T, auto... Options>
    struct ps {
        sqlitemgr *mgr;
        T t;
        sqlite3 *db;
        sqlite3_stmt *stmt;

        ps(auto *mgr) : mgr{mgr}, db{mgr->db} {
            std::string query, values;
            query += std::format("insert ");
            (overload{[&](db::or_ignore) {
                 query += std::format("or ignore ");
             },[&](db::or_replace) {
                 query += std::format("or replace ");
             }}(Options),
             ...);
            query += std::format("into {} ", type_name<TableName>());

            query += " (";
            boost::pfr::for_each_field(T{}, [&]<auto I>(auto &&field, std::integral_constant<size_t, I>) {
                using type = std::decay_t<decltype(field)>;

                if constexpr (std::is_base_of_v<db::contraint_unique_base, type>) {
                } else if constexpr (requires { field.value; } || std::is_base_of_v<db::foreign_key_base, type>) {
                    if (!field.has_attribute(db::autoincrement{})) {
                        query += std::format("{}, ", boost::pfr::get_name<I, T>());
                        values += "?, ";
                    }
                } else {
                    query += std::format("{}, ", boost::pfr::get_name<I, T>());
                    values += "?, ";
                }
            });
            query.resize(query.size() - 2);
            values.resize(values.size() - 2);
            query += ") values (" + values + ") returning *;";
            // std::print("{}\n", query);
            sqlite_check(sqlite3_prepare(db, query.c_str(), query.size(), &stmt, 0));
        }
        ~ps() noexcept(false) {
            //sqlite3_step(stmt); // execute if no-op (like delete)? but this is prepared statement
            sqlite_check(sqlite3_finalize(stmt));
        }
        auto insert(const T &value) {
            auto id = 1;
            T ret;
            boost::pfr::for_each_field(value, [&]<auto I>(auto &&field, std::integral_constant<size_t, I>) {
                using type = std::decay_t<decltype(field)>;

                if constexpr (std::is_base_of_v<db::contraint_unique_base, type>) {
                } else if constexpr (requires { field.value; } || std::is_base_of_v<db::foreign_key_base, type>) {
                    if (!field.has_attribute(db::autoincrement{})) {
                        sqlite_bind(db, stmt, id, field.value);
                        ++id;
                    }
                } else {
                    sqlite_bind(db, stmt, id, field);
                    ++id;
                }
            });
            int rc = sqlite3_step(stmt);
            if (rc == SQLITE_ROW) {
                sqlite_result(ret, db, stmt);
            }
            auto lastid = sqlite3_last_insert_rowid(db);
            sqlite_check(sqlite3_reset(stmt));
            return std::tuple{lastid, ret};
        }
    };
    template <typename T, auto... Options>
    auto prepared_insert() {
        return ps<T, T, Options...>{this};
    }
    template <typename TableName, typename T, auto... Options>
    auto prepared_insert_with_name() {
        return ps<TableName, T, Options...>{this};
    }
    template <typename T, auto... Options>
    auto prepared_insert_or_ignore() {
        return ps<T, T, db::or_ignore{}, Options...>{this};
    }
    template <typename T, auto... Options>
    auto prepared_insert_or_replace() {
        return ps<T, T, db::or_replace{}, Options...>{this};
    }

    template <typename T>
    struct sel {
        sqlitemgr *mgr;
        std::string query;
        sqlite3_stmt *stmt{nullptr};
        struct iterator {
            sqlitemgr *mgr;
            T t;
            sqlite3 *db;
            sqlite3_stmt *stmt;
            int rc{};
            bool created{};

            iterator(auto *mgr, auto &&query, sqlite3_stmt *stmt) : mgr{mgr}, db{mgr->db}, stmt{stmt} {
                if (!stmt) {
                    sqlite_check(sqlite3_prepare_v2(db, query.c_str(), query.size(), &this->stmt, 0));
                    created = true;
                }
                if (!rc) {
                    operator++();
                }
            }
            ~iterator() noexcept(false) {
                if (created) {
                    sqlite3_step(stmt); // execute if no-op
                    sqlite_check(sqlite3_finalize(stmt));
                } else {
                    sqlite_check(sqlite3_reset(stmt));
                }
            }
            auto &operator*() const {
                return t;
            }
            void operator++() {
                rc = sqlite3_step(stmt);
                if (rc == SQLITE_ROW) {
                    sqlite_result(t, db, stmt);
                } else if (rc == SQLITE_DONE) {
                } else {
                    sqlite_check(rc);
                }
            }
            bool operator!=(auto) const {
                return rc != SQLITE_DONE;
            }
            bool operator==(int) const {
                return rc == SQLITE_DONE;
            }
        };
        ~sel() noexcept(false) {
            auto db = mgr->db;
            sqlite3_step(stmt); // execute if no-op (like delete)
            sqlite_check(sqlite3_finalize(stmt));
        }
        auto begin() {
            return iterator{mgr, query, stmt};
        }
        auto begin() const {
            return iterator{mgr, query, stmt};
        }
        auto end() const {
            return 0;
        }
        bool empty() const {
            return begin() == end();
        }
        auto order_by(auto byfield) {
            T vv{};
            boost::pfr::for_each_field(vv, [&]<auto I2>(auto &&field, std::integral_constant<size_t, I2>) {
                if ((void *)&(vv.*byfield) == (void *)&field) {
                    query += std::format(" order by {}", boost::pfr::get_name<I2, T>());
                }
            });
            return *this;
        }
        auto where(const std::string &text) {
            query += " where " + text;
            return *this;
        }
    };
    template <typename T>
    auto select() {
        std::string query;
        query += std::format("select * from {}", type_name<T>());
        return sel<T>{this, query};
    }
    template <typename TableName, typename T>
    auto select_with_name() {
        std::string query;
        query += std::format("select * from {}", type_name<TableName>());
        return sel<T>{this, query};
    }
    template <typename TableName, typename T, auto... Fields, typename... FieldTypes>
    auto query_with_name(std::string query, FieldTypes &&...vals) {
        query += std::format(" from {} where ", type_name<TableName>());
        // build query
        auto and_ = "and "sv;
        bool added{};
        (overload{[&](auto &&ptr, auto &&val) {
             T vv{};
             boost::pfr::for_each_field(vv, [&]<auto I2>(auto &&field, std::integral_constant<size_t, I2>) {
                 if ((void *)&(vv.*ptr) == (void *)&field) {
                     query += std::format("{} = ?", boost::pfr::get_name<I2, T>());
                     query += and_;
                     added = true;
                 }
             });
         }}(Fields, vals),
         ...);
        if (added) {
            query.resize(query.size() - and_.size());
        }
        // bind
        sqlite3_stmt *stmt;
        sqlite_check(sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, 0));
        auto id = 1;
        (overload{[&](auto &&ptr, auto &&val) {
             T vv{};
             boost::pfr::for_each_field(vv, [&]<auto I2>(auto &&field, std::integral_constant<size_t, I2>) {
                 if ((void *)&(vv.*ptr) == (void *)&field) {
                     if constexpr (requires { val.value; }) {
                         sqlite_bind(db, stmt, id, val.value);
                     } else {
                         sqlite_bind(db, stmt, id, val);
                     }
                 }
             });
             ++id;
         }}(Fields, vals),
         ...);
        return sel<T>{this, query, stmt};
    }
    template <typename TableName, typename T, auto... Fields, typename... FieldTypes>
    auto select_with_name(FieldTypes &&...vals) {
        return query_with_name<TableName, T, Fields...>("select *", FWD(vals)...);
    }
    template <typename T, auto... Fields, typename... FieldTypes>
    auto select(FieldTypes &&...vals) {
        return select_with_name<T,T,Fields...>(FWD(vals)...);
    }
    template <typename TableName, typename T, auto... Fields, typename... FieldTypes>
    auto delete_with_name(FieldTypes &&...vals) {
        return query_with_name<TableName, T, Fields...>("delete", FWD(vals)...);
    }

private:
    template <typename T>
    static auto sqlite_bind(auto *db, auto *stmt, int paramid, const T &v) {
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
            sqlite_check(sqlite3_bind_int64(stmt, paramid, v));
        } else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
            assert(false);
        } else if constexpr (std::is_same_v<std::decay_t<T>, const char *>) {
            std::string_view sv{v};
            sqlite_check(sqlite3_bind_text(stmt, paramid, sv.data(), sv.size(), 0));
        } else if constexpr (std::is_same_v<T, std::string>) {
            sqlite_check(sqlite3_bind_text(stmt, paramid, v.c_str(), v.size(), 0));
        } else if constexpr (std::is_same_v<T, bool>) {
            sqlite_check(sqlite3_bind_int64(stmt, paramid, !!v));
        } else {
            assert(false);
        }
    }
    template <typename T>
    static void sqlite_result(T &v, auto *db, auto *stmt) {
        boost::pfr::for_each_field(v, [&]<auto I>(auto &&field, std::integral_constant<size_t, I>) {
            using type = std::decay_t<decltype(field)>;

            if constexpr (std::is_base_of_v<db::contraint_unique_base, type>) {
            } else if constexpr (requires { field.value; } || std::is_base_of_v<db::foreign_key_base, type>) {
                field.value = sqlite_result<decltype(field.value)>(db, stmt, I);
            } else {
                field = sqlite_result<std::decay_t<decltype(field)>>(db, stmt, I);
            }
        });
    }
    template <typename T>
    static T sqlite_result(auto *db, auto *stmt, int paramid) {
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
            return sqlite3_column_int64(stmt, paramid);
        } else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
            assert(false);
            return {};
        } else if constexpr (std::is_same_v<T, std::string>) {
            auto s = sqlite3_column_text(stmt, paramid);
            auto sz = sqlite3_column_bytes(stmt, paramid);
            return std::string(s, s + sz);
        } else if constexpr (std::is_same_v<T, bool>) {
            return sqlite3_column_int(stmt, paramid);
        } else {
            assert(false);
            return {};
        }
    }
    template <typename T>
    static auto sqlite_type() {
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
            return "integer"sv;
        } else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
            return "real"sv;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return "text"sv;
        } else if constexpr (std::is_same_v<T, bool>) {
            return "boolean"sv;
        } else {
            assert(false);
            return ""sv;
        }
    }
#undef sqlite_check
#undef sqlite_check2
};

// detail ns?
template <typename K, typename V>
struct kv {
    using table_type = kv;
    using key_type = K;
    using value_type = V;

    K key;
    V value;

    db::contraint_unique<&kv::key> u;
};

template <typename Table, typename ... Tables>
struct cache {
    sqlitemgr mgr;

    cache(const path &fn) : mgr{fn} {
        register_table<Table>();
        (register_table<Tables>(),...);
    }
    template <typename T>
    std::optional<typename T::value_type> find(const typename T::key_type &k) {
        static_assert(has_table<T>());

        auto q = mgr.select_with_name<T, typename T::table_type, &T::key>(k);
        if (q.empty()) {
            return {};
        }
        return (*q.begin()).value;
    }
    template <typename T>
    typename T::value_type find(const typename T::key_type &k, auto &&create_value) {
        static_assert(has_table<T>());

        auto v = find<T>(k);
        if (!v) {
            create_value([&](auto &&v) {
                set<T>(k, v);
            });
            while (!(v = find<T>(k))) {
                std::this_thread::sleep_for(100ms);
            }
        }
        return *v;
    }
    template <typename T>
    void set(const typename T::key_type &k, const typename T::value_type &v) {
        static_assert(has_table<T>());

        auto tr = mgr.scoped_transaction();
        auto ins = mgr.prepared_insert_with_name<T, typename T::table_type, db::or_replace{}>();
        typename T::table_type tt{k,v};
        ins.insert(tt);
    }
    template <typename T>
    void change(const typename T::key_type &k, const typename T::value_type &c) {
        auto v = find<T>(k);
        set<T>(k, !v ? (typename T::value_type{} + c) : (*v + c));
    }
    template <typename T>
    void inc(const typename T::key_type &k) {
        change<T>(k, 1);
    }
    template <typename T>
    void dec(const typename T::key_type &k) {
        change<T>(k, -1);
    }
    template <typename T>
    auto get_all() {
        static_assert(has_table<T>());

        std::map<typename T::key_type, typename T::value_type> m;
        for (auto &&[k,v,_] : mgr.select_with_name<T, typename T::table_type>()) {
            m.emplace(k,v);
        }
        return m;
    }
    template <typename T>
    void erase(const typename T::key_type &k) {
        static_assert(has_table<T>());

        mgr.delete_with_name<T, typename T::table_type, &T::key>(k);
    }

private:
    template <typename T> void register_table() {
        mgr.create_table_with_name<T, typename T::table_type>();
    }
    template <typename T>
    static consteval bool has_table() {
        return (std::same_as<Table, T> || ... || std::same_as<Tables, T>);
    }
};

} // namespace
