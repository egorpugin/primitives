#include <boost/algorithm/string.hpp>

#include <primitives/emitter.h>
#include <primitives/filesystem.h>
#include <primitives/main.h>
#include <primitives/yaml.h>

#include <iostream>

struct EmitterContext
{
    primitives::CppEmitter h;
    primitives::CppEmitter c;
};

struct Settings
{
    bool add_include = true;
    String namespace_;
    String prefix;
    bool generate_struct = false;
};

struct SubCommand
{

};

struct Command
{
    String name;
    String option; // rename to flag?
    String default_value; // rename to init?
    String type = "bool";
    String location;
    bool external = false;
    String description;
    String value_description;
    Strings aliases;
    String subcommand;

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

    void parse(const yaml &root)
    {
#define READ_OPT(x) \
        if (root[#x].IsDefined()) \
        x = root[#x].template as<decltype(x)>()
#define READ_SEQ(x) \
        if (root[#x].IsDefined()) \
        x = get_sequence(root[#x])

        READ_OPT(option);
        READ_OPT(type);
        READ_OPT(default_value);
        READ_OPT(location);
        READ_OPT(external);
        if (root["desc"].IsDefined())
            description = root["desc"].template as<decltype(description)>();
        READ_OPT(description);
        if (root["value_desc"].IsDefined())
            value_description = root["value_desc"].template as<decltype(value_description)>();
        READ_OPT(value_description);
        READ_SEQ(aliases);

        // bools
        READ_OPT(list);
        READ_OPT(positional);
        READ_OPT(hidden);
        READ_OPT(really_hidden);
        READ_OPT(zero_or_more);
        READ_OPT(prefix);
        READ_OPT(comma_separated);
        READ_OPT(space_separated);
        READ_OPT(required);
        READ_OPT(consume_after);
        READ_OPT(value_optional);

        //
        READ_OPT(multi_val);

#undef READ_OPT
#undef READ_SEQ

        if (option.empty())
            option = boost::replace_all_copy(name, "_", "-");
    }

    bool isExternal() const
    {
        return external || !location.empty();
    }

    String getExternalName() const
    {
        return location.empty() ? name : location;
    }

    String getName(const Settings &s) const
    {
        if (!s.prefix.empty())
            return s.prefix + name;
        return name;
    }

    String getStructType() const
    {
        if (list)
            return "::std::vector<" + type + ">";
        return type;
    }

    void emitExternal(primitives::CppEmitter &ctx) const
    {
        if (!isExternal())
            return;
        if (list)
            return;
        ctx.addLine("extern " + type + " " + getExternalName() + ";");
    }

    void emit(EmitterContext &inctx, const Settings &settings) const
    {
        auto begin = [this, &settings](auto &ctx)
        {
            ctx.addText("::cl::");
            ctx.addText(list ? "list" : "opt");
            ctx.addText("<" + type);
            if (isExternal())
            {
                if (list)
                    ctx.addText(", decltype(::" + getExternalName() + ")");
                else
                    ctx.addText(", true");
            }
            ctx.addText("> ");
            ctx.addText(getName(settings));
        };

        inctx.h.addText("extern ");
        begin(inctx.h);
        inctx.h.addText(";");
        inctx.h.addLine();

        inctx.c.addLine("// " + name);
        inctx.c.addLine();
        begin(inctx.c);

        auto &ctx = inctx.c;
        ctx.addText("(");
        if (!positional)
            ctx.addText("\"" + option + "\"");
        else
            ctx.addText("::cl::Positional");

        auto sz = ctx.getLines().size();
        ctx.increaseIndent();
        if (!description.empty())
            ctx.addLineWithoutIndent(", ::cl::desc(R\"xxx(" + description + ")xxx\")");
        if (!value_description.empty())
            ctx.addLine(", ::cl::value_desc(\"" + value_description + "\")");
        // location before default value
        if (isExternal())
            ctx.addLine(", ::cl::location(::" + getExternalName() + ")");
        if (!default_value.empty())
            ctx.addLine(", ::cl::init(" + default_value + ")");
        if (zero_or_more)
            ctx.addLine(", ::cl::ZeroOrMore");
        if (comma_separated)
            ctx.addLine(", ::cl::CommaSeparated");
        if (space_separated)
            ctx.addLine(", ::cl::SpaceSeparated");
        if (required)
            ctx.addLine(", ::cl::Required");
        if (consume_after)
            ctx.addLine(", ::cl::ConsumeAfter");
        if (prefix)
            ctx.addLine(", ::cl::Prefix");
        if (value_optional)
            ctx.addLine(", ::cl::ValueOptional");
        if (multi_val != -1)
            ctx.addLine(", ::cl::multi_val(" + std::to_string(multi_val) + ")");
        if (!subcommand.empty())
            ctx.addLine(", ::cl::sub(subcommand_" + subcommand + ")");
        if (hidden)
            ctx.addLine(", ::cl::Hidden");
        if (really_hidden)
            ctx.addLine(", ::cl::ReallyHidden");
        ctx.decreaseIndent();

        if (sz == ctx.getLines().size())
            ctx.addText(");");
        else
            ctx.addLine(");");

        int na = 1;
        for (auto &a : aliases)
        {
            ctx.addLine("static ::cl::alias " + name + std::to_string(na++) + "(\"" + a + "\"");
            ctx.increaseIndent();
            ctx.addLine(", ::cl::desc(\"Alias for -" + option + "\")");
            ctx.addLine(", ::cl::aliasopt(");
            ctx.addText(getName(settings) + ")");
            ctx.decreaseIndent();
            ctx.addLine(");");
        }
        ctx.addLine();
    }
};

using Commands = std::vector<Command>;

struct CommandLine
{
    String name;
    String description;
    Commands commands;
    std::vector<std::unique_ptr<CommandLine>> subcommands;

    void parse(const yaml &root, const String &subcommand = {})
    {
        auto cmd = root["command_line"];
        for (const auto &v : cmd)
        {
            auto n = v.first.as<String>();
            if (n == "subcommand")
            {
                subcommands.emplace_back(std::make_unique<CommandLine>());
                subcommands.back()->name = v.second["name"].as<String>();
                if (v.second["desc"].IsDefined())
                    subcommands.back()->description = v.second["desc"].as<String>();
                if (v.second["description"].IsDefined())
                    subcommands.back()->description = v.second["description"].as<String>();
                subcommands.back()->parse(v.second, subcommands.back()->name);
                continue;
            }
            Command c;
            c.name = n;
            c.subcommand = subcommand;
            c.parse(v.second);
            commands.push_back(c);
        }
    }

    void emit(EmitterContext &ctx, const Settings &settings) const
    {
        if (!name.empty())
        {
            ctx.h.addLine("extern ::cl::SubCommand subcommand_" + name + ";");
            ctx.h.emptyLines();

            ctx.c.addLine("::cl::SubCommand subcommand_" + name + "(\"" + name + "\", \"" + description + "\");");
            ctx.c.emptyLines();
        }

        for (auto &c : commands)
            c.emit(ctx, settings);
        ctx.h.emptyLines();
        ctx.c.emptyLines();
        for (auto &sb : subcommands)
            sb->emit(ctx, settings);
    }

    void emitStruct(primitives::CppEmitter &ctx, const Settings &settings) const
    {
        const auto suffix = name.empty() ? "" : ("_" + name);
        const auto Oname = "Options" + suffix;
        const auto oname = "options" + suffix;

        ctx.addLine("//");
        ctx.beginBlock("struct " + Oname);
        for (auto &c : commands)
        {
            if (c.isExternal())
                continue;
            ctx.addLine(c.getStructType() + " " + c.name + ";");
        }
        ctx.emptyLines();

        // subcommands
        for (auto &sb : subcommands)
            sb->emitStruct(ctx, settings);
        ctx.emptyLines();

        // ctor
        ctx.beginFunction(Oname + "()");
        for (auto &c : commands)
        {
            if (c.isExternal())
                continue;
            //                                                  .getValue()?
            ctx.addLine(c.name + " = " + c.getName(settings) + ";");
        }
        ctx.endFunction();

        // end of s
        ctx.endBlock(true);
        ctx.addLine();

        // var
        if (!name.empty())
        {
            ctx.addLine(Oname + " " + oname + ";");
            ctx.addLine();
        }
    }

    void emitExternal(primitives::CppEmitter &ctx) const
    {
        for (auto &c : commands)
            c.emitExternal(ctx);
        ctx.emptyLines();
        for (auto &sb : subcommands)
            sb->emitExternal(ctx);
    }
};

struct File
{
    CommandLine cmd;
    Settings settings;

    void parse(const yaml &root)
    {
        auto set = root["settings"];
        if (set)
        {
            if (set["prefix"].IsDefined())
                settings.prefix = set["prefix"].template as<decltype(settings.prefix)>();
            if (set["namespace"].IsDefined())
                settings.namespace_ = set["namespace"].template as<decltype(settings.namespace_)>();
            if (set["namespace"].IsDefined())
                settings.namespace_ = set["namespace"].template as<decltype(settings.namespace_)>();
            if (set["generate_struct"].IsDefined())
                settings.generate_struct = set["generate_struct"].template as<decltype(settings.generate_struct)>();
        }

        cmd.parse(root);
    }

    void emit(const path &h, const path &cpp) const
    {
        EmitterContext ctx;

        if (settings.add_include)
        {
            ctx.h.addLine("#include <primitives/sw/cl.h>");
            ctx.h.addLine();
        }

        ctx.c.addLine("#include \"" + h.filename().u8string() + "\"");
        ctx.c.addLine();

        // externals
        cmd.emitExternal(ctx.h);

        if (!settings.namespace_.empty())
        {
            ctx.h.beginNamespace(settings.namespace_);
            ctx.c.beginNamespace(settings.namespace_);
        }
        cmd.emit(ctx, settings);
        if (settings.generate_struct)
            cmd.emitStruct(ctx.h, settings);
        if (!settings.namespace_.empty())
        {
            ctx.h.endNamespace();
            ctx.c.endNamespace();
        }

        write_file(h, ctx.h.getText());
        write_file(cpp, ctx.c.getText());
    }
};

int main(int argc, char **argv)
{
    if (argc != 5)
        throw SW_RUNTIME_ERROR("Usage: PGM in.yml out.h out.cpp type");

    path in = argv[1];
    path outh = argv[2];
    path outcpp = argv[3];
    String type = argv[4];

    if (type != "llvm")
        throw SW_RUNTIME_ERROR("Unknown type: " + type);

    auto root = YAML::Load(read_file(argv[1]));

    File f;
    f.parse(root);
    f.emit(outh, outcpp);

    return 0;
}
