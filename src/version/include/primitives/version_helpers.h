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

*/
template <
    template <class ...> class BaseContainer,
    class ... Args
>
struct VersionContainer : BaseContainer<Version, Args...>
{
    using Base = BaseContainer<Version, Args...>;
    using This = VersionContainer;

public:
    using Base::Base;

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

    using Base::Base;

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
