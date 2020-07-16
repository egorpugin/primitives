#pragma once

#include <boost/algorithm/string.hpp>

#include <primitives/emitter.h>
#include <primitives/filesystem.h>
#include <primitives/main.h>
#include <primitives/yaml.h>

#include <iostream>
#include <optional>

struct EmitterContext
{
    primitives::CppEmitter h;
    primitives::CppEmitter c;
    primitives::CppEmitter qt;
};

struct Settings
{
    bool add_include = true;
    bool add_qt = true;
    String namespace_;
    String prefix;
    String api_name;
    bool generate_struct = false;
};

struct Category
{
    String name;
    String description;
};

using Categories = std::map<String, Category>;

struct CommandLine;

struct Command
{
    const CommandLine *owner = nullptr;

    String name;
    String option; // rename to flag?
    std::optional<String> default_value; // rename to init?
    String type = "bool";
    // if true, extern variable will be declared
    bool external = false;
    // if external == true, it will point to it
    // if external == false, in-class variable will be defined
    String location;
    String category;
    String description;
    String value_description;
    Strings aliases;
    Strings subcommands;
    bool all_subcommands = false;

    bool hidden = false;
    bool really_hidden = false;

    int multi_val = -1;

    bool value_optional = false;
    bool list = false;
    bool positional = false;
    bool zero_or_more = false;
    bool comma_separated = false;
    bool space_separated = false;
    bool required = false;
    bool consume_after = false;
    bool prefix = false; // some llvm thing - prefix each value

    /*Command(const CommandLine &owner);
    Command(const Command &) = default;
    Command &operator=(const Command &) = default;*/

    void parse(const yaml &root);
    bool isExternalOrLocation() const;
    bool isLocationOnly() const;
    String getExternalName() const;
    String getName(const Settings &s) const;
    String getClName(const Settings &s) const;
    String getStructType() const;
    void emitExternal(primitives::CppEmitter &ctx) const;
    void emitQtWidgets(primitives::CppEmitter &ctx, const Settings &settings,
        const String &qt_var_parent, const String &parent = {}) const;
    void emitOption(EmitterContext &inctx, const Settings &settings) const;
};

using Commands = std::vector<Command>;

struct CommandLine
{
    String name;
    String description;
    Commands commands;
    CommandLine *parent = nullptr;
    std::vector<std::unique_ptr<CommandLine>> subcommands;

    CommandLine(const String &name);

    void parse(const yaml &root, const String &subcommand = {});
    void emitOption(EmitterContext &ctx, const Settings &settings) const;
    void emitStruct(EmitterContext &ectx, const Settings &settings, const String &parent = {}) const;
    void emitExternal(primitives::CppEmitter &ctx) const;
    void emitQtWidgets(primitives::CppEmitter &ctx, const Settings &settings, const String &qt_var_parent) const;
    String getVariableName() const;
    String getSubCommandVariableName() const;

    using StorageCommands = std::map<String, Command>;
    StorageCommands gatherStorageCommands() const;

    bool isRoot() const { return parent == nullptr; }

private:
    String get_suffix() const { return isRoot() ? "" : ("_" + name); }
    String get_Oname() const { return "Options" + get_suffix(); }
    String get_oname() const { return "options" + get_suffix(); }
};

struct File
{
    CommandLine cmd;
    Settings settings;
    Categories categories;

    File();

    void parse(const yaml &root);
    void emit(const path &h, const path &cpp) const;
};
