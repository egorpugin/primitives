// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/settings.h>

#include <path_parser.h>
#include <settings_parser.h>

#include <primitives/overload.h>
#include <primitives/error_handling.h>

#include <boost/algorithm/string.hpp>

#include <mutex>

using Args = std::list<std::string>;

namespace primitives
{

namespace detail
{

char unescapeSettingChar(char c)
{
    switch (c)
    {
#define M(f, t) case f: return t
        M('n', '\n');
        M('\"', '\"');
        M('\\', '\\');
        M('r', '\r');
        M('t', '\t');
        M('f', '\f');
        M('b', '\b');
#undef M
    }
    return 0;
}

String escapeSettingPart(const String &s)
{
    String s2;
    for (auto &c : s)
    {
        if (c >= 0 && c <= 127 && iscntrl(c))
        {
            static const auto hex = "0123456789ABCDEF";
            s2 += "\\u00";
            s2 += hex[(c & 0xF0) >> 4];
            s2 += hex[c & 0x0F];
        }
        else
            s2 += c;
    }
    return s2;
}

std::string cp2utf8(int cp)
{
    char c[5] = { 0x00,0x00,0x00,0x00,0x00 };
    if (cp <= 0x7F) { c[0] = cp; }
    else if (cp <= 0x7FF) { c[0] = (cp >> 6) + 192; c[1] = (cp & 63) + 128; }
    else if (0xd800 <= cp && cp <= 0xdfff) {} //invalid block of utf8
    else if (cp <= 0xFFFF) { c[0] = (cp >> 12) + 224; c[1] = ((cp >> 6) & 63) + 128; c[2] = (cp & 63) + 128; }
    else if (cp <= 0x10FFFF) { c[0] = (cp >> 18) + 240; c[1] = ((cp >> 12) & 63) + 128; c[2] = ((cp >> 6) & 63) + 128; c[3] = (cp & 63) + 128; }
    return std::string(c);
}

auto &getParserStorage()
{
    static std::map<size_t, std::unique_ptr<parser_base>> parsers;
    return parsers;
}

parser_base *getParser(const std::type_info &t)
{
    auto &parsers = getParserStorage();
    auto i = parsers.find(t.hash_code());
    if (i == parsers.end())
        return nullptr;
    return i->second.get();
}

parser_base *addParser(const std::type_info &t, std::unique_ptr<parser_base> p)
{
    static std::mutex m;
    std::unique_lock lk(m);
    auto &parsers = getParserStorage();
    auto ptr = p.get();
    parsers[t.hash_code()] = std::move(p);
    return ptr;
}

} // namespace detail

namespace cl
{

// A special subcommand representing no subcommand
SubCommand TopLevelSubCommand;

// A special subcommand that can be used to put an option into all subcommands.
SubCommand AllSubCommands;

//===----------------------------------------------------------------------===//

namespace
{

struct CommandLineParser
{
    // Globals for name and overview of program.  Program name is not a string to
    // avoid static ctor/dtor issues.
    std::string ProgramName;
    String ProgramOverview;

    // This collects additional help to be printed.
    Strings MoreHelp;

    // This collects the different option categories that have been registered.
    std::set<OptionCategory *> RegisteredOptionCategories;

    // This collects the different subcommands that have been registered.
    std::set<SubCommand *> RegisteredSubCommands;

    CommandLineParser()
        : ActiveSubCommand(nullptr)
    {
        registerSubCommand(&TopLevelSubCommand);
        registerSubCommand(&AllSubCommands);
    }

    void ResetAllOptionOccurrences();

    bool ParseCommandLineOptions(int argc, const char *const *argv,
                                 const String &Overview);

    void addLiteralOption(Option &Opt, SubCommand *SC, const String &Name)
    {
        if (Opt.hasArgStr())
            return;
        if (!SC->OptionsMap.insert(std::make_pair(Name, &Opt)).second)
        {
            std::cerr << ProgramName << ": CommandLine Error: Option '" << Name << "' registered more than once!\n";
            report_fatal_error("inconsistency in registered CommandLine options");
        }

        // If we're adding this to all sub-commands, add it to the ones that have
        // already been registered.
        if (SC == &AllSubCommands)
        {
            for (const auto &Sub : RegisteredSubCommands)
            {
                if (SC == Sub)
                    continue;
                addLiteralOption(Opt, Sub, Name);
            }
        }
    }

    void addLiteralOption(Option &Opt, const String &Name)
    {
        if (Opt.Subs.empty())
            addLiteralOption(Opt, &TopLevelSubCommand, Name);
        else
        {
            for (auto SC : Opt.Subs)
                addLiteralOption(Opt, SC, Name);
        }
    }

    void addOption(Option *O, SubCommand *SC)
    {
        bool HadErrors = false;
        if (O->hasArgStr())
        {
            // Add argument to the argument map!
            if (!SC->OptionsMap.insert(std::make_pair(O->ArgStr, O)).second)
            {
                std::cerr << ProgramName << ": CommandLine Error: Option '" << O->ArgStr << "' registered more than once!\n";
                HadErrors = true;
            }
        }

        // Remember information about positional options.
        if (O->getFormattingFlag() == cl::Positional)
            SC->PositionalOpts.push_back(O);
        else if (O->getMiscFlags() & cl::Sink) // Remember sink options
            SC->SinkOpts.push_back(O);
        else if (O->getNumOccurrencesFlag() == cl::ConsumeAfter)
        {
            if (SC->ConsumeAfterOpt)
            {
                O->error("Cannot specify more than one option with cl::ConsumeAfter!");
                HadErrors = true;
            }
            SC->ConsumeAfterOpt = O;
        }

        // Fail hard if there were errors. These are strictly unrecoverable and
        // indicate serious issues such as conflicting option names or an
        // incorrectly
        // linked LLVM distribution.
        if (HadErrors)
            report_fatal_error("inconsistency in registered CommandLine options");

        // If we're adding this to all sub-commands, add it to the ones that have
        // already been registered.
        if (SC == &AllSubCommands)
        {
            for (const auto &Sub : RegisteredSubCommands)
            {
                if (SC == Sub)
                    continue;
                addOption(O, Sub);
            }
        }
    }

    void addOption(Option *O)
    {
        if (O->Subs.empty())
        {
            addOption(O, &TopLevelSubCommand);
        }
        else
        {
            for (auto SC : O->Subs)
                addOption(O, SC);
        }
    }

    void removeOption(Option *O, SubCommand *SC)
    {
        Strings OptionNames;
        O->getExtraOptionNames(OptionNames);
        if (O->hasArgStr())
            OptionNames.push_back(O->ArgStr);

        SubCommand &Sub = *SC;
        for (auto Name : OptionNames)
            Sub.OptionsMap.erase(Name);

        if (O->getFormattingFlag() == cl::Positional)
            for (auto Opt = Sub.PositionalOpts.begin();
                 Opt != Sub.PositionalOpts.end(); ++Opt)
            {
                if (*Opt == O)
                {
                    Sub.PositionalOpts.erase(Opt);
                    break;
                }
            }
        else if (O->getMiscFlags() & cl::Sink)
            for (auto Opt = Sub.SinkOpts.begin(); Opt != Sub.SinkOpts.end(); ++Opt)
            {
                if (*Opt == O)
                {
                    Sub.SinkOpts.erase(Opt);
                    break;
                }
            }
        else if (O == Sub.ConsumeAfterOpt)
            Sub.ConsumeAfterOpt = nullptr;
    }

    void removeOption(Option *O)
    {
        if (O->Subs.empty())
            removeOption(O, &TopLevelSubCommand);
        else
        {
            if (O->isInAllSubCommands())
            {
                for (auto SC : RegisteredSubCommands)
                    removeOption(O, SC);
            }
            else
            {
                for (auto SC : O->Subs)
                    removeOption(O, SC);
            }
        }
    }

    bool hasOptions(const SubCommand &Sub) const
    {
        return (!Sub.OptionsMap.empty() || !Sub.PositionalOpts.empty() ||
                nullptr != Sub.ConsumeAfterOpt);
    }

    bool hasOptions() const
    {
        for (const auto &S : RegisteredSubCommands)
        {
            if (hasOptions(*S))
                return true;
        }
        return false;
    }

    SubCommand *getActiveSubCommand()
    {
        return ActiveSubCommand;
    }

    void updateArgStr(Option *O, const String &NewName, SubCommand *SC)
    {
        SubCommand &Sub = *SC;
        if (!Sub.OptionsMap.insert(std::make_pair(NewName, O)).second)
        {
            std::cerr << ProgramName << ": CommandLine Error: Option '" << O->ArgStr
                   << "' registered more than once!\n";
            report_fatal_error("inconsistency in registered CommandLine options");
        }
        Sub.OptionsMap.erase(O->ArgStr);
    }

    void updateArgStr(Option *O, const String &NewName)
    {
        if (O->Subs.empty())
            updateArgStr(O, NewName, &TopLevelSubCommand);
        else
        {
            for (auto SC : O->Subs)
                updateArgStr(O, NewName, SC);
        }
    }

    void printOptionValues();

    void registerCategory(OptionCategory *cat)
    {
        assert(std::count_if(RegisteredOptionCategories.begin(), RegisteredOptionCategories.end(),
                        [cat](const OptionCategory *Category) {
                            return cat->getName() == Category->getName();
                        }) == 0 &&
               "Duplicate option categories");

        RegisteredOptionCategories.insert(cat);
    }

    void registerSubCommand(SubCommand *sub)
    {
        assert(std::count_if(RegisteredOptionCategories.begin(), RegisteredOptionCategories.end(),
                        [sub](const SubCommand *Sub) {
                            return (!sub->getName().empty()) &&
                                   (Sub->getName() == sub->getName());
                        }) == 0 &&
               "Duplicate subcommands");
        RegisteredSubCommands.insert(sub);

        // For all options that have been registered for all subcommands, add the
        // option to this subcommand now.
        if (sub != &AllSubCommands)
        {
            for (auto &E : AllSubCommands.OptionsMap)
            {
                Option *O = E.second;
                if ((O->isPositional() || O->isSink() || O->isConsumeAfter()) ||
                    O->hasArgStr())
                    addOption(O, sub);
                else
                    addLiteralOption(*O, sub, E.first());
            }
        }
    }

    void unregisterSubCommand(SubCommand *sub)
    {
        RegisteredSubCommands.erase(sub);
    }

    /*iterator_range<typename SmallPtrSet<SubCommand *, 4>::iterator>
    getRegisteredSubcommands()
    {
        return make_range(RegisteredSubCommands.begin(),
                          RegisteredSubCommands.end());
    }*/

    void reset()
    {
        ActiveSubCommand = nullptr;
        ProgramName.clear();
        ProgramOverview = {};

        MoreHelp.clear();
        RegisteredOptionCategories.clear();

        ResetAllOptionOccurrences();
        RegisteredSubCommands.clear();

        TopLevelSubCommand.reset();
        AllSubCommands.reset();
        registerSubCommand(&TopLevelSubCommand);
        registerSubCommand(&AllSubCommands);
    }

private:
    SubCommand *ActiveSubCommand;

    Option *LookupOption(SubCommand &Sub, const String &Arg, const String &Value);
    SubCommand *LookupSubCommand(const String &Name);
};

} // namespace

static CommandLineParser GlobalParser;

using Args = std::list<std::string>;

////////////////////////////////////////////////////////////////////////////////

void Option::setArgStr(const String &S) {
    /*if (FullyInitialized)
    GlobalParser->updateArgStr(this, S);*/
    assert((S.empty() || S[0] != '-') && "Option can't start with '-");
    ArgStr = S;
}

bool Option::error(const String &Message, String ArgName, std::ostream &Errs)
{
    if (!ArgName.data())
        ArgName = ArgStr;
    if (ArgName.empty())
        Errs << HelpStr; // Be nice for positional arguments
    else
        Errs << GlobalParser.ProgramName << ": for the -" << ArgName;

    Errs << " option: " << Message << "\n";
    return true;
}

bool Option::isInAllSubCommands() const
{
    return std::any_of(Subs.begin(), Subs.end(), [](const SubCommand *SC) {
        return SC == &AllSubCommands;
    });
}

////////////////////////////////////////////////////////////////////////////////

void parseCommandLineOptions1(const String &prog, Args args, const String &overview)
{
    // TODO: convert args to utf8

    // expand response files and settings
    bool found;
    do
    {
        found = false;
        for (auto i = args.begin(); i != args.end(); i++)
        {
            auto &a = *i;
            if (a.size() > 1 && a[0] == '@' && a[1] != '@')
            {
                // response file
                auto f = read_file(a.substr(1));
                boost::replace_all(f, "\r", "");

                std::vector<String> lines;
                boost::split(lines, f, boost::is_any_of("\n"));
                // do we need trim?
                //for (auto &l : lines)
                    //boost::trim(l);

                for (auto &l : lines)
                    args.insert(i, l);
                i = args.erase(i);
                i--;
                found = true;
            }
        }
    }
    while (found);

    // read settings
    for (auto i = args.begin(); i != args.end(); i++)
    {
        auto &a = *i;
        if (a.size() > 1 && a[0] == '@' && a[1] == '@')
        {
            // settings file
            getSettings().getLocalSettings().load(a.substr(2));
            i = args.erase(i);
            i--;
        }
    }
}

void parseCommandLineOptions(int argc, char **argv, const String &overview)
{
    std::list<std::string> args;
    for (int i = 1; i < argc; i++)
        args.push_back(argv[i]);
    parseCommandLineOptions1(argv[0], args, overview);
}

void parseCommandLineOptions(const Strings &args, const String &overview)
{
    parseCommandLineOptions1(*args.begin() , { args.begin() + 1, args.end() }, overview);
}

}

SettingPath::SettingPath()
{
}

SettingPath::SettingPath(const String &s)
{
    parse(s);
}

SettingPath &SettingPath::operator=(const String &s)
{
    return *this = SettingPath(s);
}

void SettingPath::parse(const String &s)
{
    if (s.empty())
    {
        parts.clear();
        return;
    }

    PathParserDriver d;
    auto r = d.parse(s);
    if (!d.bb.error_msg.empty())
        throw std::runtime_error("While parsing setting path '" + s + "':" + d.bb.error_msg);
    parts = d.sp;
}

bool SettingPath::isBasic(const String &part)
{
    for (auto &c : part)
    {
        if (!(c >= 0 && c <= 127 && (isalnum(c) || c == '_' || c == '-')))
            return false;
    }
    return true;
}

String SettingPath::toString() const
{
    String s;
    for (auto &p : parts)
    {
        if (isBasic(p))
            s += p;
        else
            s += "\"" + detail::escapeSettingPart(p) + "\"";
        s += ".";
    }
    if (!s.empty())
        s.resize(s.size() - 1);
    return s;
}

SettingPath SettingPath::operator/(const SettingPath &rhs) const
{
    SettingPath tmp(*this);
    tmp.parts.insert(tmp.parts.end(), rhs.parts.begin(), rhs.parts.end());
    return tmp;
}

SettingPath SettingPath::fromRawString(const String &s)
{
    SettingPath p;
    p.parts.push_back(s);
    return p;
}

void Settings::clear()
{
    settings.clear();
}

void Settings::load(const path &fn, SettingsType type)
{
    if (fn.extension() == ".yml" || fn.extension() == ".yaml" || fn.extension() == ".settings")
    {
        load(YAML::LoadFile(fn.u8string()));
    }
    else
    {
        std::unique_ptr<FILE, decltype(&fclose)> f(primitives::filesystem::fopen_checked(fn), fclose);

        SettingsParserDriver d;
        auto r = d.parse(f.get());
        if (!d.bb.error_msg.empty())
            throw std::runtime_error(d.bb.error_msg);
        settings = d.sm;
    }
}

void Settings::load(const String &s, SettingsType type)
{
    SettingsParserDriver d;
    auto r = d.parse(s);
    if (!d.bb.error_msg.empty())
        throw std::runtime_error(d.bb.error_msg);
    settings = d.sm;
}

void Settings::load(const yaml &root, const SettingPath &prefix)
{
    /*if (root.IsNull() || root.IsScalar())
        return;*/
    if (!root.IsMap())
        return;
        //throw std::runtime_error("Config must be a map");

    for (auto kv : root)
    {
        if (kv.second.IsScalar())
            operator[](prefix / SettingPath::fromRawString(kv.first.as<String>())).representation = kv.second.as<String>();
        else if (kv.second.IsMap())
            load(kv.second, SettingPath::fromRawString(kv.first.as<String>()));
        else
            throw std::runtime_error("sequences are not supported");
    }
}

String Setting::toString() const
{
    if (!representation.empty())
        return representation;
    return parser->toString(value);
}

bool Settings::hasSaveableItems() const
{
    int n_saveable = 0;
    for (auto &[k, v] : settings)
    {
        if (v.saveable)
            n_saveable++;
    }
    return n_saveable != 0;
}

void Settings::save(const path &fn) const
{
    if (!hasSaveableItems())
        return;

    if (!fn.empty())
    {
        if (fn.extension() == ".yml" || fn.extension() == ".yaml" || fn.extension() == ".settings")
        {
            yaml root;
            dump(root);
            write_file(fn, YAML::Dump(root));
        }
        else
        {
            write_file(fn, dump());
        }
    }
}

String Settings::dump() const
{
    String s;
    throw std::runtime_error("not implemented");
    return s;
}

void Settings::dump(yaml &root) const
{
    for (auto &[k, v] : settings)
    {
        if (v.saveable)
            root[k.toString()] = v.toString();
    }
}

Setting &Settings::operator[](const String &k)
{
    return settings[prefix / k];
}

const Setting &Settings::operator[](const String &k) const
{
    auto i = settings.find(prefix / k);
    if (i == settings.end())
        throw std::runtime_error("No such setting: " + k);
    return i->second;
}

Setting &Settings::operator[](const SettingPath &k)
{
    return settings[prefix / k];
}

const Setting &Settings::operator[](const SettingPath &k) const
{
    auto i = settings.find(prefix / k);
    if (i == settings.end())
        throw std::runtime_error("No such setting: " + k.toString());
    return i->second;
}

template <class T>
SettingStorage<T>::SettingStorage()
{
    // to overlive setting storage
    detail::getParserStorage();
}

template <class T>
T &SettingStorage<T>::getSystemSettings()
{
    return get(SettingsType::System);
}

template <class T>
T &SettingStorage<T>::getUserSettings()
{
    return get(SettingsType::User);
}

template <class T>
T &SettingStorage<T>::getLocalSettings()
{
    return get(SettingsType::Local);
}

template <class T>
void SettingStorage<T>::clearLocalSettings()
{
    getLocalSettings() = getUserSettings();
}

template <class T>
T &SettingStorage<T>::get(SettingsType type)
{
    if (type < SettingsType::Local || type >= SettingsType::Max)
        throw std::runtime_error("No such settings storage");

    auto &s = settings[toIndex(type)];
    switch (type)
    {
    case SettingsType::Local:
    {
        RUN_ONCE
        {
            s = get(SettingsType::User);
        }
    }
    break;
    case SettingsType::User:
    {
        RUN_ONCE
        {
            s = get(SettingsType::System);

            auto fn = userConfigFilename;
            if (!fn.empty())
            {
                if (!fs::exists(fn))
                {
                    auto ss = get(SettingsType::System);
                    if (ss.hasSaveableItems())
                    {
                        error_code ec;
                        fs::create_directories(fn.parent_path(), ec);
                        if (ec)
                            throw std::runtime_error(ec.message());
                        ss.save(fn);
                    }
                }
                else
                    s.load(fn, SettingsType::User);
            }
        }
    }
    break;
    case SettingsType::System:
    {
        RUN_ONCE
        {
            auto fn = systemConfigFilename;
            if (fs::exists(fn))
                s.load(fn, SettingsType::System);
        }
    }
    break;
    default:
        break;
    }
    return s;
}

template struct SettingStorage<primitives::Settings>;

primitives::SettingStorage<primitives::Settings> &getSettings(primitives::SettingStorage<primitives::Settings> *v)
{
    static StaticValueOrRef<primitives::SettingStorage<primitives::Settings>> settings(v);
    return settings;
}

primitives::Settings &getSettings(SettingsType type, primitives::SettingStorage<primitives::Settings> *v)
{
    return getSettings(v).get(type);
}

}
