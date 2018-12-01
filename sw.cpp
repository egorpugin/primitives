#ifdef SW_PRAGMA_HEADER
#pragma sw header on

void gen_stamp(NativeExecutedTarget &t)
{
    auto tools_stamp_gen = THIS_PREFIX "." "primitives.tools.stamp_gen" "-" THIS_VERSION_DEPENDENCY;
    {
        auto d = t + tools_stamp_gen;
        d->Dummy = true;
    }

    auto out = t.BinaryPrivateDir / "stamp.h.in";

    SW_MAKE_COMMAND_AND_ADD(c, t);
    c->always = true;
    c->setProgram(tools_stamp_gen);
    c->redirectStdout(out);
    c->addOutput(out);
    t += out;
}

void gen_sqlite2cpp(NativeExecutedTarget &t, const path &sql_file, const path &out_file, const String &ns)
{
    auto tools_sqlite2cpp = THIS_PREFIX "." "primitives.tools.sqlpp11.sqlite2cpp" "-" THIS_VERSION_DEPENDENCY;
    {
        auto d = t + tools_sqlite2cpp;
        d->Dummy = true;
    }

    auto out = t.BinaryDir / out_file;

    SW_MAKE_COMMAND_AND_ADD(c, t);
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
    if (in.is_absolute())
        throw std::runtime_error("embed: in must be relative to SourceDir");

    auto embedder = THIS_PREFIX "." "primitives.tools.embedder" "-" THIS_VERSION_DEPENDENCY;
    {
        auto d = t + embedder;
        d->Dummy = true;
    }

    auto f = t.SourceDir / in;
    auto wdir = f.parent_path();
    auto out = t.BinaryDir / in.parent_path() / in.filename().stem();

    SW_MAKE_COMMAND_AND_ADD(c, t);
    c->setProgram(embedder);
    c->working_directory = wdir;
    c->args.push_back(f.u8string());
    c->args.push_back(out.u8string());
    c->addInput(f);
    c->addOutput(out);
    t += in, out;
    t += IncludeDirectory(out.parent_path()); // but remove this later
}

void syncqt(NativeExecutedTarget &t, const Strings &modules)
{
    auto sqt = THIS_PREFIX "." "primitives.tools.syncqt" "-" THIS_VERSION_DEPENDENCY;
    {
        auto d = t + sqt;
        d->Dummy = true;
    }

    auto v = t.pkg.version.toString();
    for (auto &m : modules)
    {
        fs::create_directories(t.BinaryDir / "include" / m / v / m);

        auto c = t.addCommand();
        c << cmd::prog(sqt)
            << "-s" << t.SourceDir
            << "-b" << t.BinaryDir
            << "-m" << m
            << "-v" << v
            << cmd::end()
            << cmd::out(t.BinaryDir / "include" / m / m)
            ;

        t.Public += IncludeDirectory(t.BinaryDir / "include");
        t.Public += IncludeDirectory(t.BinaryDir / "include" / m);
        t.Public += IncludeDirectory(t.BinaryDir / "include" / m / v);
        t.Public += IncludeDirectory(t.BinaryDir / "include" / m / v / m);
    }
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
        t.PackageDefinitions = true;
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

    ADD_LIBRARY(error_handling);

    auto &templates = p.addTarget<StaticLibraryTarget>("templates");
    setup_primitives(templates);

    ADD_LIBRARY(string);
    string.Public += "org.sw.demo.boost.algorithm-1"_dep;

    ADD_LIBRARY(filesystem);
    filesystem.Public += string, templates,
        "org.sw.demo.boost.filesystem-1"_dep,
        "org.sw.demo.boost.thread-1"_dep,
        "org.sw.demo.grisumbras.enum_flags-master"_dep;

    ADD_LIBRARY(file_monitor);
    file_monitor.Public += filesystem,
        "org.sw.demo.libuv-1"_dep;

    ADD_LIBRARY(context);
    context.Public += filesystem;

    ADD_LIBRARY(executor);
    executor.Public += templates,
        "org.sw.demo.boost.asio-1"_dep,
        "org.sw.demo.boost.system-1"_dep;

    ADD_LIBRARY(command);
    command.Public += file_monitor,
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

    ADD_LIBRARY(cron);
    cron.Public += executor, log;

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
    if (s.Settings.TargetOS.Type == OSType::Windows)
    {
        win32helpers.Public += "UNICODE"_d;
        win32helpers += "Shell32.lib"_lib, "Ole32.lib"_lib, "Advapi32.lib"_lib, "user32.lib"_lib;
    }

    ADD_LIBRARY_WITH_NAME(db_common, "db.common");
    db_common.Public += filesystem, templates,
        "org.sw.demo.imageworks.pystring-1"_dep;

    ADD_LIBRARY_WITH_NAME(db_sqlite3, "db.sqlite3");
    db_sqlite3.Public += db_common, "org.sw.demo.sqlite3"_dep;

    ADD_LIBRARY_WITH_NAME(db_postgresql, "db.postgresql");
    db_postgresql.Public += db_common, "org.sw.demo.jtv.pqxx-*"_dep;

    auto &main = p.addTarget<StaticLibraryTarget>("main");
    setup_primitives(main);
    main.Public += error_handling;

    ADD_LIBRARY(settings);
    settings.Public += yaml, filesystem, templates,
        "pub.egorpugin.llvm_project.llvm.support_lite-master"_dep;
    gen_flex_bison_pair(settings, "LALR1_CPP_VARIANT_PARSER", "src/settings");
    gen_flex_bison_pair(settings, "LALR1_CPP_VARIANT_PARSER", "src/path");

    ADD_LIBRARY(symbol);
    symbol.Public += filesystem, "org.sw.demo.boost.dll-1"_dep;

    ADD_LIBRARY_WITH_NAME(sw_settings, "sw.settings");
    sw_settings.Public += settings;
    sw_settings -= "src/sw.settings.program_name.cpp";
    //sw_settings.Interface += "src/sw.settings.program_name.cpp";

    auto &sw_main = p.addTarget<StaticLibraryTarget>("sw.main");
    setup_primitives(sw_main);
    sw_main.Public += main, sw_settings,
        "org.sw.demo.boost.dll-1"_dep;
    sw_main.Interface.LinkLibraries.push_back(main.getImportLibrary()); // main itself
    sw_main.Interface.LinkLibraries.push_back(sw_main.getImportLibrary()); // then me (self, sw.main)
    sw_main.Interface.LinkLibraries.push_back(sw_settings.getImportLibrary()); // then sw.settings
    if (s.Settings.TargetOS.Type == OSType::Windows)
    {
        sw_main.Public +=
            "org.sw.demo.google.breakpad.client.windows.handler-master"_dep,
            "org.sw.demo.google.breakpad.client.windows.crash_generation.client-master"_dep,
            "org.sw.demo.google.breakpad.client.windows.crash_generation.server-master"_dep
            ;
    }

    auto &tools_embedder = p.addTarget<ExecutableTarget>("tools.embedder");
    setup_primitives_no_all_sources(tools_embedder);
    tools_embedder += "src/tools/embedder.cpp";
    tools_embedder += filesystem, sw_main;

    auto &tools_sqlite2cpp = p.addTarget<ExecutableTarget>("tools.sqlpp11.sqlite2cpp");
    setup_primitives_no_all_sources(tools_sqlite2cpp);
    tools_sqlite2cpp += "src/tools/sqlpp11.sqlite2cpp.cpp";
    tools_sqlite2cpp += filesystem, context, sw_main, "org.sw.demo.sqlite3"_dep;

    auto &tools_syncqt = p.addTarget<ExecutableTarget>("tools.syncqt");
    setup_primitives_no_all_sources(tools_syncqt);
    tools_syncqt += "src/tools/syncqt.cpp";
    tools_syncqt += filesystem, sw_main;

    auto &stamp_gen = p.addTarget<ExecutableTarget>("tools.stamp_gen");
    setup_primitives_no_all_sources(stamp_gen);
    stamp_gen += "src/tools/stamp_gen.cpp";

    ADD_LIBRARY(version);
    version.Public += string, templates,
        "org.sw.demo.fmt-*"_dep,
        "org.sw.demo.boost.container_hash-1"_dep,
        "org.sw.demo.imageworks.pystring-1"_dep;
    gen_ragel(version, "src/version.rl");
    gen_flex_bison_pair(version, "GLR_CPP_PARSER", "src/range");

    auto &test = p.addDirectory("test");
    test.Scope = TargetScope::Test;

    /*auto add_test = [](const String &name) -> decltype(auto)
    {
        auto &t = test.addTarget<ExecutableTarget>("main");
        t.CPPVersion = CPPLanguageStandard::CPP17;
        t += "src/" + name + ".cpp";
        return t;
    };

    auto &test_main = add_test("main");
    auto &test_db = add_test("db");
    auto &test_settings = add_test("settings");
    auto &test_version = add_test("version");*/
}
