// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/string.h>
#include <primitives/exceptions.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4275)
#endif
#include <yaml-cpp/yaml.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <set>
#include <unordered_set>

#define YAML_EXTRACT_VAR(r, val, var, type) \
    do                                      \
    {                                       \
        auto v = r[var];                    \
        if (v.IsDefined())                  \
            val = v.template as<type>();    \
    } while (0)
#define YAML_EXTRACT(val, type) YAML_EXTRACT_VAR(root, val, #val, type)
#define YAML_EXTRACT_AUTO(val) YAML_EXTRACT(val, decltype(val))
#define YAML_EXTRACT_AUTO2(val, var) YAML_EXTRACT_VAR(root, val, var, decltype(val))

using yaml = YAML::Node;

template <class T>
auto get_scalar(const yaml &node, const String &key, const T &default_ = T())
{
    if (const auto &n = node[key]; n.IsDefined())
    {
        if (!n.IsScalar())
            throw SW_RUNTIME_ERROR("'" + key + "' must be a scalar");
        return n.as<T>();
    }
    return default_;
}

template <class F>
void get_scalar_f(const yaml &node, const String &key, F &&f)
{
    if (const auto &n = node[key]; n.IsDefined())
    {
        if (!n.IsScalar())
            throw SW_RUNTIME_ERROR("'" + key + "' must be a scalar");
        f(n);
    }
}

template <class T = String>
auto get_sequence(const yaml &node)
{
    std::vector<T> result;
    const auto &n = node;
    if (!n.IsDefined())
        return result;
    if (n.IsScalar())
        result.push_back(n.template as<T>());
    else if (n.IsSequence())
    {
        for (const auto &v : n)
            result.push_back(v.template as<T>());
    }
    return result;
}

template <class T>
auto get_sequence(const yaml &node, const String &key, const T &default_ = T())
{
    const auto &n = node[key];
    if (n.IsDefined() && !(n.IsScalar() || n.IsSequence()))
        throw SW_RUNTIME_ERROR("'" + key + "' must be a sequence");
    auto result = get_sequence<T>(n);
    if (!default_.empty())
        result.push_back(default_);
    return result;
}

template <class T>
auto get_sequence_set(const yaml &node)
{
    auto vs = get_sequence<T>(node);
    return std::set<T>(vs.begin(), vs.end());
}

template <class T1, class T2 = T1>
auto get_sequence_set(const yaml &node, const String &key)
{
    auto vs = get_sequence<T2>(node, key);
    return std::set<T1>(vs.begin(), vs.end());
}

template <class T1, class T2 = T1>
auto get_sequence_unordered_set(const yaml &node, const String &key)
{
    auto vs = get_sequence<T2>(node, key);
    return std::unordered_set<T1>(vs.begin(), vs.end());
}

template <class F>
void get_sequence_and_iterate(const yaml &node, const String &key, F &&f)
{
    if (const auto &n = node[key]; n.IsDefined())
    {
        if (n.IsScalar())
        {
            f(n);
        }
        else if (n.IsSequence())
        {
            for (const auto &v : n)
                f(v);
        }
        else
            throw SW_RUNTIME_ERROR("'" + key + "' must be a sequence");
    }
}

template <class F>
void get_map(const yaml &node, const String &key, F &&f)
{
    if (const auto &n = node[key]; n.IsDefined())
    {
        if (!n.IsMap())
            throw SW_RUNTIME_ERROR("'" + key + "' must be a map");
        f(n);
    }
}

template <class F>
void get_map_and_iterate(const yaml &node, const String &key, F &&f)
{
    if (const auto &n = node[key]; n.IsDefined())
    {
        if (!n.IsMap())
            throw SW_RUNTIME_ERROR("'" + key + "' must be a map");
        for (const auto &v : n)
            f(v);
    }
}

template <class T>
void get_string_map(const yaml &node, const String &key, T &data)
{
    if (const auto &n = node[key]; n.IsDefined())
    {
        if (!n.IsMap())
            throw SW_RUNTIME_ERROR("'" + key + "' must be a map");
        for (const auto &v : n)
            data.emplace(v.first.as<String>(), v.second.as<String>());
    }
}

template <class F1, class F2, class F3>
void get_variety(const yaml &node, const String &key, F1 &&f_scalar, F2 &&f_seq, F3 &&f_map)
{
    if (const auto &n = node[key]; n.IsDefined())
    switch (n.Type())
    {
    case YAML::NodeType::Scalar:
        f_scalar(n);
        break;
    case YAML::NodeType::Sequence:
        f_seq(n);
        break;
    case YAML::NodeType::Map:
        f_map(n);
        break;
    }
}

template <class F1, class F3>
void get_variety_and_iterate(const yaml &node, F1 &&f_scalar, F3 &&f_map)
{
    if (const auto &n = node; n.IsDefined())
    switch (n.Type())
    {
    case YAML::NodeType::Scalar:
        f_scalar(n);
        break;
    case YAML::NodeType::Sequence:
        for (const auto &v : n)
            f_scalar(v);
        break;
    case YAML::NodeType::Map:
        for (const auto &v : n)
            f_map(v);
        break;
    }
}

template <class F1, class F3>
void get_variety_and_iterate(const yaml &node, const String &key, F1 &&f_scalar, F3 &&f_map)
{
    const auto &n = node[key];
    get_variety_and_iterate(n, std::forward<F1>(f_scalar), std::forward<F1>(f_map));
}

struct YamlMergeFlags
{
    enum
    {
        ScalarsToSet,
        OverwriteScalars,
        DontTouchScalars,
    };

    int scalar_scalar = 0;
};

// no links allowed
// to do this we call YAML::Clone()
inline void merge(yaml &dst, const yaml &src, const YamlMergeFlags &flags = YamlMergeFlags())
{
    if (!src.IsDefined())
        return;

    // if 'dst' node is not a map, make it so
    if (!dst.IsMap())
        dst = yaml();

    for (const auto &f : src)
    {
        auto sf = f.first.as<String>();
        auto ff = f.second.Type();

        bool found = false;
        for (auto t : dst)
        {
            const auto st = t.first.as<String>();
            if (sf != st)
                continue;

            const auto ft = t.second.Type();
            if (ff == YAML::NodeType::Scalar && ft == YAML::NodeType::Scalar)
            {
                switch (flags.scalar_scalar)
                {
                case YamlMergeFlags::ScalarsToSet:
                {
                    yaml nn;
                    nn.push_back(t.second);
                    nn.push_back(f.second);
                    dst[st] = nn;
                    break;
                }
                case YamlMergeFlags::OverwriteScalars:
                    dst[st] = YAML::Clone(src[sf]);
                    break;
                case YamlMergeFlags::DontTouchScalars:
                    break;
                }
            }
            else if (ff == YAML::NodeType::Scalar && ft == YAML::NodeType::Sequence)
            {
                t.second.push_back(f.second);
            }
            else if (ff == YAML::NodeType::Sequence && ft == YAML::NodeType::Scalar)
            {
                yaml nn = YAML::Clone(f);
                nn.push_back(t.second);
                dst[st] = nn;
            }
            else if (ff == YAML::NodeType::Sequence && ft == YAML::NodeType::Sequence)
            {
                for (const auto &fv : f.second)
                    t.second.push_back(YAML::Clone(fv));
            }
            else if (ff == YAML::NodeType::Map && ft == YAML::NodeType::Map)
                merge(t.second, f.second, flags);
            else // elaborate more on this?
                throw SW_RUNTIME_ERROR("yaml merge: nodes ('" + sf + "') has incompatible types");
            found = true;
        }
        if (!found)
        {
            dst[sf] = YAML::Clone(f.second);
        }
    }
}

