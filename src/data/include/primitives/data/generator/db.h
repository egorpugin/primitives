#pragma once

#include "common.h"

#include <regex>

namespace primitives::data::generator::db {

struct sqlite3_tag {};
struct postgresql_tag {};

std::string print_sql(auto &&structure, auto &&db_type) {
    constexpr auto is_sqlite = std::is_same_v<std::decay_t<decltype(db_type)>, sqlite3_tag>;
    constexpr auto is_pg = std::is_same_v<std::decay_t<decltype(db_type)>, postgresql_tag>;

    primitives::Emitter ctx;
    auto pk = hana::find_if(structure, schema::is_primary_key);

    auto printer = hana::make_tuple(
        [&](auto &&structure) {
            ctx.addLine("CREATE TABLE "s + structure[0_c].name + " (");
            ctx.increaseIndent();
        },
        [&](auto &&field) {
            ctx.addLine(field.field[0_c].name + " "s);
            auto type = hana::overload(
                [](std::string &&) {
                    if constexpr (is_sqlite)
                        return "TEXT";
                    else if constexpr (is_pg)
                        return "text";
                    //else
                        //static_assert(false, "not implemented");
                },
                [](std::int64_t &&) {
                    if constexpr (is_sqlite)
                        return "INTEGER";
                    else if constexpr (is_pg)
                        return "bigint";
                    //else
                        //static_assert(false, "not implemented");
                },
                [&field](auto &&) {
                    /*if (hana::size(field.field)) {
                        return hana::overload([](schema::sqlite3_column_type t) {
                            switch (t) {
                            case schema::sqlite3_column_type::blob: return "BLOB";
                            case schema::sqlite3_column_type::integer: return "INTEGER";
                            case schema::sqlite3_column_type::text: return "TEXT";
                            case schema::sqlite3_column_type::real: return "REAL";
                            }
                        }, [](auto &&) { return "UNIMPLEMENTED"; })(field.field[3_c]);
                    } else {*/
                        return "UNIMPLEMENTED";
                    //}
                })(typename std::decay_t<decltype(field.field[1_c])>::type{});
            ctx.addText(type);
            if (!hana::contains(field.field, schema::field_properties::optional)) {
                ctx.addText(" NOT NULL");
            }
        },
        [&]() {
            ctx.addText(","s);
        },
        [&](auto &&structure) {
        });
    generic_print_structure2(structure, printer);

    auto delim = [&ctx] {
        ctx.addText(", "s);
    };
    if constexpr (pk != hana::nothing) {
        ctx.addText(","s);
        ctx.addLine("PRIMARY KEY (");
        hana::for_each(hana::intersperse(pk->primary_key, delim), hana::overload(
                                                                        [&](auto &&field) {
                                                                            ctx.addText(field.field[0_c].name);
                                                                        },
                                                                        [&](std::decay_t<decltype(delim)> &&f) {
                                                                            f();
                                                                        }));
        ctx.addText(")");
    }
    auto fk = hana::find_if(structure, schema::is_foreign_key);
    if constexpr (fk != hana::nothing) {
        hana::for_each(hana::filter(structure, schema::is_foreign_key), [&ctx](auto &&fk) {
            ctx.addText(","s);
            ctx.addLine("FOREIGN KEY (");
            ctx.addText(fk.foreign_key[0_c].field[0_c].name);
            ctx.addText(") REFERENCES ");
            ctx.addText(fk.foreign_key[1_c][0_c].name);
            ctx.addText("(");
            ctx.addText(fk.foreign_key[0_c].field[0_c].name);
            ctx.addText(")");
        });
    }
    auto unique = hana::find_if(structure, schema::is_unique);
    if constexpr (unique != hana::nothing) {
        hana::for_each(hana::filter(structure, schema::is_unique), [&ctx, &delim](auto &&unique) {
            ctx.addText(","s);
            ctx.addLine("UNIQUE (");
            hana::for_each(hana::intersperse(unique.unique, delim), hana::overload(
                                                                        [&](auto &&field) {
                                                                            ctx.addText(field.field[0_c].name);
                                                                        },
                                                                        [&](std::decay_t<decltype(delim)> &&f) {
                                                                            f();
                                                                        }));
            ctx.addText(")");
        });
    }

    ctx.decreaseIndent();
    ctx.addLine(");");
    return ctx.getText();
}

std::string print_sqls(auto &&structures, auto &&db_type) {
    std::string s;
    hana::for_each(structures, [&](auto &&structure) {
        s += print_sql(structure, db_type) + "\n";
    });
    return s;
}

static const auto NAMESPACE = "sqlpp"s;

void print_sqlpp11(primitives::CppEmitter &ctx, auto &&table) {
    auto repl_camel_case_func = [](const std::smatch &m, String &o) {
        auto s = m[2].str();
        s[0] = toupper(s[0]);
        if (m[1] == "_")
            return s;
        else
            return m[1].str() + s;
    };
    auto toName = [&repl_camel_case_func](String s, const String &rgx) {
        String o;
        std::regex r(rgx);
        std::smatch m;
        while (std::regex_search(s, m, r))
        {
            o += m.prefix().str();
            o += repl_camel_case_func(m, o);
            s = m.suffix().str();
        }
        o += s;
        return o;
    };
    auto toClassName = [&toName](String s) {
        s[0] = toupper(s[0]);
        return toName(s, "(\\s|[_0-9])(\\S)");
    };
    auto toMemberName = [&toName](String s) {
        return toName(s, "(\\s|_|[0-9])(\\S)");
    };

    auto pk = hana::find_if(table, schema::is_primary_key);
    auto dv = hana::find_if(table, schema::is_default_value);

    String sqlTableName = table[0_c].name;
    auto tableClass = toClassName(sqlTableName);
    auto tableMember = toMemberName(sqlTableName);
    auto tableNamespace = tableClass + "_";
    auto tableTemplateParameters = tableClass;
    ctx.beginNamespace(tableNamespace);

    hana::for_each(table[1_c].fields, [&](auto &&field) {
        String sqlColumnName = field.field[0_c].name;
        auto columnClass = toClassName(sqlColumnName);
        tableTemplateParameters += ",\n               " + tableNamespace + "::" + columnClass;
        auto columnMember = toMemberName(sqlColumnName);
        ctx.beginBlock("struct " + columnClass);
        ctx.beginBlock("struct _alias_t");
        ctx.addLine("static constexpr const char _literal[] = \"" + sqlColumnName + "\";");
        ctx.emptyLines();
        ctx.addLine("using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;");
        ctx.emptyLines();
        ctx.addLine("template<typename T>");
        ctx.beginBlock("struct _member_t");
        ctx.addLine("T " + columnMember + ";");
        ctx.emptyLines();
        ctx.addLine("T& operator()() { return " + columnMember + "; }");
        ctx.addLine("const T& operator()() const { return " + columnMember + "; }");
        ctx.endBlock(true);
        ctx.endBlock(true);
        ctx.emptyLines();

        String sqlColumnType = hana::overload(
            [](std::string &&) {
                return "text";
            },
            [](std::int64_t &&) {
                return "integer";
            },
            [&field]<typename T>(T &&x) {
                if (1 || hana::contains(field.field, schema::sqlite3_column_type{})) {
                    //throw SW_RUNTIME_ERROR("missing sqlite3_column_type111111");
                    return "integeraaaaaa";
                }
                else
                    //static_assert(false, "missing sqlite3_column_type");
                    throw SW_RUNTIME_ERROR("missing sqlite3_column_type for field "s + field.field[0_c].name);
                //return type_to_sqlite3_column(std::forward<T>(x));
            })(typename std::decay_t<decltype(field.field[1_c])>::type{});

        Strings traitslist;
        traitslist.push_back(NAMESPACE + "::" + sqlColumnType);

        auto requireInsert = true;
        auto hasAutoValue = sqlColumnName == "id";
        if (hasAutoValue) {
            traitslist.push_back(NAMESPACE + "::tag::must_not_insert");
            traitslist.push_back(NAMESPACE + "::tag::must_not_update");
            requireInsert = false;
        }
        if (hana::contains(field.field, schema::field_properties::optional)) {
            traitslist.push_back(NAMESPACE + "::tag::can_be_null");
            requireInsert = false;
        }
        if (dv != hana::nothing) {
            requireInsert = false;
        }
        if (pk != hana::nothing) {
            if (hana::contains(pk->primary_key, field)) {
                requireInsert = false;
            }
        }
        if (requireInsert) {
            traitslist.push_back(NAMESPACE + "::tag::require_insert");
        }
        String l;
        for (auto &li : traitslist)
            l += li + ", ";
        if (!l.empty())
            l.resize(l.size() - 2);
        ctx.addLine("using _traits = " + NAMESPACE + "::make_traits<" + l + ">;");
        ctx.endBlock(true);
        ctx.emptyLines();
    });

    ctx.endNamespace(tableNamespace);
    ctx.emptyLines();

    ctx.beginBlock("struct " + tableClass + " : " + NAMESPACE + "::table_t<" + tableTemplateParameters + ">");
    ctx.beginBlock("struct _alias_t");
    ctx.addLine("static constexpr const char _literal[] = \"" + sqlTableName + "\";");
    ctx.emptyLines();
    ctx.addLine("using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;");
    ctx.emptyLines();
    ctx.addLine("template<typename T>");
    ctx.beginBlock("struct _member_t");
    ctx.addLine("T " + tableMember + ";");
    ctx.emptyLines();
    ctx.addLine("T& operator()() { return " + tableMember + "; }");
    ctx.addLine("const T& operator()() const { return " + tableMember + "; }");
    ctx.endBlock(true);
    ctx.endBlock(true);
    ctx.endBlock(true);
    ctx.emptyLines();
}

std::string print_sqlpp11(auto &&structures, const std::string &ns) {
    auto INCLUDE = "sqlpp11";

    primitives::CppEmitter ctx;

    ctx.addLine("// generated file, do not edit");
    ctx.addLine();
    ctx.addLine("#pragma once");
    ctx.addLine();
    ctx.addLine("#include <"s + INCLUDE + "/table.h>");
    ctx.addLine("#include <"s + INCLUDE + "/data_types.h>");
    ctx.addLine("#include <"s + INCLUDE + "/char_sequence.h>");
    ctx.addLine();
    ctx.beginNamespace(ns);
    boost::hana::for_each(structures, [&ctx](auto &&structure) {
        print_sqlpp11(ctx, structure);
    });
    ctx.endNamespace(ns);
    return ctx.getText();
}

} // namespace primitives::data::generator
