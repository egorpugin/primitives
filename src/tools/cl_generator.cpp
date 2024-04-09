#include "cl_generator.h"

static const String all_subcommands_name = "AllSubCommands";
static const String subcommand_prefix = "subcommand_";

/*Command::Command(const CommandLine &owner)
    : owner(&owner)
{
}*/

void Command::parse(const yaml &root)
{
#define READ_OPT(x)                                                                                                    \
    if (root[#x].IsDefined())                                                                                          \
    x = root[#x].template as<decltype(x)>()
#define READ_SEQ(x)                                                                                                    \
    if (root[#x].IsDefined())                                                                                          \
    {                                                                                                                  \
        auto s = get_sequence(root[#x]);                                                                               \
        x.insert(x.end(), s.begin(), s.end());                                                                         \
    }

    READ_OPT(varname);
    READ_OPT(option);
    READ_OPT(type);
    {
        if (root["default_value"].IsDefined())
            default_value = root["default_value"].template as<String>();
        if (!default_value && root["default"].IsDefined())
            default_value = root["default"].template as<String>();
    }
    READ_OPT(location);
    READ_OPT(external);
    if (root["desc"].IsDefined())
        description = root["desc"].template as<decltype(description)>();
    READ_OPT(description);
    if (root["cat"].IsDefined())
        category = root["cat"].template as<decltype(category)>();
    READ_OPT(category);
    if (root["value_desc"].IsDefined())
        value_description = root["value_desc"].template as<decltype(value_description)>();
    READ_OPT(value_description);
    READ_SEQ(aliases);
    READ_SEQ(subcommands);
    READ_OPT(all_subcommands);

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

bool Command::isExternalOrLocation() const
{
    return external || !location.empty();
}

bool Command::isLocationOnly() const
{
    return !external && !location.empty();
}

String Command::getExternalName() const
{
    return location.empty() ? (varname.empty() ? name : varname) : location;
}

String Command::getName(const Settings &s) const
{
    //if (!s.prefix.empty())
        //return s.prefix + name;
    return varname.empty() ? name : varname;
}

String Command::getClName(const Settings &s) const
{
    auto n = owner->getSubCommandVariableName();
    if (!n.empty())
        n += "_";
    return n + getName(s);
}

String Command::getStructType() const
{
    if (list)
        return "::std::vector<" + type + ">";
    return type;
}

void Command::emitExternal(primitives::CppEmitter &ctx) const
{
    if (!external)
        return;
    ctx.addLine("extern ");
    if (list)
        ctx.addText("std::vector<");
    ctx.addText(type);
    if (list)
        ctx.addText(">");
    ctx.addText(" " + getExternalName() + ";");
}

void Command::emitQtWidgets(primitives::CppEmitter &ctx, const Settings &settings,
    const String &qt_var_parent, const String &parent) const
{
    ctx.addLine("cl_option_add_widget(\"" + name + "\", " + qt_var_parent + ", ");
    if (!isExternalOrLocation())
    {
        ctx.addText("options.");
        if (!owner->isRoot())
            ctx.addText("options_" + parent + ".");
        ctx.addText(getExternalName() + ", options.getClOptions().");
    }
    else
    {
        if (isLocationOnly())
            ctx.addText("options.getClOptions().getStorage()." + getExternalName() + ", options.getClOptions().");
        else
            ctx.addText(getExternalName() + ", options.getClOptions().");
    }
    ctx.addText(getClName(settings));
    ctx.addText(");");
}

void Command::emitOption(EmitterContext &inctx, const Settings &settings) const
{
    auto begin = [this, &settings](auto &ctx)
    {
        ctx.addText("::cl::");
        ctx.addText(list ? "list" : "opt");
        ctx.addText("<" + type);
        if (isExternalOrLocation())
        {
            if (list)
            {
                if (isLocationOnly())
                    ctx.addText(", decltype(Externals::" + getExternalName() + ")");
                else
                    ctx.addText(", decltype(::" + getExternalName() + ")");
            }
            else
                ctx.addText(", true");
        }
        ctx.addText("> ");
        ctx.addText(getClName(settings));
    };

    inctx.h.addLine();
    begin(inctx.h);
    inctx.h.addText(";");

    auto &ctx = inctx.c;
    ctx.addLine(getClName(settings));
    ctx.addText("(");
    if (!positional)
        ctx.addText("\"" + option + "\"");
    else
        ctx.addText("::cl::Positional");

    auto sz = ctx.getLines().size();
    ctx.increaseIndent();
    if (!description.empty())
        ctx.addLine(", ::cl::desc(R\"xxx(" + description + ")xxx\")");
    if (!category.empty())
        ctx.addLine(", ::cl::cat(cat_" + category + ")");
    if (!value_description.empty())
        ctx.addLine(", ::cl::value_desc(\"" + value_description + "\")");
    // location before default value
    if (isExternalOrLocation())
    {
        if (isLocationOnly())
            ctx.addLine(", ::cl::location(getStorage()." + getExternalName() + ")");
        else
            ctx.addLine(", ::cl::location(::" + getExternalName() + ")");
    }
    if (default_value)
        ctx.addLine(", ::cl::init(" + *default_value + ")");
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
    // TopLevelSubCommand - default
    // AllSubCommands - all subs
    if (all_subcommands)
        ctx.addLine(", ::cl::sub(::cl::get" + all_subcommands_name + "())");
    for (auto &s : subcommands)
        ctx.addLine(", ::cl::sub(" + subcommand_prefix + s + ")");
    if (hidden)
        ctx.addLine(", ::cl::Hidden");
    if (really_hidden)
        ctx.addLine(", ::cl::ReallyHidden");
    ctx.decreaseIndent();

    if (sz == ctx.getLines().size())
        ctx.addText("),");
    else
        ctx.addLine("),");

    int na = 1;
    for (auto &a : aliases)
    {
        inctx.h.addLine("::cl::alias " + name + std::to_string(na) + ";");
        ctx.addLine(name + std::to_string(na) + "(\"" + a + "\"");
        ctx.increaseIndent();
        ctx.addLine(", ::cl::desc(\"Alias for -" + option + "\")");
        ctx.addLine(", ::cl::aliasopt(");
        ctx.addText(getClName(settings) + ")");
        ctx.decreaseIndent();
        ctx.addLine("),");
        na++;
    }
}

CommandLine::CommandLine(const String &name)
    : name(name)
{
}

void CommandLine::parse(const yaml &root, const String &subcommand)
{
    auto cmd = root["command_line"];
    for (const auto &v : cmd)
    {
        auto n = v.first.as<String>();
        n = boost::replace_all_copy(n, "-", "_");
        if (n == "subcommand")
        {
            if (v.second["name"].as<String>() == all_subcommands_name)
            {
                // do not add subcommand here
                parse(v.second, v.second["name"].as<String>());
                continue;
            }

            auto name = v.second["name"].as<String>();
            subcommands.emplace_back(std::make_unique<CommandLine>(name));
            subcommands.back()->parent = this;
            if (v.second["desc"].IsDefined())
                subcommands.back()->description = v.second["desc"].as<String>();
            if (v.second["description"].IsDefined())
                subcommands.back()->description = v.second["description"].as<String>();
            subcommands.back()->parse(v.second, subcommands.back()->name);
            continue;
        }
        Command c;
        c.owner = this;
        c.name = n;
        c.varname = c.name;
        if (!subcommand.empty())
        {
            if (subcommand == all_subcommands_name)
                c.all_subcommands = true;
            else
                c.subcommands.push_back(subcommand);
        }
        c.parse(v.second);
        commands.push_back(c);
    }
}

void CommandLine::emitOption(EmitterContext &ctx, const Settings &settings) const
{
    if (parent)
    {
        ctx.h.addLine("::primitives::cl::ClSubCommand " + getSubCommandVariableName() + ";");
        ctx.c.addLine(getSubCommandVariableName() + "(\"" + name + "\", R\"xxx(" + description + ")xxx\"),");
    }

    for (auto &c : commands)
        c.emitOption(ctx, settings);
    ctx.h.emptyLines();
    ctx.qt.emptyLines();
    for (auto &sb : subcommands)
        sb->emitOption(ctx, settings);
}

void CommandLine::emitStruct(EmitterContext &ectx, const Settings &settings, const String &parent) const
{
    auto Oname = get_Oname();

    auto &ctx = ectx.h;
    ctx.addLine("//");
    ctx.beginBlock("struct " + settings.api_name + " " + Oname);
    if (isRoot())
    {
        ctx.addLine("ClOptions &cl_options__;");
        ctx.addLine("ClOptions &getClOptions() { return cl_options__; }");
        ctx.addLine("const ClOptions &getClOptions() const { return cl_options__; }");
        ctx.addLine();
    }
    for (auto &c : commands)
    {
        if (c.isExternalOrLocation())
            continue;
        ctx.addLine(c.getStructType() + " " + c.varname + ";");
    }
    ctx.emptyLines();

    // subcommands
    for (auto &sb : subcommands)
        sb->emitStruct(ectx, settings, Oname);
    ctx.emptyLines();

    // ctor
    ctx.addLine(Oname + "(ClOptions &);");
    ectx.c.addLine(parent + "::" + Oname + "::" + Oname + "(ClOptions &cl_options)");
    ectx.c.increaseIndent();
    ectx.c.addLine(":");
    if (isRoot())
    {
        ectx.c.addLine("cl_options__(cl_options),");
        ectx.c.addLine();
    }
    for (auto &c : commands)
    {
        if (c.isExternalOrLocation())
            continue;
        //                                                  .getValue()?
        ectx.c.addLine(c.varname + "(cl_options." + c.getClName(settings) + "),");
    }
    if (!subcommands.empty())
        ectx.c.addLine();
    for (auto &sb : subcommands)
        ectx.c.addLine(sb->get_oname() + "(cl_options),");
    ectx.c.trimEnd(1);
    ectx.c.decreaseIndent();
    ectx.c.beginBlock();
    ectx.c.endBlock();
    ectx.c.emptyLines();

    // end of s
    ctx.endBlock(true);
    ctx.addLine();

    // var
    if (!isRoot())
    {
        ctx.addLine(Oname + " " + get_oname() + ";");
        ctx.addLine();
    }
}

void CommandLine::emitExternal(primitives::CppEmitter &ctx) const
{
    for (auto &c : commands)
        c.emitExternal(ctx);
    ctx.emptyLines();
    for (auto &sb : subcommands)
        sb->emitExternal(ctx);
    ctx.emptyLines();
}

void CommandLine::emitQtWidgets(primitives::CppEmitter &ctx, const Settings &settings, const String &qt_var_parent) const
{
    const auto n = name.empty() ? all_subcommands_name : ("SubCommand " + name);
    const String var = "gbl";

    ctx.addLine("// subcommand "s + n);
    ctx.beginBlock();
    ctx.addLine("auto gb = new QGroupBox(\"" + n + "\");");
    ctx.addLine(qt_var_parent + "->addWidget(gb);");
    ctx.addLine("QVBoxLayout *" + var + " = new QVBoxLayout;");
    ctx.addLine("gb->setLayout(" + var + ");");
    ctx.emptyLines();

    for (auto &c : commands)
        c.emitQtWidgets(ctx, settings, var, name);
    ctx.emptyLines();
    for (auto &sb : subcommands)
        sb->emitQtWidgets(ctx, settings, var);
    ctx.endBlock();
}

String CommandLine::getVariableName() const { return name; }
String CommandLine::getSubCommandVariableName() const { return isRoot() ? "" : (subcommand_prefix + name); }

CommandLine::StorageCommands CommandLine::gatherStorageCommands() const
{
    StorageCommands cmds;
    for (auto &c : commands)
    {
        if (c.isLocationOnly())
            cmds[c.location] = c;
    }
    for (auto &s : subcommands)
        cmds.merge(s->gatherStorageCommands());
    return cmds;
}

File::File()
    : cmd("global")
{
}

void File::parse(const yaml &root)
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
        if (set["api_name"].IsDefined())
            settings.api_name = set["api_name"].template as<decltype(settings.api_name)>();
    }

    auto cats = root["categories"];
    if (cats)
    {
        for (const auto &c : cats)
        {
            auto key = c.first.template as<String>();
            Category v;
            v.name = c.second["name"].template as<String>();
            if (c.second["desc"].IsDefined())
                v.description = c.second["desc"].template as<String>();
            if (c.second["description"].IsDefined())
                v.description = c.second["description"].template as<String>();
            categories[key] = v;
        }
    }

    cmd.parse(root);
}

void File::emit(const path &h, const path &cpp) const
{
    EmitterContext ctx;

    // h
    ctx.h.addLine("#include <primitives/sw/cl.h>");
    ctx.h.addLine();

    // cpp
    ctx.c.addLine("#include \"" + to_printable_string(h.filename()) + "\"");
    ctx.c.addLine();

    // externals
    cmd.emitExternal(ctx.h);

    if (!settings.namespace_.empty())
    {
        ctx.h.beginNamespace(settings.namespace_);
        ctx.c.beginNamespace(settings.namespace_);
    }

    {
        ctx.h.beginBlock("struct " + settings.api_name + " ClOptions : ::primitives::cl::ClOptions");

        {
            auto &ctx1 = ctx.h;
            auto &ctx = ctx1;
            ctx.beginBlock("struct Externals");
            for (auto &[n,c] : cmd.gatherStorageCommands())
                ctx.addLine(c.getStructType() + " " + c.name + ";");
            ctx.endBlock(true);
            ctx.addLine("Externals storage__;");
            ctx.addLine("auto &getStorage() { return storage__; }");
            ctx.addLine("const auto &getStorage() const { return storage__; }");
            ctx.addLine();
        }

        ctx.c.addLine("ClOptions::ClOptions()");
        ctx.c.increaseIndent();
        ctx.c.addLine(":");

        for (auto &[n, c] : categories)
        {
            ctx.h.addLine("::cl::OptionCategory cat_" + n + ";");
            ctx.c.addLine("cat_" + n + "(\"" + c.name + "\", R\"xxx(" + c.description + ")xxx\"),");
        }
        if (!categories.empty())
        {
            ctx.h.emptyLines();
            ctx.c.emptyLines();
        }

        cmd.emitOption(ctx, settings);

        ctx.h.addLine("ClOptions();");
        ctx.h.addLine("~ClOptions();");
        ctx.h.endBlock(true);
        ctx.h.emptyLines();

        ctx.c.trimEnd(1);
        ctx.c.decreaseIndent();
        ctx.c.beginBlock();
        ctx.c.endBlock();
        ctx.c.emptyLines();

        ctx.c.beginFunction("ClOptions::~ClOptions()");
        ctx.c.endFunction();
        ctx.c.emptyLines();
    }

    if (settings.generate_struct)
        cmd.emitStruct(ctx, settings);
    if (!settings.namespace_.empty())
    {
        ctx.h.endNamespace();
        ctx.c.endNamespace();
    }

    // qt
    const String qt_var = "in_widget";
    ctx.qt.beginFunction("void createOptionWidgets(QLayout *" + qt_var + ", Options &options)");
    cmd.emitQtWidgets(ctx.qt, settings, qt_var);
    ctx.qt.endFunction();

    write_file(h, ctx.h.getText());
    write_file(cpp, ctx.c.getText());
    write_file(h.parent_path() / h.stem() += ".qt.inl", ctx.qt.getText());
}

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
