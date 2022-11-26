/*
Copyright (C) 2017-2018 Egor Pugin <egor.pugin@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <boost/algorithm/string.hpp>
#include <primitives/filesystem.h>
#include <primitives/sw/main.h>
#include <primitives/sw/cl.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>

struct File
{
    bool public_header = true;
    bool qpa_header = true;
    bool no_master_include = false;
    bool clean = true;
    String requires_;
};

const StringMap<path> module_dirs {
    // base
    { "QtGui", "src/gui" },
    { "QtWidgets", "src/widgets" },
    { "QtPrintSupport", "src/printsupport" },
    { "QtOpenGL", "src/opengl" },
    { "QtCore", "src/corelib" },
    { "QtXml", "src/xml" },
    { "QtSql", "src/sql" },
    { "QtNetwork", "src/network" },
    { "QtTest", "src/testlib" },
    { "QtDBus", "src/dbus" },
    { "QtConcurrent", "src/concurrent" },
    { "QtAccessibilitySupport", "src/platformsupport/accessibility" },
    { "QtWindowsUIAutomationSupport", "src/platformsupport/windowsuiautomation" },
    { "QtLinuxAccessibilitySupport", "src/platformsupport/linuxaccessibility" },
    { "QtClipboardSupport", "src/platformsupport/clipboard" },
    { "QtDeviceDiscoverySupport", "src/platformsupport/devicediscovery" },
    { "QtEventDispatcherSupport", "src/platformsupport/eventdispatchers" },
    { "QtFontDatabaseSupport", "src/platformsupport/fontdatabases" },
    { "QtInputSupport", "src/platformsupport/input" },
    { "QtPlatformCompositorSupport", "src/platformsupport/platformcompositor" },
    { "QtServiceSupport", "src/platformsupport/services" },
    { "QtThemeSupport", "src/platformsupport/themes" },
    { "QtGraphicsSupport", "src/platformsupport/graphics" },
    { "QtCglSupport", "src/platformsupport/cglconvenience" },
    { "QtEglSupport", "src/platformsupport/eglconvenience" },
    { "QtFbSupport", "src/platformsupport/fbconvenience" },
    { "QtGlxSupport", "src/platformsupport/glxconvenience" },
    { "QtKmsSupport", "src/platformsupport/kmsconvenience" },
    { "QtPlatformHeaders", "src/platformheaders" },
    //"QtANGLE/KHR", "!src/3rdparty/angle/include/KHR",
    //"QtANGLE/GLES2", "!src/3rdparty/angle/include/GLES2",
    //"QtANGLE/GLES3", "!src/3rdparty/angle/include/GLES3",
    //"QtANGLE/EGL", "!src/3rdparty/angle/include/EGL",
    //"QtZlib", "!>src/corelib;src/3rdparty/zlib",
    //"QtOpenGLExtensions", "src/openglextensions",
    //"QtEglFSDeviceIntegration", "src/plugins/platforms/eglfs",

    // other
    {"Qt3DCore" , "src/core"},
    {"Qt3DRender" , "src/render"},
    {"Qt3DQuick" , "src/quick3d/quick3d"},
    {"Qt3DQuickRender" , "src/quick3d/quick3drender"},
    {"Qt3DInput" , "src/input"},
    {"Qt3DAnimation" , "src/animation"},
    {"Qt3DQuickAnimation" , "src/quick3d/quick3danimation"},
    {"Qt3DQuickInput" , "src/quick3d/quick3dinput"},
    {"Qt3DLogic" , "src/logic"},
    {"Qt3DExtras" , "src/extras"},
    {"Qt3DQuickExtras" , "src/quick3d/quick3dextras"},
    {"Qt3DQuickScene2D" , "src/quick3d/quick3dscene2d"},
    {"ActiveQt" , "src/activeqt"},
    {"QtAndroidExtras" , "src/androidextras"},
    {"QtCanvas3D" , "src"},
    {"QtCharts" , "src/charts"},
    {"QtBluetooth" , "src/bluetooth"},
    {"QtNfc" , "src/nfc"},
    {"QtDataVisualization" , "src/datavisualization"},
    {"QtQml" , "src/qml"},
    {"QtQmlModels" , "src/qmlmodels"},
    {"QtQmlWorkerScript" , "src/qmlworkerscript"},
    {"QtQuick" , "src/quick"},
    {"QtQuickWidgets" , "src/quickwidgets"},
    {"QtQuickParticles" , "src/particles"},
    {"QtQuickTest" , "src/qmltest"},
    {"QtPacketProtocol" , "src/plugins/qmltooling/packetprotocol"},
    {"QtQmlDebug" , "src/qmldebug"},
    {"QtGamepad" , "src/gamepad"},
    {"QtLocation" , "src/location"},
    {"QtPositioning" , "src/positioning"},
    {"QtMacExtras" , "src/macextras"},
    {"QtMultimedia" , "src/multimedia"},
    {"QtMultimediaWidgets" , "src/multimediawidgets"},
    {"QtMultimediaQuick_p" , "src/qtmultimediaquicktools"},
    {"QtNetworkAuth" , "src/oauth"},
    {"QtPurchasing" , "src/purchasing"},
    {"QtQuickControls2" , "src/quickcontrols2"},
    {"QtQuickTemplates2" , "src/quicktemplates2"},
    {"QtRemoteObjects" , "src/remoteobjects"},
    {"QtRepParser" , "src/repparser"},
    {"QtScript" , "src/script"},
    {"QtScriptTools" , "src/scripttools"},
    {"QtScxml" , "src/scxml"},
    {"QtSensors" , "src/sensors"},
    {"QtSerialBus" , "src/serialbus"},
    {"QtSerialPort" , "src/serialport"},
    {"QtSvg", "src/svg"},
    {"QtTextToSpeech" , "src/tts"},
    {"QtHelp" , "src/assistant/help"},
    {"QtUiTools" , "src/designer/src/uitools"},
    {"QtUiPlugin" , "src/designer/src/uiplugin"},
    {"QtDesigner" , "src/designer/src/lib"},
    {"QtDesignerComponents" , "src/designer/src/components/lib"},
    {"QtWaylandCompositor" , "src/compositor"},
    {"QtWaylandClient" , "src/client"},
    {"QtWaylandEglClientHwIntegration" , "src/hardwareintegration/client/wayland-egl"},
    {"QtWebChannel" , "src/webchannel"},
    {"QtWebEngine" , "src/webengine"},
    {"QtWebEngineWidgets" , "src/webenginewidgets"},
    {"QtWebEngineCore" , "src/core"},
    {"QtWebSockets" , "src/websockets"},
    {"QtWebView" , "src/webview"},
    {"QtWinExtras" , "src/winextras"},
    {"QtX11Extras" , "src/x11extras"},
    {"QtXmlPatterns" , "src/xmlpatterns"},
    {"QtCore5Compat" , "src/core5"},
};

const StringMap<String> moduleheaders {
    { "QtEglFSDeviceIntegration", "api" },
};

const StringMap<StringSet> classnames {

   { "qscopeguard.h", {"QScopeGuard"} },
   { "qshareddata.h", {"QSharedData"} },

   { "qglobal.h", {"QtGlobal"} },
   { "qendian.h", {"QtEndian"} },
   { "qconfig.h", {"QtConfig"} },
   { "qplugin.h", {"QtPlugin"} },
   { "qalgorithms.h", {"QtAlgorithms"} },
   { "qcontainerfwd.h", {"QtContainerFwd"} },
   { "qdebug.h", {"QtDebug"} },
   { "qevent.h", {"QtEvents"} },
   { "qnamespace.h", {"Qt"} },
   { "qnumeric.h", {"QtNumeric"} },
   { "qvariant.h", {"QVariantHash","QVariantList","QVariantMap"} },
   { "qgl.h", {"QGL"} },
   { "qsql.h", {"QSql"} },
   { "qssl.h", {"QSsl"} },
   { "qtest.h", {"QTest"} },
   { "qtconcurrentmap.h", {"QtConcurrentMap"} },
   { "qtconcurrentfilter.h", {"QtConcurrentFilter"} },
   { "qtconcurrentrun.h", {"QtConcurrentRun"} },

    {"qchartglobal.h" , {"QChartGlobal"}    },
    {"qaudio.h" , {"QAudio"}},
    {"qmediametadata.h" , {"QMediaMetaData"}},
    {"qmultimedia.h" , {"QMultimedia" }   },
    {"qwaylandquickextension.h" , {"QWaylandQuickExtension"}},
    {"qwebsocketprotocol.h" , {"QWebSocketProtocol"}},

    { "qscopedvaluerollback.h", {"QScopedValueRollback"} },
};

const StringMap<StringMap<String>> deprecatedheaders {
    {"QtGui", {
        {"QGenericPlugin", "QtGui/QGenericPlugin"},
        {"QGenericPluginFactory", "QtGui/QGenericPluginFactory"},
    }},
    {"QtSql", {
        {"qsql.h", "QtSql/qtsqlglobal.h"},
    }},
    {"QtDBus", {
        {"qdbusmacros.h", "QtDbus/qtdbusglobal.h"},
    }},
};

const StringSet angle_headers {
    "egl.h", "eglext.h", "eglplatform.h", "gl2.h", "gl2ext.h", "gl2platform.h", "ShaderLang.h", "khrplatform.h"
};

const StringSet internal_zlib_headers {
    "crc32.h", "deflate.h", "gzguts.h", "inffast.h", "inffixed.h", "inflate.h", "inftrees.h", "trees.h", "zutil.h"
};

const StringSet zlib_headers {
    "zconf.h", "zlib.h"
};

const StringSet ignore_headers = internal_zlib_headers;

StringSet ignore_for_include_check {
    "qsystemdetection.h", "qcompilerdetection.h", "qprocessordetection.h"
};

StringSet ignore_for_qt_begin_namespace_check {
    "qt_windows.h"
};

const std::map<String, StringSet> inject_headers {
    {"src/corelib/global", { "qconfig.h", "qconfig_p.h" }},
    {"src/qml", { "qqmljsgrammar_p.h", "qqmljsparser_p.h" }},
};

const std::set<String> qpa_headers {
    "^(?!qplatformheaderhelper)qplatform", "^qwindowsystem"
};

std::regex r_header("^[-a-z0-9_]*\\.h$");
std::regex r_skip_header("^qt[a-z0-9]+-config(_p)?\\.h$");
std::regex r_private_header("_p(ch)?\\.h$");
std::vector<std::regex> r_qpa_headers;
std::regex r_qt_class("^#pragma qt_class\\(([^)]*)\\)[\r\n]*$");
std::regex r_class(R"((class|struct)\s+(\[?\[?\w+\]?\]?\s+)?(\S+\s+)?(Q\w+))");
std::regex r_typedef(R"(typedef.*\W(Q\w+);)");
std::regex r_require(R"xx(^QT_REQUIRE_CONFIG\((.*)\);[\r\n]*$)xx");

std::map<String, std::map<path, File>> modules;

void init()
{
    ignore_for_include_check.insert(zlib_headers.begin(), zlib_headers.end());
    ignore_for_include_check.insert(angle_headers.begin(), angle_headers.end());

    ignore_for_qt_begin_namespace_check.insert(zlib_headers.begin(), zlib_headers.end());
    ignore_for_qt_begin_namespace_check.insert(angle_headers.begin(), angle_headers.end());

    for (auto &qpa : qpa_headers)
        r_qpa_headers.emplace_back(qpa);
}

// elaborate more
StringSet class_names(std::map<path, File>::value_type &ff)
{
    StringSet classes;

    auto lines = read_lines(ff.first);
    auto &f = ff.second;

    std::smatch m;
    for (auto &line : lines)
    {
        if (line[0] == '#')
        {
            if (line.find("#pragma qt_no_master_include") == 0)
                f.no_master_include = true;
            if (line.find("#pragma qt_sync_skip_header_check") == 0)
                f.clean = false;
            if (line.find("#pragma qt_sync_stop_processing") == 0)
                return classes;
            if (std::regex_search(line, m, r_qt_class))
                classes.insert(m[1]);
            continue;
        }
        if (line.find(';') == -1 && std::regex_search(line, m, r_class)) {
            if (1) {
                classes.insert(m[4]);
            }
        }
        if (std::regex_search(line, m, r_typedef))
            classes.insert(m[1]);
        if (std::regex_search(line, m, r_require))
            f.requires_ = m[1];
    }
    return classes;
}

int main(int argc, char *argv[])
{
    cl::opt<path> sdir("sdir", cl::desc("source dir"), cl::Required);
    cl::opt<path> bdir("bdir", cl::desc("binary dir"), cl::Required);
    cl::list<std::string> selected_modules_cl("modules", cl::desc("modules"), cl::SpaceSeparated);
    cl::opt<std::string> version("qt_version", cl::desc("version"), cl::Required);

    cl::alias sdirA("s", cl::desc("Alias for -sdir"), cl::aliasopt(sdir));
    cl::alias bdirA("b", cl::desc("Alias for -bdir"), cl::aliasopt(bdir));
    cl::alias modulesA("m", cl::desc("Alias for -modules"), cl::aliasopt(selected_modules_cl));
    cl::alias versionA("v", cl::desc("Alias for -version"), cl::aliasopt(version));

    cl::ParseCommandLineOptions(argc, argv);

    init();

    Strings selected_modules;
    for (auto &m : module_dirs)
        selected_modules.push_back(m.first);
    // if selected_modules_cl specified
    selected_modules = selected_modules_cl;

    // find files
    for (auto &m : selected_modules)
    {
        auto im = module_dirs.find(m);
        if (im == module_dirs.end())
        {
            std::cout << "bad module: " << m << "\n";
            continue;
        }

        auto files = enumerate_files(sdir / im->second);
        for (auto &f : files)
        {
            if (is_under_root(f, sdir / im->second / "doc"))
                continue;
            auto fn = f.filename().string();
            if (!(
                std::regex_search(fn, r_header) &&
                !std::regex_search(fn, r_skip_header) &&
                fn.find("ui_") != 0/* &&
                fn.find("q") == 0*/
                ))
                continue;

            modules[m][f];
        }
    }

    // process
    for (auto &mm : modules)
    {
        auto m = mm.first;
        std::ostringstream master;
        //master << "#pragma once" << "\n";
        master << "#ifndef QT_" << boost::to_upper_copy(m) << "_MODULE_H" << "\n";
        master << "#define QT_" << boost::to_upper_copy(m) << "_MODULE_H" << "\n";
        master << "#include <" << m << "/" << m << "Depends>" << "\n";
        master << "#include \"qglobal.h\"" << "\n";

        for (auto &ff : mm.second)
        {
            auto p = ff.first;
            auto fn = p.filename().string();
            auto &f = ff.second;

            if (fn.find('_') != -1 || fn.find("qconfig") != -1)
                f.no_master_include = true;

            // check qpa
            for (auto &qpa : r_qpa_headers)
            {
                f.qpa_header = std::regex_search(fn, qpa);
                if (f.qpa_header)
                {
                    f.public_header = false;
                    break;
                }
            }

            if (!f.qpa_header)
                f.public_header = !std::regex_search(fn, r_private_header);

            StringSet classes;
            if (f.public_header)
                classes = class_names(ff);

            auto ic = classnames.find(fn);
            if (ic != classnames.end())
                classes.insert(ic->second.begin(), ic->second.end());

            auto oheader = bdir / "include" / m;
            if (f.public_header)
            {
                for (auto &c : classes) {
                    if (c != "Queue") {
                        write_file(oheader / c, "#include \"" + fn + "\"\n");
                    }
                }

                if (!f.no_master_include)
                {
                    if (!f.requires_.empty())
                        master << "#if QT_CONFIG(" << f.requires_ << ")\n";
                    master << "#include \"" << fn << "\"\n";
                    if (!f.requires_.empty())
                        master << "#endif\n";
                }
            }
            else if (f.qpa_header)
                oheader /= path(version.getValue()) / m / "qpa";
            else
                oheader /= path(version.getValue()) / m / "private";
            oheader /= fn;
            // if we're on the other disk or whatever else, use abs path
            auto rel = fs::relative(p, oheader.parent_path());
            // sometimes rel is incorrect (5.10.0/QtEventDispatcherSupport/private/qwindowsguieventdispatcher_p.h),
            // set abs for the moment
            //if (rel.empty())
                rel = p;
            write_file(oheader, "#include \"" + to_string(normalize_path(rel)) + "\"\n");
        }

        master << "#include \"" << boost::to_lower_copy(m) + "version.h\"" << "\n";
        master << "#endif" << "\n";

        int ma, mi, pa;
        sscanf(&version[0], "%d.%d.%d", &ma, &mi, &pa);
        char x[10];
        sprintf(x, "0x%02d%02d%02d", ma, mi, pa);

        std::ofstream v(bdir / "include" / m / (boost::to_lower_copy(m) + "version.h"));
        v << "#pragma once" << "\n";
        v << "\n";
        v << "#define " << boost::to_upper_copy(m) + "_VERSION_STR \"" << version << "\"\n";
        v << "\n";
        v << "#define " << boost::to_upper_copy(m) + "_VERSION " << x << "\n";

        std::ofstream v2(bdir / "include" / m / (m + "Version"));
        v2 << "#include \"" << boost::to_lower_copy(m) + "version.h" <<  "\"\n";

        // make last to perform commit
        std::ofstream(bdir / "include" / m / m) << master.str();
    }

    return 0;
}
