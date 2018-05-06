// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/string.h>

#include <yaml-cpp/yaml.h>

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

using yaml = YAML::Node;

template <class T>
auto get_scalar(const yaml &node, const String &key, const T &default_ = T())
{
    if (const auto &n = node[key]; n.IsDefined())
    {
        if (!n.IsScalar())
            throw std::runtime_error("'" + key + "' must be a scalar");
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
            throw std::runtime_error("'" + key + "' must be a scalar");
        f(n);
    }
}

template <class T>
auto get_sequence(const yaml &node)
{
    std::vector<T> result;
    const auto &n = node;
    if (!n.IsDefined())
        return result;
    if (n.IsScalar())
        result.push_back(n.as<String>());
    else if (n.IsSequence())
    {
        for (const auto &v : n)
            result.push_back(v.as<String>());
    }
    return result;
}

template <class T>
auto get_sequence(const yaml &node, const String &key, const T &default_ = T())
{
    const auto &n = node[key];
    if (n.IsDefined() && !(n.IsScalar() || n.IsSequence()))
        throw std::runtime_error("'" + key + "' must be a sequence");
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
            throw std::runtime_error("'" + key + "' must be a sequence");
    }
}

template <class F>
void get_map(const yaml &node, const String &key, F &&f)
{
    if (const auto &n = node[key]; n.IsDefined())
    {
        if (!n.IsMap())
            throw std::runtime_error("'" + key + "' must be a map");
        f(n);
    }
}

template <class F>
void get_map_and_iterate(const yaml &node, const String &key, F &&f)
{
    if (const auto &n = node[key]; n.IsDefined())
    {
        if (!n.IsMap())
            throw std::runtime_error("'" + key + "' must be a map");
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
            throw std::runtime_error("'" + key + "' must be a map");
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

PRIMITIVES_YAML_API
void merge(yaml &dst, const yaml &src, const YamlMergeFlags &flags = YamlMergeFlags());
