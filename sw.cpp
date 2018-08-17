#ifdef SW_PRAGMA_HEADER
#pragma sw header on

void gen_sqlite2cpp(NativeExecutedTarget &t, const path &sql_file, const path &out_file, const String &ns)
{
    auto tools_sqlite2cpp = THIS_PREFIX "." "primitives.tools.sqlite2cpp" "-" THIS_VERSION_DEPENDENCY;
    {
        auto d = t + tools_sqlite2cpp;
        d->Dummy = true;
    }

    auto out = t.BinaryDir / out_file;

    auto c = std::make_shared<Command>();
    c->setProgram(tools_sqlite2cpp);
    c->args.push_back(sql_file.u8string());
    c->args.push_back(out.u8string());
    c->args.push_back(ns);
    c->addInput(sql_file);
    c->addOutput(out);
    t += out;
}

void embed(NativeExecutedTarget &t, const path &in)
{
    auto embedder = THIS_PREFIX "." "primitives.tools.embedder" "-" THIS_VERSION_DEPENDENCY;
    {
        auto d = t + embedder;
        d->Dummy = true;
    }

    auto wdir = in.parent_path();
    auto out = t.BinaryDir / in.filename().stem();

    auto c = std::make_shared<Command>();
    c->setProgram(embedder);
    c->working_directory = wdir;
    c->args.push_back(in.u8string());
    c->args.push_back(out.u8string());
    c->addInput(in);
    c->addOutput(out);
    t += out;
}

#pragma sw header off
#endif

#pragma sw require header org.sw.demo.ragel-6
#pragma sw require header org.sw.demo.lexxmark.winflexbison.bison-master

#define ADD_LIBRARY_WITH_NAME(var, name)          \
    auto &var = p.addTarget<LibraryTarget>(name); \
    setup_primitives(var)

#define ADD_LIBRARY(x) ADD_LIBRARY_WITH_NAME(x, #x)

void configure(Solution &s)
{
    //s.Settings.Native.LibrariesType = LibraryType::Static;
    s.Settings.Native.ConfigurationType = ConfigurationType::Debug;
}

void build(Solution &s)
{
    auto &p = s.addProject("primitives", "master");
    p += Git("https://github.com/egorpugin/primitives", "", "master");

    auto setup_primitives_no_all_sources = [](auto &t)
    {
        path p = "src";
        auto n = t.getPackage().ppath.slice(t.getPackage().ppath.isAbsolute() ? 3 : 1);
        auto n2 = n;
        while (!n.empty())
        {
            p /= n.front();
            n = n.slice(1);
        }
        t.ApiName = "PRIMITIVES_" + boost::to_upper_copy(n2.toString("_")) + "_API";
        t.CPPVersion = CPPLanguageStandard::CPP17;
        return p;
    };

    auto setup_primitives = [&setup_primitives_no_all_sources](auto &t)
    {
        auto p = setup_primitives_no_all_sources(t);
        t.setRootDirectory(p);
        // explicit!
        t += "include/.*"_rr;
        t += "src/.*"_rr;
    };

    ADD_LIBRARY(templates);
    ADD_LIBRARY(context);
    ADD_LIBRARY(error_handling);

    ADD_LIBRARY(string);
    string.Public += "org.sw.demo.boost.algorithm-1"_dep;

    ADD_LIBRARY(filesystem);
    filesystem.Public += string,
        "org.sw.demo.libuv-1"_dep,
        "org.sw.demo.boost.filesystem-1"_dep,
        "org.sw.demo.boost.thread-1"_dep,
        "org.sw.demo.grisumbras.enum_flags-master"_dep;

    auto &minidump = p.addTarget<StaticLibraryTarget>("minidump");
    setup_primitives(minidump);
    if (s.Settings.TargetOS.Type == OSType::Windows)
        minidump.Public += "dbghelp.lib"_l;

    ADD_LIBRARY(executor);
    executor.Public += templates, minidump,
        "org.sw.demo.boost.asio-1"_dep,
        "org.sw.demo.boost.system-1"_dep;

    ADD_LIBRARY(command);
    command.Public += templates, filesystem,
        "org.sw.demo.boost.process-1"_dep;
    if (s.Settings.TargetOS.Type == OSType::Windows)
        command.Public += "Shell32.lib"_l;

    ADD_LIBRARY(date_time);
    date_time.Public += string,
        "org.sw.demo.boost.date_time-1"_dep;

    ADD_LIBRARY(lock);
    lock.Public += filesystem,
        "org.sw.demo.boost.interprocess-1"_dep;

    ADD_LIBRARY(log);
    log.Public += "org.sw.demo.boost.log-1"_dep;

#ifndef SW_SELF_BUILD
    ADD_LIBRARY(cron);
    cron.Public += executor, log;
#endif

    ADD_LIBRARY(yaml);
    yaml.Public += string,
        "org.sw.demo.jbeder.yaml_cpp-master"_dep;

    ADD_LIBRARY(pack);
    pack.Public += filesystem, templates,
        "org.sw.demo.libarchive.libarchive-3"_dep;

    ADD_LIBRARY(http);
    http.Public += filesystem, templates,
        "org.sw.demo.badger.curl.libcurl-7"_dep;
    if (s.Settings.TargetOS.Type == OSType::Windows)
        http.Public += "Winhttp.lib"_l;

    ADD_LIBRARY(hash);
    hash.Public += filesystem,
        "org.sw.demo.aleksey14.rhash-1"_dep,
        "org.sw.demo.openssl.crypto-1.*.*.*"_dep;

    ADD_LIBRARY(win32helpers);
    win32helpers.Public += filesystem,
        "org.sw.demo.boost.dll-1"_dep,
        "org.sw.demo.boost.algorithm-1"_dep;
    win32helpers += "Shell32.lib"_lib, "Ole32.lib"_lib;
    if (s.Settings.TargetOS.Type == OSType::Windows)
        win32helpers.Public += "UNICODE"_d;

    ADD_LIBRARY_WITH_NAME(db_common, "db.common");
    db_common.Public += filesystem, templates,
        "org.sw.demo.imageworks.pystring-1"_dep;

    ADD_LIBRARY_WITH_NAME(db_sqlite3, "db.sqlite3");
    db_sqlite3.Public += db_common, "org.sw.demo.sqlite3"_dep;

#ifndef SW_SELF_BUILD
    ADD_LIBRARY_WITH_NAME(db_postgresql, "db.postgresql");
    db_postgresql.Public += db_common, "org.sw.demo.jtv.pqxx-*"_dep;
#endif

    auto &main = p.addTarget<StaticLibraryTarget>("main");
    setup_primitives(main);
    main.Public += error_handling;

    ADD_LIBRARY(settings);
    settings.Public += yaml, filesystem, templates,
        "pub.egorpugin.llvm_project.llvm.support_lite-master"_dep;
    gen_flex_bison_pair(settings, "LALR1_CPP_VARIANT_PARSER", "src/settings");
    gen_flex_bison_pair(settings, "LALR1_CPP_VARIANT_PARSER", "src/path");

    ADD_LIBRARY_WITH_NAME(sw_settings, "sw.settings");
    sw_settings.Public += settings;
    sw_settings.Interface += "src/sw.settings.program_name.cpp";

    auto &sw_main = p.addTarget<StaticLibraryTarget>("sw.main");
    setup_primitives(sw_main);
    sw_main.Public += main, sw_settings;

    auto &tools_embedder = p.addTarget<ExecutableTarget>("tools.embedder");
    setup_primitives_no_all_sources(tools_embedder);
    tools_embedder += "src/tools/embedder.cpp";
    tools_embedder += filesystem, sw_main;

    auto &tools_sqlite2cpp = p.addTarget<ExecutableTarget>("tools.sqlite2cpp");
    setup_primitives_no_all_sources(tools_sqlite2cpp);
    tools_sqlite2cpp += "src/tools/sqlpp11.sqlite2cpp.cpp";
    tools_sqlite2cpp += filesystem, context, sw_main, "org.sw.demo.sqlite3"_dep;

    ADD_LIBRARY(version);
    version.Public += hash, templates,
        "org.sw.demo.fmt-5"_dep,
        "org.sw.demo.imageworks.pystring-1"_dep;
    gen_ragel(version, "src/version.rl");
    gen_flex_bison_pair(version, "GLR_CPP_PARSER", "src/range");
}
