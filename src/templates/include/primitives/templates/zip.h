#pragma once

#include <iterator>
#include <tuple>
#include <utility>

namespace primitives::templates {

template <typename... T>
struct zip_helper {
    struct iterator {
    private:
        using value_type = std::tuple<decltype(*std::declval<T>().begin())...>;
        std::tuple<decltype(std::declval<T>().begin())...> iters_;

        template <std::size_t... I>
        auto deref(std::index_sequence<I...>) const {
            return typename iterator::value_type{ *std::get<I>(iters_)... };
        }

        template <std::size_t... I>
        void increment(std::index_sequence<I...>) {
            auto l = { (++std::get<I>(iters_), 0)... };
        }

    public:
        explicit iterator(decltype(iters_) iters) : iters_{ std::move(iters) } {
        }

        iterator &operator++() {
            increment(std::index_sequence_for<T...>{});
            return *this;
        }

        iterator operator++(int) {
            auto saved{ *this };
            increment(std::index_sequence_for<T...>{});
            return saved;
        }

        bool operator!=(const iterator &other) const {
            return iters_ != other.iters_;
        }

        auto operator*() const {
            return deref(std::index_sequence_for<T...>{});
        }
    };

    zip_helper(T && ...seqs) : begin_{ std::make_tuple(seqs.begin()...) }, end_{ std::make_tuple(seqs.end()...) } {
    }

    auto begin() const { return begin_; }
    auto end() const { return end_; }

private:
    iterator begin_;
    iterator end_;
};

template <typename... T>
auto zip(T && ... seqs) {
    return zip_helper<T...>{seqs...};
}

//

template <std::size_t N, typename... Tuples>
auto zip_tuples_at(Tuples && ... tuples) {
    return std::forward_as_tuple(std::get<N>(std::forward<Tuples>(tuples))...);
}
template <typename Tuple, typename ...>
constexpr auto indexes() {
    return std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>();
}
template <std::size_t... Ns, typename... Tuples>
constexpr auto zip_tuples_impl(std::index_sequence<Ns...>, Tuples&&... tuples) {
    return std::make_tuple(zip_tuples_at<Ns>(tuples...)...);
}
template <typename... Tuples>
constexpr auto zip_tuples(Tuples && ... tuples) {
    return zip_tuples_impl(indexes<Tuples...>(), std::forward<Tuples>(tuples)...);
    /*return []<std::size_t ... Index>(auto, auto && ... tuples) {
    return std::make_tuple(zip_tuples_at<Index>(tuples...)...);
    }(indexes<Tuples...>(), std::forward<Tuples>(tuples)...);*/
}

template <typename Fn, typename... Tuples>
auto zipped_tuples_map(Fn &&fn, Tuples && ... tuples) {
    std::apply(
        [&fn](auto && ... x) {
        (..., [&fn](auto &&zipped_elements) {
            return std::apply(std::forward<Fn>(fn), zipped_elements);
        }(x));
    },
        tuples_zip(std::forward<Tuples>(tuples)...));
}
//

} // namespace primitives::templates
