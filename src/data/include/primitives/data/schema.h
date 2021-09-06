#pragma once

#include <boost/hana.hpp>

//#include <optional>
//#include <string>

namespace primitives::data::schema {

inline namespace common_properties {

#define TEXT_FIELD(x)                                                                                                  \
    struct x##_ {                                                                                                      \
        char x[64];                                                                                                    \
        constexpr x##_(const char *n) : x() {                                                                          \
            for (int i = 0; *n; i++)                                                                                   \
                x[i] = *n++;                                                                                           \
        }                                                                                                              \
        constexpr auto operator<=>(const x##_ &) const = default;                                                      \
    };                                                                                                                 \
    template <x##_ v>                                                                                                  \
    constexpr x##_ operator""_##x() {                                                                                  \
        return v;                                                                                                      \
    }

TEXT_FIELD(name)
TEXT_FIELD(type)

} // namespace common_properties

inline namespace table_properties {

#define TYPED_STORAGE(x)                                                                                               \
    template <typename T>                                                                                              \
    struct x##_ {                                                                                                      \
        T x;                                                                                                           \
        constexpr x##_(T &&v) : x{v} {}                                                                                \
    };                                                                                                                 \
    template <typename T>                                                                                              \
    x##_(T &&v)->x##_<T>;                                                                                              \
                                                                                                                       \
    template <typename T>                                                                                              \
    struct is_##x##_ : std::false_type {};                                                                             \
    template <typename T>                                                                                              \
    struct is_##x##_<x##_<T>> : std::true_type {};                                                                     \
    constexpr auto is_##x = boost::hana::compose(boost::hana::trait<is_##x##_>, boost::hana::typeid_)

TYPED_STORAGE(field);
template <typename T>
auto operator==(const field_<T> &f1, const field_<T> &f2) {
    return f1.field == f2.field;
}
template <typename T>
auto operator!=(const field_<T> &f1, const field_<T> &f2) {
    return f1.field != f2.field;
}
TYPED_STORAGE(fields);
TYPED_STORAGE(primary_key);
TYPED_STORAGE(foreign_key);
TYPED_STORAGE(unique);
TYPED_STORAGE(on_conflict);
TYPED_STORAGE(on_delete);
TYPED_STORAGE(on_update);

#undef TYPED_STORAGE

constexpr struct {} replace;
constexpr struct {} cascade;

} // inline namespace table_properties

inline namespace field_properties {

#define FIELD_CHECK(x)                                                                                                 \
    template <typename T>                                                                                              \
    struct is_##x##_ : std::false_type {};                                                                             \
    template <>                                                                                                        \
    struct is_##x##_<x> : std::true_type {};                                                                           \
    constexpr auto is_##x = boost::hana::compose(boost::hana::trait<is_##x##_>, boost::hana::typeid_)

/*template <typename T>
struct type {
    using type_t = T;

    auto operator<=>(const type &) const = default;
};*/

template <typename T>
constexpr boost::hana::type<T> type{};

#define BOOL_FIELD(x)                                                                                                  \
    struct x##_ {                                                                                                      \
        auto operator<=>(const x##_ &) const = default;                                                                \
    };                                                                                                                 \
    constexpr x##_ x{};                                                                                                \
    FIELD_CHECK(x##_)

BOOL_FIELD(optional);
BOOL_FIELD(unique_field);

/*struct skip_required_field_for_sqlpp {
    auto operator<=>(const skip_required_field_for_sqlpp &) const = default;
};
FIELD_CHECK(skip_required_field_for_sqlpp);*/

//template <typename T>
//struct default_value { T v; };
//or
template <auto v>
struct default_value {};

template <typename T>
struct is_default_value_ : std::false_type {};
template <auto v>
struct is_default_value_<default_value<v>> : std::true_type {};
constexpr auto is_default_value = boost::hana::compose(boost::hana::trait<is_default_value_>, boost::hana::typeid_);

//struct default_value_expression { std::string expr; };
//or
template <const char *expr>
struct default_value_expression {};

#undef FIELD_CHECK
#undef BOOL_FIELD

} // inline namespace field_properties

// low level
constexpr auto make_field(auto && ... args) {
    return boost::hana::make_tuple(std::forward<decltype(args)>(args)...);
}
constexpr auto make_fields(auto && ... args) {
    return boost::hana::make_tuple(std::forward<decltype(args)>(args)...);
}
constexpr auto make_entity(auto && ... args) {
    // make_set?
    return boost::hana::make_tuple(std::forward<decltype(args)>(args)...);
}
constexpr auto make_entities(auto && ... args) {
    // make_set?
    return boost::hana::make_tuple(std::forward<decltype(args)>(args)...);
}

// high level
constexpr auto entities(auto && ... args) {
    return make_entities(std::forward<decltype(args)>(args)...);
}
constexpr auto entity(auto && ... args) {
    return make_entity(std::forward<decltype(args)>(args)...);
}
constexpr auto field(auto && ... args) {
    return field_{ make_field(std::forward<decltype(args)>(args)...) };
}
constexpr auto fields(auto && ... args) {
    return fields_{ make_fields(std::forward<decltype(args)>(args)...) };
}
constexpr auto primary_key(auto && ... args) {
    return primary_key_{ boost::hana::make_tuple(std::forward<decltype(args)>(args)...) };
}
constexpr auto foreign_key(auto && ... args) {
    return foreign_key_{ boost::hana::make_tuple(std::forward<decltype(args)>(args)...) };
}
constexpr auto unique(auto && ... args) {
    return unique_{ boost::hana::make_tuple(std::forward<decltype(args)>(args)...) };
}

enum class sqlite3_column_type {
    integer,
    real,
    text,
    blob,
};

} // namespace primitives::data::schema
