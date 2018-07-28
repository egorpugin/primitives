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

namespace primitives
{

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

namespace cl
{

// Flags permitted to be passed to command line arguments
//

enum NumOccurrencesFlag { // Flags for the number of occurrences allowed
    Optional = 0x00,        // Zero or One occurrence
    ZeroOrMore = 0x01,      // Zero or more occurrences allowed
    Required = 0x02,        // One occurrence required
    OneOrMore = 0x03,       // One or more occurrences required

                            // ConsumeAfter - Indicates that this option is fed anything that follows the
                            // last positional argument required by the application (it is an error if
                            // there are zero positional arguments, and a ConsumeAfter option is used).
                            // Thus, for example, all arguments to LLI are processed until a filename is
                            // found.  Once a filename is found, all of the succeeding arguments are
                            // passed, unprocessed, to the ConsumeAfter option.
                            //
                            ConsumeAfter = 0x04
};

enum ValueExpected { // Is a value required for the option?
                     // zero reserved for the unspecified value
    ValueOptional = 0x01,  // The value can appear... or not
    ValueRequired = 0x02,  // The value is required to appear!
    ValueDisallowed = 0x03 // A value may not be specified (for flags)
};

enum OptionHidden {   // Control whether -help shows this option
    NotHidden = 0x00,   // Option included in -help & -help-hidden
    Hidden = 0x01,      // -help doesn't, but -help-hidden does
    ReallyHidden = 0x02 // Neither -help nor -help-hidden show this arg
};

// Formatting flags - This controls special features that the option might have
// that cause it to be parsed differently...
//
// Prefix - This option allows arguments that are otherwise unrecognized to be
// matched by options that are a prefix of the actual value.  This is useful for
// cases like a linker, where options are typically of the form '-lfoo' or
// '-L../../include' where -l or -L are the actual flags.  When prefix is
// enabled, and used, the value for the flag comes from the suffix of the
// argument.
//
// Grouping - With this option enabled, multiple letter options are allowed to
// bunch together with only a single hyphen for the whole group.  This allows
// emulation of the behavior that ls uses for example: ls -la === ls -l -a
//

enum FormattingFlags {
    NormalFormatting = 0x00, // Nothing special
    Positional = 0x01,       // Is a positional argument, no '-' required
    Prefix = 0x02,           // Can this option directly prefix its value?
    Grouping = 0x03          // Can this option group with other options?
};

enum MiscFlags {             // Miscellaneous flags to adjust argument
    CommaSeparated = 0x01,     // Should this cl::list split between commas?
    PositionalEatsArgs = 0x02, // Should this positional cl::list eat -args?
    Sink = 0x04                // Should this cl::list eat all unknown options?
};

class SubCommand;

class OptionCategory
{
private:
    String const Name;
    String const Description;

    void registerCategory();

public:
    OptionCategory(const String &Name,
                   const String &Description = "")
        : Name(Name), Description(Description)
    {
        registerCategory();
    }

    String getName() const
    {
        return Name;
    }
    String getDescription() const
    {
        return Description;
    }
};

struct Option
{
    String ArgStr;   // The argument string itself (ex: "help", "o")
    String HelpStr;  // The descriptive text message for -help
    String ValueStr; // String describing what the value of this option is
    OptionCategory *Category; // The Category this option belongs to
    std::vector<SubCommand *> Subs; // The subcommands this option belongs to.
    bool FullyInitialized = false; // Has addArgument been called?

    int NumOccurrences = 0; // The number of times specified
                            // Occurrences, HiddenFlag, and Formatting are all enum types but to avoid
                            // problems with signed enums in bitfields.
    unsigned Occurrences : 3; // enum NumOccurrencesFlag
                              // not using the enum type for 'Value' because zero is an implementation
                              // detail representing the non-value
    unsigned Value : 2;
    unsigned HiddenFlag : 2; // enum OptionHidden
    unsigned Formatting : 2; // enum FormattingFlags
    unsigned Misc : 3;
    unsigned Position = 0;       // Position of last occurrence of the option
    unsigned AdditionalVals = 0; // Greater than 0 for multi-valued option.

    virtual ~Option() = default;

    inline enum NumOccurrencesFlag getNumOccurrencesFlag() const {
        return (enum NumOccurrencesFlag)Occurrences;
    }

    /*inline enum ValueExpected getValueExpectedFlag() const {
        return Value ? ((enum ValueExpected)Value) : getValueExpectedFlagDefault();
    }*/

    inline enum OptionHidden getOptionHiddenFlag() const {
        return (enum OptionHidden)HiddenFlag;
    }

    inline enum FormattingFlags getFormattingFlag() const {
        return (enum FormattingFlags)Formatting;
    }

    inline unsigned getMiscFlags() const { return Misc; }
    inline unsigned getPosition() const { return Position; }
    inline unsigned getNumAdditionalVals() const { return AdditionalVals; }

    // hasArgStr - Return true if the argstr != ""
    bool hasArgStr() const { return !ArgStr.empty(); }
    bool isPositional() const { return getFormattingFlag() == cl::Positional; }
    bool isSink() const { return getMiscFlags() & cl::Sink; }

    bool isConsumeAfter() const {
        return getNumOccurrencesFlag() == cl::ConsumeAfter;
    }

    bool isInAllSubCommands() const;

    //-------------------------------------------------------------------------===
    // Accessor functions set by OptionModifiers
    //
    void setArgStr(const String &S);
    void setDescription(const String &S) { HelpStr = S; }
    void setValueStr(const String &S) { ValueStr = S; }
    void setNumOccurrencesFlag(enum NumOccurrencesFlag Val) { Occurrences = Val; }
    void setValueExpectedFlag(enum ValueExpected Val) { Value = Val; }
    void setHiddenFlag(enum OptionHidden Val) { HiddenFlag = Val; }
    void setFormattingFlag(enum FormattingFlags V) { Formatting = V; }
    void setMiscFlag(enum MiscFlags M) { Misc |= M; }
    void setPosition(unsigned pos) { Position = pos; }
    void setCategory(OptionCategory &C) { Category = &C; }
    void addSubCommand(SubCommand &S) { Subs.push_back(&S); }

    //

    virtual void getExtraOptionNames(Strings &) {}

    // Prints option name followed by message.  Always returns true.
    bool error(const String &Message, String ArgName = {}, std::ostream &Errs = std::cerr);
    bool error(const String &Message, std::ostream &Errs) {
        return error(Message, {}, Errs);
    }

protected:
    explicit Option(enum NumOccurrencesFlag OccurrencesFlag,
                         enum OptionHidden Hidden)
        : Occurrences(OccurrencesFlag), Value(0), HiddenFlag(Hidden),
          Formatting(NormalFormatting), Misc(0)
        //, Category(&GeneralCategory)
    {
    }
};

} // namespace cl

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

template <> struct applicator<cl::NumOccurrencesFlag> {
    static void opt(cl::NumOccurrencesFlag N, cl::Option &O) {
        O.setNumOccurrencesFlag(N);
    }
};

template <> struct applicator<cl::FormattingFlags> {
    static void opt(cl::FormattingFlags FF, cl::Option &O) { O.setFormattingFlag(FF); }
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

namespace cl
{

class SubCommand
{
private:
    String Name;
    String Description;

protected:
    void registerSubCommand();
    void unregisterSubCommand();

public:
    SubCommand(const String &Name, const String &Description = "")
        : Name(Name), Description(Description)
    {
        registerSubCommand();
    }
    SubCommand() = default;

    void reset();

    explicit operator bool() const;

    String getName() const
    {
        return Name;
    }
    String getDescription() const
    {
        return Description;
    }

    std::vector<Option *> PositionalOpts;
    std::vector<Option *> SinkOpts;
    StringHashMap<Option *> OptionsMap;

    Option *ConsumeAfterOpt = nullptr; // The ConsumeAfter option if it exists.
};

struct desc
{
    String description;

    desc(const String &Str) : description(Str) {}

    void apply(Option &O) const { O.setDescription(description); }
};

struct value_desc
{
    String value_description;
};

template <class T>
struct option : Option
{

    template <class... Mods>
    explicit option(const Mods &... Ms)
        : Option(Optional, NotHidden)
        //, Parser(*this)
    {
        detail::apply(this, Ms...);
        //done();
    }
};

template <class T>
using opt = option<T>;

PRIMITIVES_SETTINGS_API
void parseCommandLineOptions(int argc, char **argv, const String &overview = {});

PRIMITIVES_SETTINGS_API
void parseCommandLineOptions(const Strings &args, const String &overview = {});

} // namespace cl

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
primitives::SettingStorage<primitives::Settings> &getSettings(primitives::SettingStorage<primitives::Settings> * = nullptr);

PRIMITIVES_SETTINGS_API
primitives::Settings &getSettings(SettingsType type, primitives::SettingStorage<primitives::Settings> * = nullptr);

////////////////////////////////////////////////////////////////////////////////

} // namespace primitives

using primitives::SettingsType;
