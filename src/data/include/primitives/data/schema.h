#pragma once

#include "string.h"

#include <boost/hana.hpp>

namespace primitives::data::schema {

namespace hana = boost::hana;
using string = std::string;

template <typename Class, typename Field>
struct field_pointer {
    Field Class:: *value;
};
template <typename Class, typename ... Fields>
struct fields : field_pointer<Class, Fields>... {
    constexpr fields(Fields Class:: * ... v) : field_pointer<Class, Fields>(v)... {}
    constexpr void for_each(auto &&f) const {
        (f(field_pointer<Class, Fields>::value), ...);
    }
};

//
struct primary_key_t {};
constexpr primary_key_t primary_key;
struct not_null_t {};
constexpr not_null_t not_null;
struct unique_t {};
constexpr unique_t unique;
template <typename T> struct default_t { T value; };
struct table_name : string {};
struct field_name : string {};

enum class action {
    none,
    cascade,
    replace, // conflict only?
};
template <typename Class, typename Field>
struct foreign_key {
    Field Class:: *value;
    action on_delete{action::none};
    action on_update{action::none};
};

namespace table {

template <typename Class, typename ... Fields>
struct primary_key {
    fields<Class, Fields...> fields_;
    //constexpr primary_key(Fields Class:: * ... v) : fields<Class, Fields>(v)... {}
};

template <typename Class, typename ... Fields>
struct unique {
    fields<Class, Fields...> fields_;
    action on_conflict{action::none};
};

} // namespace table

template <typename T>
concept class_type = std::is_class_v<T>;
template <typename T>
concept non_class_type = !class_type<T>;

template <typename T>
concept enum_type = std::is_enum_v<T>;

template <typename T, auto ... attrs>
struct type_with_attrs : T {
    using type = T;

    static constexpr auto attributes() {
        return hana::make_tuple(attrs...);
    }
};
template <non_class_type T, auto ... attrs>
struct type_with_attrs<T, attrs...> {
    using type = T;

    T value;

    static constexpr auto attributes() {
        return hana::make_tuple(attrs...);
    }

    operator T&() { return value; }
    operator const T&() const { return value; }
    type_with_attrs &operator=(auto &&v) { value = v; return *this; }
};
template <typename T, auto ... attrs>
using t = type_with_attrs<T, attrs...>;

TEXT_FIELD(name);
TEXT_FIELD(table_name);

namespace updates {

struct empty { int skip{1}; }; // skip one ore more updates

template <typename T> struct add_table { T value; };
template <typename T, typename C> struct drop_column { T table; C column; };

TEXT_FIELD(raw_query);

} // namespace updates

} // namespace primitives::data::schema
