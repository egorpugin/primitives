#pragma once

#include "type_name.h"

#include <boost/pfr.hpp>
#include <primitives/overload.h>
#include <pqxx/pqxx>

#include <format>
#include <tuple>

namespace primitives::postgres {

namespace db {

struct constraint_base {};

struct autoincrement {};
struct primary_key {};
struct unique {};

struct or_ignore {};

struct timestamp {
    struct db_type;
    using value_type = int64_t;

    value_type value;
};

template <typename T, auto... Attributes>
struct type {
    struct db_type;
    using value_type = T;

    value_type value;

    static void for_attributes(auto &&f) {
        (f(Attributes), ...);
    }
    static constexpr bool has_attribute(auto a) {
        return (false || ... || std::is_same_v<decltype(a), decltype(Attributes)>);
    }

    operator auto &&() const {
        return value;
    }
    type &operator=(const T &v) {
        value = v;
        return *this;
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

template <typename Class, typename Type>
constexpr Type get_field_type(Type Class::*);
template <typename Class, typename Type>
constexpr Class get_table_type(Type Class::*);

template <typename Class, typename Type>
auto get_field_name(Type Class::*ptr) {
    Class vv{};
    std::string name;
    boost::pfr::for_each_field(vv, [&]<auto I2>(auto &&field, std::integral_constant<size_t, I2>) {
        if ((void *)&(vv.*ptr) == (void *)&field) {
            name = boost::pfr::get_name<I2, Class>();
        }
    });
    return name;
}
template <typename Class, typename Type>
auto get_table_name(Type Class::*) {
    return type_name<Class>();
}

template <typename Class, typename Type>
consteval auto get_foreign_field_type(Type Class::*f) {
    if constexpr (requires {Type{}.value;}) {
        return decltype(Type{}.value){};
    } else {
        return Type{};
    }
}

struct foreign_key_base : constraint_base {};
template <auto Field, auto... Attributes>
struct foreign_key : foreign_key_base, type<decltype(get_foreign_field_type(Field)), Attributes...> {
    using Table = decltype(get_table_type(Field));
    using type_base_ = type<decltype(get_foreign_field_type(Field)), Attributes...>;
    using table_name = typename Table;
    //using value_type = type_base_::value_type;

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

    operator auto &&() const {
        return type_base_::value;
    }
};

template <auto ... Fields>
struct new_fields {};
template <auto... Fields>
struct new_constraints {};

} // namespace db

struct pgmgr {
    using this_type = pgmgr;

    pqxx::connection c;

    pgmgr(std::string_view conn_string) : c{pqxx::zview{conn_string}} {
    }

    template <typename T>
    void create_table(pqxx::work &tx) {
        // std::print("{}\n", std::source_location::current().function_name());
        std::string query;
        query += std::format("create table if not exists \"{}\"", type_name<T>());
        if (boost::pfr::tuple_size<T>()) {
            query += " (\n";
            boost::pfr::for_each_field(T{}, [&]<auto I>(auto &&field, std::integral_constant<size_t, I>) {
                using type = std::decay_t<decltype(field)>;

                if constexpr (false) {
                } else if constexpr (std::is_base_of_v<db::contraint_unique_base, type>) {
                    query += "  unique (";
                    field.for_fields([&](auto &&f) {
                        T vv{};
                        boost::pfr::for_each_field(vv, [&]<auto I2>(auto &&field, std::integral_constant<size_t, I2>) {
                            if ((void *)&(vv.*f) == (void *)&field) {
                                query += std::format("\"{}\", ", boost::pfr::get_name<I2, T>());
                            }
                        });
                    });
                    query.resize(query.size() - 2);
                    query += ")";
                } else if constexpr (requires { field.value; } || std::is_base_of_v<db::foreign_key_base, type>) {
                    query += std::format("  \"{}\" \"{}\"", boost::pfr::get_name<I, T>(), pg_type<type>());
                    if constexpr (requires { field.for_attributes([]{}); }) {
                        field.for_attributes(overload{[&](db::autoincrement) {
                                                          //query += " autoincrement";
                                                      },
                                                      [&](db::primary_key) {
                                                          query += " primary key";
                                                      },
                                                      [&](db::unique) {
                                                          query += " unique";
                                                      }});
                    }
                } else if constexpr (std::is_base_of_v<db::constraint_base, type>) {
                } else {
                    query += std::format("  \"{}\" \"{}\"", boost::pfr::get_name<I, T>(),
                                         pg_type<type>());
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
                                    std::format("  FOREIGN KEY(\"{}\") REFERENCES \"{}\"(\"{}\")"
                                        , boost::pfr::get_name<I, T>()
                                        , type_name<type::table_name>()
                                        , boost::pfr::get_name<I2, type::table_name>()
                                    );
                            }
                        });
                    });
                    query += ",\n";
                } else if constexpr (std::is_base_of_v<db::constraint_base, type>) {
                    throw std::logic_error{"not implemented"};
                }
            });
            query.resize(query.size() - 2);
            query += "\n)";
        }
        query += ";";
        tx.exec(query);
    }
    template <typename T>
    void create_table() {
        pqxx::work tx{c};
        create_table(tx);
        tx.commit();
    }

    template <auto I>
    struct field_id {};
    template <typename C, typename T>
    static void get_field_id(T C::*byfield, auto &&f) {
        C vv{};
        boost::pfr::for_each_field(vv, [&]<auto I>(auto &&field, std::integral_constant<size_t, I>) {
            if ((void *)&(vv.*byfield) == (void *)&field) {
                f(field_id<I>{});
            }
        });
    }

    template <auto... Fields>
    void update_db(pqxx::work &tx, db::new_constraints<Fields...>) {
        std::string q;
        auto f = [&]<typename C, typename T>(T C::*byfield) {
            q += std::format("alter table \"{}\" ", type_name<C>());
            get_field_id(byfield, [&]<auto I>(field_id<I>) {
                q += std::format("add \"{}\" \"{}\";", boost::pfr::get_name<I, C>(), pg_type<T>());
            });
        };
        (f(Fields), ...);
        tx.exec(q);
    }
    template <auto... Fields>
    void update_db(pqxx::work &tx, db::new_fields<Fields...>) {
        std::string q;
        auto f = [&]<typename C, typename T>(T C::*byfield) {
            q += std::format("alter table \"{}\" ", type_name<C>());
            C vv{};
            boost::pfr::for_each_field(vv, [&]<auto I2>(auto &&field, std::integral_constant<size_t, I2>) {
                if ((void *)&(vv.*byfield) == (void *)&field) {
                    q += std::format("add column \"{}\" \"{}\";", boost::pfr::get_name<I2, C>(), pg_type<T>());
                }
            });
        };
        (f(Fields), ...);
        tx.exec(q);
    }
    template <auto... Fields>
    void update_db(db::new_fields<Fields...> v) {
        pqxx::work tx{c};
        update_db(tx, v);
    }

    auto tx() {
        return pqxx::work{c};
    }

    template <auto Field>
    decltype(db::get_field_type(Field)) select1(pqxx::work &tx) {
        std::string query;
        query += std::format("select \"{}\" from \"{}\"", db::get_field_name(Field), db::get_table_name(Field));
        auto r = tx.exec1(query);
        return r[0].as<decltype(db::get_field_type(Field))>();
    }
    template <auto Field>
    decltype(db::get_field_type(Field)) select1() {
        pqxx::work tx{c};
        return select1<Field>(tx);
    }
    template <auto... Fields, typename... Types>
    void update(pqxx::work &tx, Types &&...values) {
        std::string query;
        [&](auto &&f, auto && ...) {
            query += std::format("update \"{}\" set ", db::get_table_name(f));
        }(Fields...);
        ([&](auto &&f) {
            query += std::format("\"{}\"", db::get_field_name(f));
        }(Fields),...);
        query += " = ";
        // build query
        ([&](auto &&val) {
             query += std::format("{}", val);
        }(values), ...);
        tx.exec(query);
    }
    template <auto... Fields, typename... Types>
    void update(Types &&...values) {
        pqxx::work tx{c};
        update<Fields...>(tx, values...);
    }

    template <typename T>
    static auto row_to_type(const pqxx::row &r) {
        T v;
        boost::pfr::for_each_field(v, [&]<auto I>(auto &&field, std::integral_constant<size_t, I>) {
            using T = std::decay_t<decltype(field)>;
            if constexpr (requires { field.value.value; }) {
                r[I].to(field.value.value);
                throw std::logic_error{"????? fix me"};
            } else if constexpr (requires { field.value; }) {
                r[I].to(field.value);
            } else {
                r[I].to(field);
            }
        });
        return v;
    }

    template <auto Field, auto... Fields, typename ... Types>
    auto insert(pqxx::work &tx, Types &&...values) {
        std::string query;
        query += std::format("insert into \"{}\" (", db::get_table_name(Field));
        query += std::format("\"{}\",", db::get_field_name(Field));
        ([&](auto &&f) {
            query += std::format("\"{}\",", db::get_field_name(f));
        }(Fields),...);
        query.resize(query.size() - 1);
        query += ") values (";
        int id{};
        ([&](auto &&) {
             query += std::format("${},", ++id);
        }(values), ...);
        query.resize(query.size() - 1);
        query += ") returning *";
        auto r = tx.exec_params1(query, values...);
        return row_to_type<decltype(db::get_table_type(Field))>(r);
    }
    template <auto Field, auto... Fields, typename... Types>
    auto insert(Types &&...values) {
        pqxx::work tx{c};
        auto v = insert<Field, Fields...>(tx, values...);
        tx.commit();
        return v;
    }
    template <typename T>
    void insert(T &&value) {
    }
    struct sc_tr {
        this_type *mgr;
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
        pqxx::work tx{c};
        tx.query(query);
        tx.commit();
    }

    template <typename T>
    struct sel {
        this_type *mgr;
        std::string query;
        struct iterator {
            this_type *mgr;
            mutable T t;
            pqxx::work tx;
            pqxx::result r;
            pqxx::result::iterator iter;
            int rc{};

            iterator(auto *mgr, auto &&query) : mgr{mgr}, tx{mgr->c} {
                r = tx.exec(query);
                iter = r.begin();
            }
            auto &operator*() const {
                t = mgr->row_to_type<T>(*iter);
                return t;
            }
            void operator++() {
                ++iter;
            }
            bool operator==(int) const {
                return iter == r.end();
            }
        };
        auto begin() {
            return iterator{mgr, query};
        }
        auto begin() const {
            return iterator{mgr, query};
        }
        auto end() const {
            return 0;
        }
        bool empty() const {
            return begin() == end();
        }
        template <auto byfield>
        auto order_by() {
            query += std::format(" order by \"{}\"", get_field_name(byfield));
            return *this;
        }
        auto order_by(auto byfield) {
            query += std::format(" order by \"{}\"", get_field_name(byfield));
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
        query += std::format("select * from \"{}\"", type_name<T>());
        return sel<T>{this, query};
    }
    template <auto Field, auto... Fields, typename... FieldTypes>
    auto select1(pqxx::work &tx, FieldTypes &&...vals) {
        std::string query;
        query += std::format("select * from \"{}\" where ", db::get_table_name(Field));
        query += std::format("\"{}\" = $1", db::get_field_name(Field));
        /*[&](auto &&ptr, auto &&val) {
            query += std::format("\"{}\" = ?", db::get_field_name(ptr));
        }(Fields);*/
        auto r = tx.exec_params1(query, vals...);
        return row_to_type<decltype(db::get_table_type(Field))>(r);
    }
    template <auto Field, auto... Fields, typename... FieldTypes>
    auto select1(FieldTypes &&...vals) {
        pqxx::work tx{c};
        return select1<Field, Fields...>(tx, vals...);
    }
    template <auto Field, auto... Fields, typename... FieldTypes>
    auto count(pqxx::work &tx, FieldTypes &&...vals) {
        std::string query;
        query += std::format("select count(*) from \"{}\" where ", db::get_table_name(Field));
        query += std::format("\"{}\" = $1", db::get_field_name(Field));
        /*[&](auto &&ptr, auto &&val) {
            query += std::format("\"{}\" = ?", db::get_field_name(ptr));
        }(Fields);*/
        auto r = tx.exec_params1(query, vals...);
        return r[0].as<int>();
    }
    template <auto Field, auto... Fields, typename... FieldTypes>
    auto count(FieldTypes &&...vals) {
        pqxx::work tx{c};
        return count<Field,Fields...>(tx, vals...);
    }

    void create_tables(auto db) {
        struct service_table {
            int schema_version;
        };
        pqxx::work tx{c};
        create_table<service_table>(tx);
        int ver{};
        bool initial{};
        try {
            ver = select1<&service_table::schema_version>(tx);
        } catch (std::exception &) {
            insert<&service_table::schema_version>(tx, ver);
            initial = true;
        }
        boost::pfr::for_each_field(db.tables, [&]<auto I>(auto &&field, std::integral_constant<size_t, I>) {
            create_table<std::decay_t<decltype(field)>>(tx);
        });
        if (initial) {
            ver = boost::pfr::detail::fields_count<decltype(db.updates)>();
            update<&service_table::schema_version>(tx, ver);
            tx.commit();
            return;
        }
        tx.commit();
        boost::pfr::for_each_field(db.updates, [&]<auto I>(auto &&field, std::integral_constant<size_t, I>) {
            if (I + 1 > ver) {
                pqxx::work tx{c};
                update_db(tx, field);
                update<&service_table::schema_version>(tx, ++ver);
                tx.commit();
            }
        });
    }

private:
    template <typename Intype>
    static auto pg_type() {
        constexpr auto get_real_type = [&](){
            if constexpr (requires {typename Intype::db_type;}) {
                return typename Intype::value_type{};
            } else {
                return Intype{};
            }
        };
        constexpr auto has_attr = [&](auto a) {
            if constexpr (requires { typename Intype::db_type; }) {
                if constexpr (requires { Intype::has_attribute(a); }) {
                    return Intype::has_attribute(a);
                } else {
                    return false;
                }
            } else {
                return false;
            }
        };
        using T = decltype(get_real_type());
        constexpr auto seq = has_attr(db::autoincrement{});
        if constexpr (false) {
        } else if constexpr (std::is_same_v<T, int>) {
            if constexpr (seq) {
                return "serial4"sv;
            } else {
                return "int4"sv;
            }
        } else if constexpr (std::is_same_v<T, int64_t>) {
            if constexpr (seq) {
                return "serial8"sv;
            } else {
                return "int8"sv;
            }
        } else if constexpr (std::is_same_v<T, float>) {
            return "float4"sv;
        } else if constexpr (std::is_same_v<T, double>) {
            return "float8"sv;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return "varchar"sv;
        } else if constexpr (std::is_same_v<T, db::timestamp>) {
            return "timestamp"sv;
        } else if constexpr (std::is_same_v<T, bool>) {
            return "bool"sv;
        } else {
            assert(false);
            return ""sv;
        }
    }
};

} // namespace
