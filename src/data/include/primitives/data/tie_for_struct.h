#include <type_traits>
#include <utility>
#include <algorithm>

namespace primitives::data {

template <typename T>
concept aggregate = std::is_aggregate_v<T>;

template <typename T, typename... Args>
concept aggregate_initializable = aggregate<T> && requires { T{std::declval<Args>()...}; };

namespace detail {

struct any {
    template <typename T> constexpr operator T() const noexcept;
};

template <std::size_t I>
using indexed_any = any;

template <aggregate T, typename Indices>
struct aggregate_initializable_from_indices;

template <aggregate T, std::size_t... Indices>
struct aggregate_initializable_from_indices<T, std::index_sequence<Indices...>>
    : std::bool_constant<aggregate_initializable<T, indexed_any<Indices>...>> {};

}

template <typename T, std::size_t N>
concept aggregate_initializable_with_n_args = aggregate<T> &&
    detail::aggregate_initializable_from_indices<T, std::make_index_sequence<N>>::value;

template <template<std::size_t> typename Predicate, std::size_t Beg, std::size_t End>
struct binary_search;

template <template<std::size_t> typename Predicate, std::size_t Beg, std::size_t End>
using binary_search_base =
    std::conditional_t<(End - Beg <= 1), std::integral_constant<std::size_t, Beg>,
        std::conditional_t<Predicate<(Beg + End) / 2>::value,
            binary_search<Predicate, (Beg + End) / 2, End>,
            binary_search<Predicate, Beg, (Beg + End) / 2>>>;

template <template<std::size_t> typename Predicate, std::size_t Beg, std::size_t End>
struct binary_search : binary_search_base<Predicate, Beg, End> {};

template <template<std::size_t> typename Predicate, std::size_t Beg, std::size_t End>
constexpr std::size_t binary_search_v = binary_search<Predicate, Beg, End>::value;

namespace detail {

template <aggregate T, std::size_t N, bool CanInitialize>
struct aggregate_field_count : aggregate_field_count<T, N+1,
    aggregate_initializable_with_n_args<T, N+1>> {};

template <aggregate T, std::size_t N>
struct aggregate_field_count<T, N, false>
    : std::integral_constant<std::size_t, N-1> {};

template <aggregate T>
struct aggregate_inquiry {
    template <std::size_t N>
    struct initializable : std::bool_constant<aggregate_initializable_with_n_args<T, N>> {};
};

}

template <aggregate T>
struct num_aggregate_fields : binary_search<
    detail::aggregate_inquiry<T>::template initializable, 0, 8 * sizeof(T) + 1> {};

template <aggregate T>
constexpr std::size_t num_aggregate_fields_v = num_aggregate_fields<T>::value;

namespace detail {

template <aggregate T, typename Indices, typename FieldIndices>
struct aggregate_with_indices_initializable_with;

template <aggregate T, std::size_t... Indices, std::size_t... FieldIndices>
struct aggregate_with_indices_initializable_with<T,
    std::index_sequence<Indices...>, std::index_sequence<FieldIndices...>>
    : std::bool_constant<requires
    {
        T{std::declval<indexed_any<Indices>>()...,
            {std::declval<indexed_any<FieldIndices>>()...}};
    }> {};

}

template <typename T, std::size_t N, std::size_t M>
concept aggregate_field_n_initializable_with_m_args = aggregate<T> &&
    detail::aggregate_with_indices_initializable_with<T,
        std::make_index_sequence<N>,
        std::make_index_sequence<M>>::value;

namespace detail {

template <aggregate T, std::size_t N>
struct aggregate_field_inquiry
{
    template <std::size_t M>
    struct initializable : std::bool_constant<
        aggregate_field_n_initializable_with_m_args<T, N, M>> {};
};

}

template <aggregate T, std::size_t N>
struct num_fields_on_aggregate_field : binary_search<
    detail::aggregate_field_inquiry<T, N>::template initializable,
    0, 8 * sizeof(T) + 1> {};

template <aggregate T, std::size_t N>
constexpr std::size_t num_fields_on_aggregate_field_v
    = num_fields_on_aggregate_field<T, N>::value;

namespace detail {

template <std::size_t Val, std::size_t TotalFields>
constexpr auto detect_special_type =
    Val > TotalFields ? 1 : Val;

template <aggregate T, std::size_t CurField,
    std::size_t TotalFields, std::size_t CountUniqueFields>
struct unique_field_counter : unique_field_counter<T,
    CurField + detect_special_type<
        num_fields_on_aggregate_field_v<T, CurField>, TotalFields>,
    TotalFields, CountUniqueFields + 1> {};

template <aggregate T, std::size_t Fields, std::size_t UniqueFields>
struct unique_field_counter<T, Fields, Fields, UniqueFields>
    : std::integral_constant<std::size_t, UniqueFields> {};

}

template <aggregate T>
struct num_aggregate_unique_fields
    : detail::unique_field_counter<T, 0, num_aggregate_fields_v<T>, 0> {};

template <aggregate T>
constexpr std::size_t num_aggregate_unique_fields_v = num_aggregate_unique_fields<T>::value;

constexpr auto tie_as_tuple(auto &&data) {
    using T = std::decay_t<decltype(data)>;
    constexpr auto N = num_aggregate_unique_fields_v<T>;

    if constexpr (0);
#include <primitives/data/tie_for_struct.inl>
}

} // namespace primitives::data
