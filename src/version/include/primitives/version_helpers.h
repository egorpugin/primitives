// Copyright (C) 2018-2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "version.h"

#include <algorithm>
#include <map>
#include <unordered_map>

namespace primitives::version
{

namespace detail
{

template <typename T>
struct is_pair : std::false_type { };

template <typename T, typename U>
struct is_pair<std::pair<T, U>> : std::true_type { };

template <typename T>
constexpr bool is_pair_v = is_pair<T>::value;

} // namespace detail

template <class T>
auto findBestMatch(const T &begin, const T &end, const Version &what, bool accept_prereleases = false)
{
    if (what.isPreRelease())
        accept_prereleases = true;

    Version::Level max = 0;
    auto imax = end;
    for (auto i = begin; i != end; ++i)
    {
        Version::Level l;

        //if constexpr (detail::is_pair_v<std::iterator_traits<T>::value_type>)
        if constexpr (detail::is_pair_v<T::value_type>)
        {
            if (!accept_prereleases && std::get<0>(*i).isPreRelease())
                continue;
            l = std::get<0>(*i).getMatchingLevel(what);
        }
        else
        {
            if (!accept_prereleases && i->isPreRelease())
                continue;
            l = i->getMatchingLevel(what);
        }

        if (l > max)
        {
            max = l;
            imax = i;
        }
    }
    return imax;
}

namespace detail
{

/*

Version container.

By default performs operations only on releases.
    * begin(), end(), rbegin(), rend() etc.

To run things on all versions, use '_all' or similar 'All' suffixes on all operations.
    * Call .all() to get proxy object to iterate in range-for loops.

*/
template <
    template <class ...> class BaseContainer,
    class ... Args
>
struct VersionContainer : BaseContainer<Version, Args...>
{
    using Base = BaseContainer<Version, Args...>;
    using This = VersionContainer;

    template <class base_iterator>
    struct ReleaseIterator : base_iterator
    {
        using value_type = typename base_iterator::value_type;

        base_iterator end;

        ReleaseIterator(const base_iterator &in, const base_iterator &end)
            : base_iterator(in), end(end)
        {
        }

        bool operator==(const ReleaseIterator &rhs) const
        {
            return (const base_iterator&)(*this) == (const base_iterator&)(rhs);
        }

        bool operator!=(const ReleaseIterator &rhs) const
        {
            return !operator==(rhs);
        }

        ReleaseIterator &operator++()
        {
            move_to_next();
            return *this;
        }

    private:
        void move_to_next()
        {
            while (1)
            {
                ++((base_iterator&)(*this));
                if ((base_iterator&)*this == end)
                    return;
                if constexpr (detail::is_pair_v<Base::value_type>)
                {
                    if (std::get<0>(*(*this)).isRelease())
                        break;
                }
                else
                {
                    if ((*this)->isRelease())
                        break;
                }
            }
        }
    };

    using iterator_all = typename Base::iterator;
    using const_iterator_all = typename Base::const_iterator;

    using iterator = ReleaseIterator<iterator_all>;
    using const_iterator = ReleaseIterator<const_iterator_all>;

public:
    using Base::Base;

    // .all();

    // everything
    iterator_all findBestMatchFromAll(const Version &what)
    {
        return ::primitives::version::findBestMatch(begin_all(), end_all(), what);
    }
    const_iterator_all findBestMatchFromAll(const Version &what) const
    {
        return ::primitives::version::findBestMatch(begin_all(), end_all(), what);
    }

    // release only
    // but if 'what' is passed as pre prelease itself, we call all variants
    iterator findBestMatch(const Version &what)
    {
        if (what.isPreRelease())
            return findBestMatchFromAll(what);
        auto i = ::primitives::version::findBestMatch(begin(), end(), what);
        return static_cast<typename Base::iterator>(i);
    }
    const_iterator findBestMatch(const Version &what) const
    {
        if (what.isPreRelease())
            return findBestMatchFromAll(what);
        auto i = ::primitives::version::findBestMatch(begin(), end(), what);
        return static_cast<typename Base::const_iterator>(i);
    }

    // releases
    iterator begin()
    {
        auto start = std::find_if(begin_all(), end_all(), [](const auto &e) {
            if constexpr (detail::is_pair_v<Base::value_type>)
                return std::get<0>(e).isRelease();
            else
                return e.isRelease();
            });
        return {start, end_all()};
    }
    const_iterator begin() const
    {
        auto start = std::find_if(begin_all(), end_all(), [](const auto &e) {
            if constexpr (detail::is_pair_v<Base::value_type>)
                return std::get<0>(e).isRelease();
            else
                return e.isRelease();
            });
        return {start, end_all()};
    }
    iterator end() { return { end_all(), end_all() }; }
    const_iterator end() const { return { end_all(), end_all() }; }

    // pre releases
    iterator_all begin_all() { return Base::begin(); }
    const_iterator_all begin_all() const { return Base::begin(); }
    iterator_all end_all() { return Base::end(); }
    const_iterator_all end_all() const { return Base::end(); }

    //
    // TODO: also add range functions?
};

template <
    template <class ...> class BaseContainer,
    class ... Args
>
struct ReverseVersionContainer : VersionContainer<BaseContainer, Args...>
{
    using Base = VersionContainer<BaseContainer, Args...>;
    using This = ReverseVersionContainer;

    using reverse_iterator_all = typename Base::reverse_iterator;
    using const_reverse_iterator_all = typename Base::const_reverse_iterator;

    using reverse_iterator = typename Base::template ReleaseIterator<reverse_iterator_all>;
    using const_reverse_iterator = typename Base::template ReleaseIterator<const_reverse_iterator_all>;

    using Base::Base;

    // .all();

    // everything
    reverse_iterator_all rfindBestMatchFromAll(const Version &what)
    {
        return ::primitives::version::findBestMatch(rbegin_all(), rend_all(), what);
    }
    const_reverse_iterator_all rfindBestMatchFromAll(const Version &what) const
    {
        return ::primitives::version::findBestMatch(rbegin_all(), rend_all(), what);
    }

    // release only
    // but if 'what' is passed as pre prelease itself, we call all variants
    reverse_iterator rfindBestMatch(const Version &what)
    {
        if (what.isPreRelease())
            return rfindBestMatchFromAll(what);
        auto i = ::primitives::version::findBestMatch(rbegin(), rend(), what);
        return static_cast<typename Base::reverse_iterator>(i);
    }
    const_reverse_iterator rfindBestMatch(const Version &what) const
    {
        if (what.isPreRelease())
            return rfindBestMatchFromAll(what);
        auto i = ::primitives::version::findBestMatch(rbegin(), rend(), what);
        return static_cast<typename Base::const_reverse_iterator>(i);
    }

    // releases, reversed
    reverse_iterator rbegin()
    {
        auto start = std::find_if(rbegin_all(), rend_all(), [](const auto &e) {
            if constexpr (detail::is_pair_v<Base::value_type>)
                return std::get<0>(e).isRelease();
            else
                return e.isRelease();
            });
        return {start, rend_all()};
    }
    const_reverse_iterator rbegin() const
    {
        auto start = std::find_if(rbegin_all(), rend_all(), [](const auto &e) {
            if constexpr (detail::is_pair_v<Base::value_type>)
                return std::get<0>(e).isRelease();
            else
                return e.isRelease();
            });
        return {start, rend_all()};
    }
    reverse_iterator rend() { return { rend_all(), rend_all() }; }
    const_reverse_iterator rend() const { return { rend_all(), rend_all() }; }

    // pre releases, reversed
    reverse_iterator_all rbegin_all() { return Base::rbegin(); }
    const_reverse_iterator_all rbegin_all() const { return Base::rbegin(); }
    reverse_iterator_all rend_all() { return Base::rend(); }
    const_reverse_iterator_all rend_all() const { return Base::rend(); }

    //
    // TODO: also add range functions?
};

} // namespace detail

using VersionSet = detail::ReverseVersionContainer<std::set>;

template <class ... Args>
using VersionMap = detail::ReverseVersionContainer<std::map, Args...>;

template <class ... Args>
using UnorderedVersionMap = detail::VersionContainer<std::unordered_map, Args...>;

} // namespace primitives::version
