// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Some parts of this code are taken from LLVM.Support.CommandLine.

#pragma once

#include <primitives/enums.h>
#include <primitives/filesystem.h>
#include <primitives/stdcompat/variant.h>
#include <primitives/templates.h>
#include <primitives/yaml.h>

#include <any>
#include <iostream>

#if defined(CPPAN_EXECUTABLE)
#ifdef _WIN32
#define EXPORT_FROM_EXECUTABLE __declspec(dllexport)
#else
// visibility default
// set explicit for clang/gcc on *nix
#endif
#endif

#ifndef EXPORT_FROM_EXECUTABLE
#define EXPORT_FROM_EXECUTABLE
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4190) // function with C-linkage returns UDT
#endif

namespace primitives
{

EXPORT_FROM_EXECUTABLE
String getVersionString();

////////////////////////////////////////////////////////////////////////////////

namespace detail
{

char unescapeSettingChar(char c);
std::string escapeSettingPart(const std::string &s);
std::string cp2utf8(int cp);

} // namespace detail

////////////////////////////////////////////////////////////////////////////////

struct Setting;
struct Settings;

enum class SettingsType
{
    // File or Document? // per document settings!
    // Project
    // Other levels below? Or third one here

    Local,  // in current dir
    User,   // in user's home
    // Group?
    System, // in OS

    Max,

    Default = User
};

////////////////////////////////////////////////////////////////////////////////

// tags

// if data type is changed
//struct discard_malformed_on_load {};

// if data is critical
struct do_not_save {};

namespace detail
{

// applicator class - This class is used because we must use partial
// specialization to handle literal string arguments specially (const char* does
// not correctly respond to the apply method).  Because the syntax to use this
// is a pain, we have the 'apply' method below to handle the nastiness...
//
template <class Mod> struct applicator {
    template <class Opt> static void opt(const Mod &M, Opt &O) { M.apply(O); }
};

// Handle const char* as a special case...
template <unsigned n> struct applicator<char[n]> {
    template <class Opt> static void opt(const String &Str, Opt &O) {
        O.setArgStr(Str);
    }
};
template <unsigned n> struct applicator<const char[n]> {
    template <class Opt> static void opt(const String &Str, Opt &O) {
        O.setArgStr(Str);
    }
};
template <> struct applicator<String> {
    template <class Opt> static void opt(const String &Str, Opt &O) {
        O.setArgStr(Str);
    }
};

template <class Ty> struct storeable {
    const Ty &V;
    storeable(const Ty &Val) : V(Val) {}
};

// init - Specify a default (initial) value for the command line argument, if
// the default constructor for the argument type does not give you what you
// want.  This is only valid on "opt" arguments, not on "list" arguments.
//
template <class Ty> struct initializer : storeable<Ty> {
    using base = storeable<Ty>;
    using base::base;
    template <class Opt> void apply(Opt &O) const { O.setInitialValue(base::V); }
};

//
struct must_not_be : storeable<std::string> {
    using base = storeable<std::string>;
    using base::base;
    template <class Opt> void apply(Opt &O) const { O.addExcludedValue(V); }
};

template <> struct applicator<do_not_save> {
    template <class Opt> static void opt(const do_not_save &, Opt &O) { O.setNonSaveable(); }
};

template <> struct applicator<Settings> {
    template <class Opt> static void opt(const Settings &s, Opt &O) { O.setSettings(s); }
};

template <class Opt, class Mod, class... Mods>
void apply(Opt *O, const Mod &M, const Mods &... Ms) {
    applicator<Mod>::opt(M, *O);
    if constexpr (sizeof...(Ms) > 0)
        apply(O, Ms...);
}

} // namespace detail

template <class Ty> detail::initializer<Ty> init(const Ty &Val) {
    return detail::initializer<Ty>(Val);
}

// check if default value is changed
// this can be used to explicitly state dummy passwords
// in default config files

inline detail::must_not_be must_not_be(const std::string &Val) {
    return detail::must_not_be(Val);
}

////////////////////////////////////////////////////////////////////////////////

struct PRIMITIVES_SETTINGS_API SettingPath
{
    using Parts = Strings;

    SettingPath();
    SettingPath(const String &s);
    SettingPath(const SettingPath &s) = default;
    SettingPath &operator=(const String &s);
    SettingPath &operator=(const SettingPath &s) = default;

    String toString() const;

    const String &at(size_t i) const { return parts.at(i); }
    const String &operator[](size_t i) const { return parts[i]; }
    bool operator<(const SettingPath &rhs) const { return parts < rhs.parts; }
    bool operator==(const SettingPath &rhs) const { return parts == rhs.parts; }

    Parts::iterator begin() { return parts.begin(); }
    Parts::iterator end() { return parts.end(); }
    Parts::const_iterator begin() const { return parts.begin(); }
    Parts::const_iterator end() const { return parts.end(); }

    SettingPath operator/(const SettingPath &rhs) const;

    static SettingPath fromRawString(const String &s);

#ifndef HAVE_BISON_SETTINGS_PARSER
private:
#endif
    Parts parts;

    static bool isBasic(const String &part);
    void parse(const String &s);
};

////////////////////////////////////////////////////////////////////////////////

namespace detail
{

struct parser_base
{
    virtual ~parser_base() = default;

    virtual std::any fromString(const String &value) const = 0;
    virtual String toString(const std::any &value) const = 0;
};

template <class T>
struct parser_to_string_converter : parser_base
{
    String toString(const std::any &value) const override
    {
        return std::to_string(std::any_cast<T>(value));
    }
};

template <class T, typename Enable = void>
struct parser;

#define PARSER_SPECIALIZATION(t, f)                             \
    template <>                                                 \
    struct parser<t, void> : parser_to_string_converter<t>      \
    {                                                           \
        std::any fromString(const String &value) const override \
        {                                                       \
            return f(value);                                    \
        }                                                       \
    }

PARSER_SPECIALIZATION(char, std::stoi);
PARSER_SPECIALIZATION(unsigned char, std::stoi);
PARSER_SPECIALIZATION(short, std::stoi);
PARSER_SPECIALIZATION(unsigned short, std::stoi);
PARSER_SPECIALIZATION(int, std::stoi);
PARSER_SPECIALIZATION(unsigned int, std::stoi);
PARSER_SPECIALIZATION(int64_t, std::stoll);
PARSER_SPECIALIZATION(uint64_t, std::stoull);
PARSER_SPECIALIZATION(float, std::stof);
PARSER_SPECIALIZATION(double, std::stod);

// bool
template <>
struct parser<bool, void> : parser_to_string_converter<bool>
{
    std::any fromString(const String &value) const override
    {
        bool b;
        b = value == "true" || value == "1";
        return b;
    }
};

// std::string
template <>
struct parser<std::string, void> : parser_base
{
    String toString(const std::any &value) const override
    {
        return std::any_cast<std::string>(value);
    }

    std::any fromString(const String &value) const override
    {
        return value;
    }
};

// std::string
template <>
struct parser<path, void> : parser_base
{
    String toString(const std::any &value) const override
    {
        return std::any_cast<path>(value).u8string();
    }

    std::any fromString(const String &value) const override
    {
        return path(value);
    }
};

////////////////////////////////////////////////////////////////////////////////

PRIMITIVES_SETTINGS_API
parser_base *getParser(const std::type_info &);

PRIMITIVES_SETTINGS_API
parser_base *addParser(const std::type_info &, std::unique_ptr<parser_base>);

template <class U>
parser_base *getParser()
{
    auto &ti = typeid(U);
    if (auto p = getParser(ti); p)
        return p;
    return addParser(ti, std::unique_ptr<parser_base>(new parser<U>));
}

} // namespace detail

///////////////////////////////////////////////////////////////////////////////

// rename to value?
struct Setting
{
    // add setting path?
    mutable std::string representation;
    mutable detail::parser_base *parser;
    std::any value;
    std::any defaultValue;
    // store typeid to provide type safety?
    // scope = local, user, thread
    // read only
    bool saveable = true;
    std::unordered_set<String> excludedValues;

    Setting() = default;
    Setting(const Setting &) = default;
    Setting &operator=(const Setting &) = default;

    explicit Setting(const std::any &v)
    {
        value = v;
        defaultValue = v;
    }

    String toString() const;

    template <class U>
    void setType() const
    {
        parser = detail::getParser<U>();
    }

    template <class U>
    Setting &operator=(const U &v)
    {
        if (!representation.empty())
            representation.clear();
        if constexpr (std::is_same_v<char[], U>)
            return operator=(std::string(v));
        else if constexpr (std::is_same_v<const char[], U>)
            return operator=(std::string(v));
        value = v;
        setType<U>();
        return *this;
    }

    bool operator==(char v[]) const
    {
        return std::any_cast<std::string>(value) == v;
    }

    bool operator==(const char v[]) const
    {
        return std::any_cast<std::string>(value) == v;
    }

    template <class U>
    bool operator==(const U &v) const
    {
        if constexpr (std::is_same_v<char[], U>)
            return std::any_cast<std::string>(value) == v;
        else if constexpr (std::is_same_v<const char[], U>)
            return std::any_cast<std::string>(value) == v;
        return std::any_cast<U>(value) == v;
    }

    template <class U>
    bool operator!=(const U &v) const
    {
        return !operator==(v);
    }

    template <class U>
    U &as()
    {
        convert<U>();
        if (auto p = std::any_cast<U>(&value); p)
            return *p;
        throw std::runtime_error("Bad any_cast");
    }

    template <class U>
    const U &as() const
    {
        convert<U>();
        if (auto p = std::any_cast<U>(&value); p)
            return *p;
        throw std::runtime_error("Bad any_cast");
    }

    template <class U>
    void convert()
    {
        if (representation.empty())
            return;
        if (excludedValues.find(representation) != excludedValues.end())
            throw std::runtime_error("Setting value is prohibited: '" + representation + "'");
        setType<U>();
        value = parser->fromString(representation);
        representation.clear();
    }
};

////////////////////////////////////////////////////////////////////////////////

struct Settings;

struct base_setting
{
    Settings *settings;
    Setting *s;

    template <class T>
    void setValue(const T &V, bool initial = false)
    {
        *s = V;
        if (initial)
            s->defaultValue = V;
    }

    void setNonSaveable()
    {
        s->saveable = false;
    }

    void setSettings(const Settings &s)
    {
        settings = (Settings *)&s;
    }
};

template <class DataType>
struct setting : base_setting
{
    using base = base_setting;
    using base::setSettings;

    setting() = default;

    template <class... Mods>
    explicit setting(const Mods &... Ms)
    {
        init(Ms...);
    }

    template <class... Mods>
    void init(const Mods &... Ms)
    {
        detail::apply(this, Ms...);
        s->convert<DataType>();
        if (!s->value.has_value())
            setInitialValue(DataType());
    }

    void setArgStr(const String &name);

    void setInitialValue(const DataType &v, bool initialValue = true)
    {
        s->value = v;
        if (initialValue)
            s->defaultValue = v;
    }

    void addExcludedValue(const String &repr)
    {
        s->excludedValues.insert(repr);
    }

    DataType &getValue()
    {
        return s->as<DataType>();
    }
    DataType getValue() const
    {
        return s->as<DataType>();
    }

    const DataType &getDefault() const
    {
        return s->defaultValue;
    }

    operator DataType() const
    {
        return getValue();
    }

    // If the datatype is a pointer, support -> on it.
    DataType operator->() const
    {
        return s->as<DataType>();
    }

    template <class U>
    setting &operator=(const U &v)
    {
        *s = v;
        return *this;
    }

    template <class U>
    bool operator==(const U &v) const
    {
        return s->operator==(v);
    }

    template <class U>
    bool operator!=(const U &v) const
    {
        return !operator==(v);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct PRIMITIVES_SETTINGS_API Settings
{
    using SettingsMap = std::map<SettingPath, Setting>;

    SettingPath prefix;

    Settings(const Settings &s) = default;
    Settings &operator=(const Settings &s) = default;

    void clear();

    // remove type parameter?
    void load(const path &fn, SettingsType type = SettingsType::Default);
    void load(const String &s, SettingsType type = SettingsType::Default);
    void load(const yaml &root, const SettingPath &prefix = {});

    // dump to object
    String dump() const;
    void dump(yaml &root) const;

    // save to file (default storage)
    void save(const path &fn = {}) const;
    // also from yaml, json, registry
    bool hasSaveableItems() const;

    Setting &operator[](const String &k);
    const Setting &operator[](const String &k) const;
    Setting &operator[](const SettingPath &k);
    const Setting &operator[](const SettingPath &k) const;
    //void getSubSettings();

    SettingsMap::iterator begin() { return settings.begin(); }
    SettingsMap::iterator end() { return settings.end(); }

    SettingsMap::const_iterator begin() const { return settings.begin(); }
    SettingsMap::const_iterator end() const { return settings.end(); }

#ifndef HAVE_BISON_SETTINGS_PARSER
private:
#endif
    SettingsMap settings;

    Settings() = default;

    template <class T>
    friend struct SettingStorage;
};

template <class T>
void setting<T>::setArgStr(const String &name)
{
    s = &(*settings)[name];
    s->setType<T>();
}

////////////////////////////////////////////////////////////////////////////////

struct SettingStorageBase
{
    path systemConfigFilename;
    path userConfigFilename;
};

template <class T>
struct SettingStorage : SettingStorageBase
{
    SettingStorage();

    T &getSystemSettings();
    T &getUserSettings();
    T &getLocalSettings();

    void clearLocalSettings();

    T &get(SettingsType type = SettingsType::Default);

    // We could add here more interfaces from Settings,
    // but instead we add getSettings(type) overload.

private:
    T settings[toIndex(SettingsType::Max)];
};

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4661)
PRIMITIVES_SETTINGS_API_EXTERN
template struct PRIMITIVES_SETTINGS_API SettingStorage<::primitives::Settings>;
#pragma warning(pop)
#endif

////////////////////////////////////////////////////////////////////////////////

/// Users may want to define their own getSettings()
/// with some initial setup.
/// 'settings_auto' package will also provide tuned global getSettings() to use.
PRIMITIVES_SETTINGS_API
primitives::SettingStorage<primitives::Settings> &getSettingStorage(primitives::SettingStorage<primitives::Settings> * = nullptr);

PRIMITIVES_SETTINGS_API
primitives::Settings &getSettings(SettingsType type, primitives::SettingStorage<primitives::Settings> * = nullptr);

////////////////////////////////////////////////////////////////////////////////

} // namespace primitives
