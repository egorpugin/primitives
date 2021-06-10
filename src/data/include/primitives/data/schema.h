#pragma once

#include <boost/hana.hpp>

//#include <optional>
//#include <string>

namespace primitives::data::schema {

inline namespace common_properties {

struct name_ {
    const char *name;
    constexpr name_(const char *n) : name(n) {}
};
template <name_ n>
constexpr name_ operator ""_name() {
    return n;
}

} // namespace common_properties

inline namespace table_properties {

#define TYPED_STORAGE(x)                                                                                               \
    template <typename T>                                                                                              \
    struct x##_ {                                                                                                      \
        T x;                                                                                                           \
        constexpr x##_(T &&v) : x{v} {                                                                                 \
        }                                                                                                              \
    };                                                                                                                 \
    template <typename T>                                                                                              \
    x##_(T &&v)->x##_<T>;                                                                                              \
                                                                                                                       \
    template <typename T>                                                                                              \
    struct is_##x##_ : std::false_type {};                                                                             \
    template <typename T>                                                                                              \
    struct is_##x##_<x##_<T>> : std::true_type {};                                                                     \
    constexpr auto is_##x = boost::hana::compose(boost::hana::trait<is_##x##_>, boost::hana::typeid_)

TYPED_STORAGE(fields);
TYPED_STORAGE(primary_key);
TYPED_STORAGE(foreign_key);
TYPED_STORAGE(unique);

#undef TYPED_STORAGE

} // inline namespace table_properties

inline namespace field_properties {

#define FIELD_CHECK(x)                                                                                                 \
    template <typename T>                                                                                              \
    struct is_##x##_ : std::false_type {};                                                                             \
    template <>                                                                                                        \
    struct is_##x##_<x> : std::true_type {};                                                                           \
    constexpr auto is_##x = boost::hana::compose(boost::hana::trait<is_##x##_>, boost::hana::typeid_)

template <typename T>
struct type {
    using type_t = T;
};

/*struct primary_key {
    auto operator<=>(const primary_key &) const = default;
};*/

struct optional {
    auto operator<=>(const optional &) const = default;
};
FIELD_CHECK(optional);

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

} // inline namespace field_properties

// low level
constexpr auto make_field(auto && ... args) {
    return boost::hana::make_tuple(std::forward<decltype(args)>(args)...);
}
constexpr auto make_fields(auto && ... args) {
    return boost::hana::make_tuple(std::forward<decltype(args)>(args)...);
}
constexpr auto make_entity(auto && ... args) {
    return boost::hana::make_tuple(std::forward<decltype(args)>(args)...);
}
constexpr auto make_entities(auto && ... args) {
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
    return make_field(std::forward<decltype(args)>(args)...);
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

} // namespace primitives::data::schema
