// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/win32helpers.h>

#include <primitives/exceptions.h>

#include <boost/algorithm/string.hpp>
#include <boost/dll.hpp>

#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <winnls.h>
#include <shobjidl.h>
#include <objbase.h>
#include <objidl.h>
#include <shlguid.h>
#include <Shellapi.h>

// CreateLink - Uses the Shell's IShellLink and IPersistFile interfaces
//              to create and store a shortcut to the specified object.
//
// Returns the result of calling the member functions of the interfaces.
//
// Parameters:
// lpszPathObj  - Address of a buffer that contains the path of the object,
//                including the file name.
// lpszPathLink - Address of a buffer that contains the path where the
//                Shell link is to be stored, including the file name.
// lpszDesc     - Address of a buffer that contains a description of the
//                Shell link, stored in the Comment field of the link
//                properties.

HRESULT CreateLink(LPCTSTR lpszPathObj, LPCTSTR lpszPathLink, LPCTSTR lpszDesc)
{
    HRESULT hres;
    IShellLink* psl;

    hres = CoInitialize(nullptr);

    // Get a pointer to the IShellLink interface. It is assumed that CoInitialize
    // has already been called.
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if (SUCCEEDED(hres))
    {
        IPersistFile* ppf;

        // Set the path to the shortcut target and add the description.
        psl->SetPath(lpszPathObj);
        psl->SetDescription(lpszDesc);

        // Query IShellLink for the IPersistFile interface, used for saving the
        // shortcut in persistent storage.
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

        if (SUCCEEDED(hres))
        {
            //WCHAR wsz[MAX_PATH];

            // Ensure that the string is Unicode.
            //MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1, wsz, MAX_PATH);

            // Add code here to check return value from MultiByteWideChar
            // for success.

            // Save the link by calling IPersistFile::Save.
            hres = ppf->Save(lpszPathLink, TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    return hres;
}

BOOL IsRunAsAdmin()
{
    BOOL fIsRunAsAdmin = FALSE;
    DWORD dwError = ERROR_SUCCESS;
    PSID pAdministratorsGroup = NULL;

    // Allocate and initialize a SID of the administrators group.
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(
        &NtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &pAdministratorsGroup))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Determine whether the SID of administrators group is enabled in
    // the primary access token of the process.
    if (!CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

Cleanup:
    // Centralized cleanup for all allocated resources.
    if (pAdministratorsGroup)
    {
        FreeSid(pAdministratorsGroup);
        pAdministratorsGroup = NULL;
    }

    // Throw the error if something failed in the function.
    if (ERROR_SUCCESS != dwError)
    {
        throw dwError;
    }

    return fIsRunAsAdmin;
}

ULONG_PTR GetParentProcessId()
{
    ULONG_PTR pbi[6];
    ULONG ulSize = 0;
    LONG(WINAPI * NtQueryInformationProcess)(HANDLE ProcessHandle, ULONG ProcessInformationClass,
        PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
    *(FARPROC *)&NtQueryInformationProcess = GetProcAddress(LoadLibraryA("NTDLL.DLL"), "NtQueryInformationProcess");
    if (NtQueryInformationProcess)
    {
        auto r = NtQueryInformationProcess(GetCurrentProcess(), 0, &pbi, sizeof(pbi), &ulSize);
        if (r >= 0 && ulSize == sizeof(pbi))
            return pbi[5];
    }
    return (ULONG_PTR)-1;
}

void Elevate()
{
    if (IsRunAsAdmin())
        return;

    const auto loc = boost::dll::program_location().wstring();

    std::wstring cmd = GetCommandLine();
    int nargs;
    auto wargv = CommandLineToArgvW(cmd.c_str(), &nargs);
    int o = cmd[0] == '\"' ? 1 : 0;
    cmd.replace(cmd.find(wargv[0]) - o, wcslen(wargv[0]) + o + o, L"");
    LocalFree(wargv);

    // Launch as administrator.
    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = loc.c_str();
    sei.lpParameters = cmd.c_str();
    sei.nShow = SW_NORMAL;

    if (!ShellExecuteEx(&sei))
        throw SW_RUNTIME_ERROR("Cannot elevate privilegies: " + get_last_error());

    // normal exit
    exit(0);
}

std::string get_last_error(DWORD ec)
{
    LPVOID lpMsgBuf;

    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        ec,
        0,
        //MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&lpMsgBuf,
        0, NULL);

    std::string s = (const char *)lpMsgBuf;
    LocalFree(lpMsgBuf);

    boost::trim(s);

    return s;
}

std::string get_last_error()
{
    return get_last_error(GetLastError());
}

void message_box(const std::string &caption, const std::string &s)
{
    if (MessageBoxA(0, s.c_str(), caption.c_str(), 0) == 0)
    {
        auto e = GetLastError();
        message_box(caption, "MessageBoxA error: " + get_last_error(e));
    }
}

void message_box(const std::string &caption, const std::wstring &s)
{
    auto ws = to_wstring(caption);
    if (MessageBoxW(0, s.c_str(), ws.c_str(), 0) == 0)
    {
        auto e = GetLastError();
        message_box(caption, "MessageBoxW error: " + get_last_error(e));
    }
}

void message_box(const std::string &s)
{
    message_box(PACKAGE_NAME_CLEAN, s);
}

void message_box(const std::wstring &s)
{
    message_box(PACKAGE_NAME_CLEAN, s);
}

#include <fcntl.h>
#include <io.h>

void BindStdHandlesToConsole()
{
    // Redirect the CRT standard input, output, and error handles to the console
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);

    /*setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);*/

    //Clear the error state for each of the C++ standard stream objects. We need to do this, as
    //attempts to access the standard streams before they refer to a valid target will cause the
    //iostream objects to enter an error state. In versions of Visual Studio after 2005, this seems
    //to always occur during startup regardless of whether anything has been read from or written to
    //the console or not.
    std::wcout.clear();
    std::cout.clear();
    std::wcerr.clear();
    std::cerr.clear();
    std::wcin.clear();
    std::cin.clear();

    //std::ios_base::sync_with_stdio();
}

void SetupConsole()
{
    if (!GetConsoleWindow())
    {
        // read flags before attach
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD flags;
        GetHandleInformation(out, &flags);

        // We still need to attach to console even if handles are inherited.
        // Without this we will get flickering windows on process startups.

        if (!AttachConsole(ATTACH_PARENT_PROCESS))
        {
            auto e = GetLastError();
            // specified process does not have a console
            if (e == ERROR_INVALID_HANDLE)
            {
                // probably here we start as a gui program
            }
            // specified process does not exist
            else if (e == ERROR_INVALID_PARAMETER)
            {
                // probably here we start as a gui program
            }

            if (!AllocConsole())
            {
                std::string s;
                s += "Cannot AllocConsole(): " + get_last_error();
                message_box(s);
                exit(1);
            }
        }
        // if we've attached (got inherited streams), do not rebind std handles
        else if (flags & HANDLE_FLAG_INHERIT)
            return;
    }

    BindStdHandlesToConsole();
}

DWORD InstallService(ServiceDescriptor desc)
{
    //elevate();

    auto schSCManager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
    if (!schSCManager)
        return GetLastError();

    desc.module += L" " + to_wstring(desc.args);

    auto schService = CreateService(schSCManager,
        to_wstring(desc.name).c_str(), to_wstring(desc.name).c_str(),
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
        desc.module.wstring().c_str(),
        0, 0, 0, 0, 0);

    if (!schService)
    {
        auto e = GetLastError();
        CloseServiceHandle(schSCManager);
        return e;
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return 0;
}
#endif

bool create_link(const path &file, const path &link, const String &description)
{
#ifdef _WIN32
    fs::create_directories(link.parent_path());
    return SUCCEEDED(CreateLink(file.wstring().c_str(), link.wstring().c_str(), to_wstring(description).c_str()));
    //return SUCCEEDED(CreateLink(file.string().c_str(), link.string().c_str(), description.c_str()));
#else
    return true;
#endif
}

void elevate()
{
#ifdef _WIN32
    Elevate();
#endif
}

bool is_elevated()
{
#ifdef _WIN32
    return IsRunAsAdmin();
#endif
    return false;
}
