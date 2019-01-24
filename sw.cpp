#pragma sw require header org.sw.demo.ragel-6
#pragma sw require header org.sw.demo.lexxmark.winflexbison.bison-master

#pragma sw header on

static void gen_stamp(const DependencyPtr &tools_stamp_gen, NativeExecutedTarget &t)
{
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

static void gen_sqlite2cpp(const DependencyPtr &tools_sqlite2cpp, NativeExecutedTarget &t, const path &sql_file, const path &out_file, const String &ns)
{
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

static void embed(const DependencyPtr &embedder, NativeExecutedTarget &t, const path &in)
{
    if (in.is_absolute())
        throw std::runtime_error("embed: in must be relative to SourceDir");

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

static Files syncqt(const DependencyPtr &sqt, NativeExecutedTarget &t, const Strings &modules)
{
    {
        auto d = t + sqt;
        d->Dummy = true;
    }

    Files out;
    auto i = t.BinaryDir / "include";
    auto v = t.pkg.version.toString();
    for (auto &m : modules)
    {
        fs::create_directories(i / m / v / m);

        auto c = t.addCommand();
        c << cmd::prog(sqt)
            << "-s" << t.SourceDir
            << "-b" << t.BinaryDir
            << "-m" << m
            << "-v" << v
            << cmd::end()
            << cmd::out(i / m / m)
            ;
        c.c->strict_order = 1; // run before moc
        out.insert(i / m / m);
        //t.Interface += i / m / m; makes cyclic deps

        t.Public += IncludeDirectory(i);
        t.Public += IncludeDirectory(i / m);
        t.Public += IncludeDirectory(i / m / v);
        t.Public += IncludeDirectory(i / m / v / m);
    }
    return out;
}

#pragma sw header off

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

#define ADD_LIBRARY_WITH_NAME(var, name)          \
    auto &var = p.addTarget<LibraryTarget>(name); \
    setup_primitives(var)

#define ADD_LIBRARY(x) ADD_LIBRARY_WITH_NAME(x, #x)

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
    yaml.Public += string, templates,
        "org.sw.demo.jbeder.yaml_cpp-master"_dep;

    ADD_LIBRARY(pack);
    pack.Public += filesystem, templates,
        "org.sw.demo.libarchive.libarchive-3"_dep;

    ADD_LIBRARY(patch);
    patch.Public += filesystem, templates;

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
    gen_flex_bison_pair("org.sw.demo.lexxmark.winflexbison-master"_dep, settings, "LALR1_CPP_VARIANT_PARSER", "src/settings");
    gen_flex_bison_pair("org.sw.demo.lexxmark.winflexbison-master"_dep, settings, "LALR1_CPP_VARIANT_PARSER", "src/path");

    ADD_LIBRARY(symbol);
    symbol.Public += filesystem, "org.sw.demo.boost.dll-1"_dep;

    ADD_LIBRARY_WITH_NAME(sw_settings, "sw.settings");
    sw_settings.Public += settings;
    sw_settings -= "src/sw.settings.program_name.cpp";
    //sw_settings.Interface += "src/sw.settings.program_name.cpp";

    ADD_LIBRARY(version);
    version.Public += string, templates,
        "org.sw.demo.fmt-*"_dep,
        "org.sw.demo.boost.container_hash-1"_dep,
        "org.sw.demo.imageworks.pystring-1"_dep;
    gen_ragel("org.sw.demo.ragel-6"_dep, version, "src/version.rl");
    gen_flex_bison_pair("org.sw.demo.lexxmark.winflexbison-master"_dep, version, "GLR_CPP_PARSER", "src/range");

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

    auto &test = p.addDirectory("test");
    test.Scope = TargetScope::Test;

    auto add_test = [&test, &s](const String &name) -> decltype(auto)
    {
        auto &t = test.addTarget<ExecutableTarget>(name);
        t.CPPVersion = CPPLanguageStandard::CPP17;
        t += path("src/" + name + ".cpp");
        t += "org.sw.demo.catchorg.catch2-*"_dep;
        if (s.Settings.Native.CompilerType == CompilerType::MSVC)
            t.CompileOptions.push_back("-bigobj");
        //else if (s.Settings.Native.CompilerType == CompilerType::GNU)
            //t.CompileOptions.push_back("-Wa,-mbig-obj");
        return t;
    };

    auto &test_main = add_test("main");
    test_main += sw_main, command, date_time,
        executor, hash, yaml, context, http,
        "org.sw.demo.nlohmann.json-*"_dep;

    auto &test_db = add_test("db");
    test_db += sw_main, command, db_sqlite3, db_postgresql, date_time,
        executor, hash, yaml;

    auto &test_settings = add_test("settings");
    test_settings.PackageDefinitions = true;
    test_settings += sw_main, settings;

    auto &test_version = add_test("version");
    test_version += sw_main, version;

    auto &test_patch = add_test("patch");
    test_patch += sw_main, patch;

    s.addTest(test_main);
    auto tdb = s.addTest(test_db);
    if (s.Settings.TargetOS.Type == OSType::Windows)
    {
        tdb.c->addLazyAction([&s, c = tdb.c]
        {
            auto &pq = s.getTarget<LibraryTarget>(PackageId{"org.sw.demo.find.libpq", "master"});
            c->addPathDirectory(pq.LinkDirectories.begin()->parent_path() / "bin");
        });
    }
    s.addTest(test_patch);
    s.addTest(test_settings);
    s.addTest(test_version);
}
