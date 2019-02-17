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

/*
By default performs operations only on releases.
If input version is pre release, searchs for pre releases too.
*/
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

By default performs operations on every version.
    * begin(), end(), rbegin(), rend() etc.

To run things on releases versions, use '_releases' suffix on operations.
    * Call .releases() to get proxy object to iterate in range-for loops.

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

    using iterator_releases = ReleaseIterator<typename Base::iterator>;
    using const_iterator_releases = ReleaseIterator<typename Base::const_iterator>;

public:
    using Base::Base;

    using Base::begin;
    using Base::end;

    // .releases();

    // releases
    iterator_releases begin_releases()
    {
        auto start = std::find_if(begin(), end(), [](const auto &e) {
            if constexpr (detail::is_pair_v<Base::value_type>)
                return std::get<0>(e).isRelease();
            else
                return e.isRelease();
            });
        return { start, end() };
    }
    const_iterator_releases begin_releases() const
    {
        auto start = std::find_if(begin(), end(), [](const auto &e) {
            if constexpr (detail::is_pair_v<Base::value_type>)
                return std::get<0>(e).isRelease();
            else
                return e.isRelease();
            });
        return { start, end() };
    }
    iterator_releases end_releases() { return { end(), end() }; }
    const_iterator_releases end_releases() const { return { end(), end() }; }

    //
    // add range functions?
};

template <
    template <class ...> class BaseContainer,
    class ... Args
>
struct ReverseVersionContainer : VersionContainer<BaseContainer, Args...>
{
    using Base = VersionContainer<BaseContainer, Args...>;
    using This = ReverseVersionContainer;

    using reverse_iterator_releases = typename Base::template ReleaseIterator<typename Base::reverse_iterator>;
    using const_reverse_iterator_releases = typename Base::template ReleaseIterator<typename Base::const_reverse_iterator>;

public:
    using Base::Base;

    using Base::rbegin;
    using Base::rend;

    // .releases();

    // releases, reversed
    reverse_iterator_releases rbegin_releases()
    {
        auto start = std::find_if(rbegin(), rend(), [](const auto &e) {
            if constexpr (detail::is_pair_v<Base::value_type>)
                return std::get<0>(e).isRelease();
            else
                return e.isRelease();
            });
        return { start, rend() };
    }
    const_reverse_iterator_releases rbegin_releases() const
    {
        auto start = std::find_if(rbegin(), rend(), [](const auto &e) {
            if constexpr (detail::is_pair_v<Base::value_type>)
                return std::get<0>(e).isRelease();
            else
                return e.isRelease();
            });
        return { start, rend() };
    }
    reverse_iterator_releases rend_releases() { return { rend(), rend() }; }
    const_reverse_iterator_releases rend_releases() const { return { rend(), rend() }; }

    //
    // add range functions?
};

} // namespace detail

using VersionSet = detail::ReverseVersionContainer<std::set>;

template <class ... Args>
using VersionMap = detail::ReverseVersionContainer<std::map, Args...>;

template <class ... Args>
using UnorderedVersionMap = detail::VersionContainer<std::unordered_map, Args...>;

} // namespace primitives::version
