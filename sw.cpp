#define ADD_LIBRARY(x) \
    auto &x = p.addTarget<StaticLibraryTarget>(#x); \
    x.setRootDirectory(#x); \
    x.CPPVersion = CPPLanguageStandard::CPP17

void build(Solution &s)
{
    auto &p = s.addProject("primitives", "master");
    p.Source = Git("https://github.com/egorpugin/primitives", "", "{v}");

    ADD_LIBRARY(string);
    string.Public += "pub.cppan2.demo.boost.algorithm"_dep;

    ADD_LIBRARY(filesystem);
    filesystem.Public += string,
        "pub.cppan2.demo.boost.filesystem"_dep,
        "pub.cppan2.demo.artyom_beilis.nowide-master"_dep;

    ADD_LIBRARY(templates);
    ADD_LIBRARY(context);
    ADD_LIBRARY(minidump);
    if (s.Settings.TargetOS.Type == OSType::Windows)
        minidump.Public += "dbghelp.lib"_l;

    ADD_LIBRARY(executor);
    executor.Public += templates, minidump,
        "pub.cppan2.demo.boost.asio"_dep,
        "pub.cppan2.demo.boost.system"_dep;

    ADD_LIBRARY(command);
    command.Public += templates, filesystem,
        "pub.cppan2.demo.boost.process"_dep;
    if (s.Settings.TargetOS.Type == OSType::Windows)
        command.Public += "Shell32.lib"_l;

    ADD_LIBRARY(date_time);
    date_time.Public += string,
        "pub.cppan2.demo.boost.date_time"_dep;

    ADD_LIBRARY(lock);
    lock.Public += filesystem,
        "pub.cppan2.demo.boost.interprocess"_dep;

    ADD_LIBRARY(log);
    log.Public +=
        "pub.cppan2.demo.boost.log"_dep;

    ADD_LIBRARY(yaml);
    yaml.Public += string,
        "pub.cppan2.demo.jbeder.yaml_cpp-master"_dep;

    ADD_LIBRARY(pack);
    pack.Public += filesystem, templates,
        "pub.cppan2.demo.libarchive.libarchive-3"_dep;

    ADD_LIBRARY(http);
    http.Public += filesystem, templates,
        "pub.cppan2.demo.badger.curl.libcurl-7"_dep;
    if (s.Settings.TargetOS.Type == OSType::Windows)
        http.Public += "Winhttp.lib"_l;

    ADD_LIBRARY(hash);
    hash.Public += filesystem,
        "pub.cppan2.demo.aleksey14.rhash-1"_dep,
        "pub.cppan2.demo.openssl.crypto-1"_dep;
}
