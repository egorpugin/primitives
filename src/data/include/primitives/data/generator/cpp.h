#pragma once

#include "common.h"

namespace primitives::data::generator::cpp {

void print_cpp_structures(auto &&ctx, auto &&structures) {
    auto printer = hana::make_tuple(
        [&](auto &&structure) {
            ctx.beginBlock("struct "s + structure[0_c].name);
        },
        [&](auto &&field) {
            ctx.addLine();
            if (hana::contains(field.field, schema::field_properties::optional{})) {
                ctx.addText("std::optional<");
            }
            hana::overload(
                [&ctx](std::string &&) {
                    ctx.addText("std::string");
                },
                [&ctx](int64_t &&) {
                    ctx.addText("std::int64_t");
                })(typename std::decay_t<decltype(field.field[1_c])>::type{});
            if (hana::contains(field.field, schema::field_properties::optional{})) {
                ctx.addText(">");
            }
            ctx.addText(" ");
            ctx.addText(field.field[0_c].name + ";"s);
        },
        [&](auto &&structure) {
            ctx.endBlock(true);
            ctx.addLine();
        });
    generic_print(structures, printer);
}

std::string print_cpp(auto &&structures, auto &&ns) {
    primitives::CppEmitter ctx;

    ctx.addLine("// generated file, do not edit");
    ctx.addLine();
    ctx.addLine("#pragma once");
    ctx.addLine();
    ctx.addLine("#include <cstdint>");
    ctx.addLine("#include <string>");
    ctx.addLine();
    ctx.beginNamespace(ns);
    print_cpp_structures(ctx, structures);
    ctx.endNamespace(ns);
    return ctx.getText();
}

} // namespace primitives::data::generator
