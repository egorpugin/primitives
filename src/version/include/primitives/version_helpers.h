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
auto findBestMatch(const T &begin, const T &end, const Version &what)
{
    auto i = std::max_element(begin, end, [&what](const auto &e1, const auto &e2) {
        //if constexpr (detail::is_pair_v<std::iterator_traits<T>::value_type>)
        if constexpr (detail::is_pair_v<T::value_type>)
            return std::get<0>(e1).getMatchingLevel(what) < std::get<0>(e2).getMatchingLevel(what);
        else
            return e1.getMatchingLevel(what) < e2.getMatchingLevel(what);
    });
    if (i == end)
        return i;
    if constexpr (detail::is_pair_v<T::value_type>)
        return std::get<0>(*i).getMatchingLevel(what) == 0 ? end : i;
    else
        return i->getMatchingLevel(what) == 0 ? end : i;
}

namespace detail
{

/*

Version container.

By default performs operations only on releases.

    * begin(), end(), rbegin(), rend() etc.

To run things on all versions, use '_all' or similar 'All' suffixes on all operations.
Call .all() to get proxy object to iterate in range-for loops.

*/
template <
    template <class ...> class BaseContainer,
    class ... Args
>
struct VersionContainer : BaseContainer<Version, Args...>
{
    using Base = BaseContainer<Version, Args...>;
    using This = VersionContainer;

    using iterator_all = typename Base::iterator;
    using const_iterator_all = typename Base::const_iterator;

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

    using Base::Base;

    // all();

    // everything
    auto findBestMatchFromAll(const Version &what)
    {
        return ::primitives::version::findBestMatch(begin_all(), end_all(), what);
    }

    auto findBestMatchFromAll(const Version &what) const
    {
        return ::primitives::version::findBestMatch(begin_all(), end_all(), what);
    }

    auto rfindBestMatchFromAll(const Version &what)
    {
        return ::primitives::version::findBestMatch(rbegin_all(), rend_all(), what);
    }

    auto rfindBestMatchFromAll(const Version &what) const
    {
        return ::primitives::version::findBestMatch(rbegin_all(), rend_all(), what);
    }

    // release only
    // but if 'what' is passed as pre prelease itself, we call all variants
    auto findBestMatch(const Version &what)
    {
        if (what.isPreRelease())
            return findBestMatchFromAll(what);
        auto i = ::primitives::version::findBestMatch(begin(), end(), what);
        return static_cast<typename Base::iterator>(i);
    }

    auto findBestMatch(const Version &what) const
    {
        if (what.isPreRelease())
            return findBestMatchFromAll(what);
        auto i = ::primitives::version::findBestMatch(begin(), end(), what);
        return static_cast<typename Base::const_iterator>(i);
    }

    auto rfindBestMatch(const Version &what)
    {
        if (what.isPreRelease())
            return rfindBestMatchFromAll(what);
        auto i = ::primitives::version::findBestMatch(rbegin(), rend(), what);
        return static_cast<typename Base::reverse_iterator>(i);
    }

    auto rfindBestMatch(const Version &what) const
    {
        if (what.isPreRelease())
            return rfindBestMatchFromAll(what);
        auto i = ::primitives::version::findBestMatch(rbegin(), rend(), what);
        return static_cast<typename Base::const_reverse_iterator>(i);
    }

    // releases
    auto begin()
    {
        auto start = std::find_if(begin_all(), end_all(), [](const auto &e) {
            if constexpr (detail::is_pair_v<Base::value_type>)
                return std::get<0>(e).isRelease();
            else
                return e.isRelease();
            });
        return ReleaseIterator<decltype(start)>{start, end_all()};
    }

    auto begin() const
    {
        auto start = std::find_if(begin_all(), end_all(), [](const auto &e) {
            if constexpr (detail::is_pair_v<Base::value_type>)
                return std::get<0>(e).isRelease();
            else
                return e.isRelease();
            });
        return ReleaseIterator<decltype(start)>{start, end_all()};
    }

    auto end() { return ReleaseIterator<typename Base::iterator>{ end_all(), end_all() }; }
    auto end() const { return ReleaseIterator<typename Base::const_iterator>{ end_all(), end_all() }; }

    // releases, reversed
    auto rbegin()
    {
        auto start = std::find_if(rbegin_all(), rend_all(), [](const auto &e) {
            if constexpr (detail::is_pair_v<Base::value_type>)
                return std::get<0>(e).isRelease();
            else
                return e.isRelease();
            });
        return ReleaseIterator<decltype(start)>{start, rend_all()};
    }

    auto rbegin() const
    {
        auto start = std::find_if(rbegin_all(), rend_all(), [](const auto &e) {
            if constexpr (detail::is_pair_v<Base::value_type>)
                return std::get<0>(e).isRelease();
            else
                return e.isRelease();
            });
        return ReleaseIterator<decltype(start)>{start, rend_all()};
    }

    auto rend() { return ReleaseIterator<typename Base::reverse_iterator>{ rend_all(), rend_all() }; }
    auto rend() const { return ReleaseIterator<typename Base::const_reverse_iterator>{ rend_all(), rend_all() }; }

    // pre releases
    iterator_all begin_all() { return Base::begin(); }
    const_iterator_all begin_all() const { return Base::begin(); }
    iterator_all end_all() { return Base::end(); }
    const_iterator_all end_all() const { return Base::end(); }

    // pre releases, reversed
    auto rbegin_all() { return Base::rbegin(); }
    auto rbegin_all() const { return Base::rbegin(); }
    auto rend_all() { return Base::rend(); }
    auto rend_all() const { return Base::rend(); }

    //
    // TODO: also add range functions?
};

template <class ... Args>
struct ReverseVersionContainer : VersionContainer<Args...>
{
    using Base = VersionContainer<Args...>;
    using This = ReverseVersionContainer;

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

    using Base::Base;

    // all();

    // everything
    auto findBestMatchFromAll(const Version &what)
    {
        return ::primitives::version::findBestMatch(begin_all(), end_all(), what);
    }

    auto findBestMatchFromAll(const Version &what) const
    {
        return ::primitives::version::findBestMatch(begin_all(), end_all(), what);
    }

    auto rfindBestMatchFromAll(const Version &what)
    {
        return ::primitives::version::findBestMatch(rbegin_all(), rend_all(), what);
    }

    auto rfindBestMatchFromAll(const Version &what) const
    {
        return ::primitives::version::findBestMatch(rbegin_all(), rend_all(), what);
    }

    // release only
    // but if 'what' is passed as pre prelease itself, we call all variants
    auto findBestMatch(const Version &what)
    {
        if (what.isPreRelease())
            return findBestMatchFromAll(what);
        auto i = ::primitives::version::findBestMatch(begin(), end(), what);
        return static_cast<typename Base::iterator>(i);
    }

    auto findBestMatch(const Version &what) const
    {
        if (what.isPreRelease())
            return findBestMatchFromAll(what);
        auto i = ::primitives::version::findBestMatch(begin(), end(), what);
        return static_cast<typename Base::const_iterator>(i);
    }

    auto rfindBestMatch(const Version &what)
    {
        if (what.isPreRelease())
            return rfindBestMatchFromAll(what);
        auto i = ::primitives::version::findBestMatch(rbegin(), rend(), what);
        return static_cast<typename Base::reverse_iterator>(i);
    }

    auto rfindBestMatch(const Version &what) const
    {
        if (what.isPreRelease())
            return rfindBestMatchFromAll(what);
        auto i = ::primitives::version::findBestMatch(rbegin(), rend(), what);
        return static_cast<typename Base::const_reverse_iterator>(i);
    }

    // releases
    auto begin()
    {
        auto start = std::find_if(begin_all(), end_all(), [](const auto &e) {
            if constexpr (detail::is_pair_v<Base::value_type>)
                return std::get<0>(e).isRelease();
            else
                return e.isRelease();
            });
        return ReleaseIterator<decltype(start)>{start, end_all()};
    }

    auto begin() const
    {
        auto start = std::find_if(begin_all(), end_all(), [](const auto &e) {
            if constexpr (detail::is_pair_v<Base::value_type>)
                return std::get<0>(e).isRelease();
            else
                return e.isRelease();
            });
        return ReleaseIterator<decltype(start)>{start, end_all()};
    }

    auto end() { return ReleaseIterator<typename Base::iterator>{ end_all(), end_all() }; }
    auto end() const { return ReleaseIterator<typename Base::const_iterator>{ end_all(), end_all() }; }

    // releases, reversed
    auto rbegin()
    {
        auto start = std::find_if(rbegin_all(), rend_all(), [](const auto &e) {
            if constexpr (detail::is_pair_v<Base::value_type>)
                return std::get<0>(e).isRelease();
            else
                return e.isRelease();
            });
        return ReleaseIterator<decltype(start)>{start, rend_all()};
    }

    auto rbegin() const
    {
        auto start = std::find_if(rbegin_all(), rend_all(), [](const auto &e) {
            if constexpr (detail::is_pair_v<Base::value_type>)
                return std::get<0>(e).isRelease();
            else
                return e.isRelease();
            });
        return ReleaseIterator<decltype(start)>{start, rend_all()};
    }

    auto rend() { return ReleaseIterator<typename Base::reverse_iterator>{ rend_all(), rend_all() }; }
    auto rend() const { return ReleaseIterator<typename Base::const_reverse_iterator>{ rend_all(), rend_all() }; }

    // pre releases
    iterator_all begin_all() { return Base::begin(); }
    const_iterator_all begin_all() const { return Base::begin(); }
    iterator_all end_all() { return Base::end(); }
    const_iterator_all end_all() const { return Base::end(); }

    // pre releases, reversed
    auto rbegin_all() { return Base::rbegin(); }
    auto rbegin_all() const { return Base::rbegin(); }
    auto rend_all() { return Base::rend(); }
    auto rend_all() const { return Base::rend(); }

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
