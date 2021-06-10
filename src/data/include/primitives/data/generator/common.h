#pragma once

#include <primitives/emitter.h>

namespace primitives::data::generator {

namespace hana = boost::hana;
using namespace hana::literals;

constexpr void generic_print_structure(auto &&structure, auto &&printer) {
    printer[0_c](structure);
    hana::for_each(structure[1_c].fields, [&](auto &&field) {
        printer[1_c](field);
    });
    printer[2_c](structure);
}

constexpr void generic_print_structure2(auto &&structure, auto &&printer) {
    printer[0_c](structure);
    hana::for_each(hana::intersperse(structure[1_c].fields, printer[2_c]),
        hana::overload(
            [&](auto &&field) {
        printer[1_c](field);
    },
            [&](std::decay_t<decltype(printer[2_c])> &&delim) {
        delim();
    }));
    printer[3_c](structure);
}

constexpr void generic_print(auto &&structures, auto &&printer) {
    hana::for_each(structures, [&](auto &&structure) {
        printer[0_c](structure);
        hana::for_each(structure[1_c].fields, [&](auto &&field) {
            printer[1_c](field);
        });
        printer[2_c](structure);
    });
}

constexpr void generic_print2(auto &&structures, auto &&printer) {
    hana::for_each(structures, [&](auto &&structure) {
        printer[0_c](structure);
        hana::for_each(hana::intersperse(structure[1_c].fields, printer[2_c]),
                       hana::overload(
                           [&](auto &&field) {
                               printer[1_c](field);
                           },
                           [&](std::decay_t<decltype(printer[2_c])> &&delim) {
                               delim();
                           }));
        printer[3_c](structure);
    });
}

std::string print_cpp_structures(auto &&structures) {
    primitives::CppEmitter ctx;
    constexpr auto printer = hana::make_tuple(
        [&](auto &&structure) {
            ctx.addLine("struct "s + structure[0_c].name + " {");
            ctx.increaseIndent();
        },
        [&](auto &&field) {
            ctx.addLine();
            if (hana::contains(field, schema::field_properties::optional{})) {
                ctx.addText("std::optional<");
            }
            hana::overload(
                [&ctx](std::string &&) {
                    ctx.addText("std::string");
                },
                [&ctx](int64_t &&) {
                    ctx.addText("std::int64_t");
                })(typename std::decay_t<decltype(field[1_c])>::type_t{});
            if (hana::contains(field, schema::field_properties::optional{})) {
                ctx.addText(">");
            }
            ctx.addText(" ");
            ctx.addText(field[0_c].name + ";"s);
        },
        [&](auto &&structure) {
            ctx.endBlock(true);
        });
    generic_print(structures, printer);
    return ctx.getText();
}

std::string print_protobuf(auto &&structures) {
    primitives::Emitter ctx;
    constexpr auto printer = hana::make_tuple(
        [&](auto &&structure) {
            ctx.addLine("message "s + structure[0_c].name + " {");
            ctx.increaseIndent();
        },
        [&](auto &&field) {
            hana::overload(
                [&ctx](std::string &&) {
                    ctx.addLine("string");
                },
                [&ctx](std::int64_t &&) {
                    ctx.addLine("int64_t");
                })(typename std::decay_t<decltype(field[1_c])>::type_t{});
            ctx.addText(" ");
            ctx.addText(field[0_c].name + ";"s);
        },
        [&](auto &&structure) {
            ctx.decreaseIndent();
            ctx.addLine("}");
        });
    generic_print(structures, printer);
    return ctx.getText();
}

} // namespace primitives::data::generator
