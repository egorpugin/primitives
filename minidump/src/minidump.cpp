#include <primitives/minidump.h>

#ifdef _WIN32
#include <Windows.h>
#include <dbghelp.h>
#include <shellapi.h>
#include <shlobj.h>
#include <Strsafe.h>
#include <tchar.h>

namespace primitives::minidump
{

std::wstring dir = L"primitives\\dump";
int v_major;
int v_minor;
int v_patch;

int GenerateDump(_EXCEPTION_POINTERS* pExceptionPointers)
{
    // we allow further program debugging
    if (IsDebuggerPresent())
        return EXCEPTION_CONTINUE_SEARCH;

    _tprintf(_T("Fatal error. Creating minidump.\n"));

    BOOL bMiniDumpSuccessful;
    WCHAR szPath[MAX_PATH];
    WCHAR szFileName[MAX_PATH];
    const WCHAR* szAppName = dir.c_str();
    DWORD dwBufferSize = MAX_PATH;
    HANDLE hDumpFile;
    SYSTEMTIME stLocalTime;
    MINIDUMP_EXCEPTION_INFORMATION ExpParam;

    GetLocalTime(&stLocalTime);
    GetTempPathW(dwBufferSize, szPath);

    StringCchPrintfW(szFileName, MAX_PATH, L"%s%s", szPath, szAppName);
    CreateDirectoryW(szFileName, NULL);

    if (v_major == 0 && v_minor == 0 && v_patch == 0)
        StringCchPrintfW(szFileName, MAX_PATH, L"%s%s\\%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp",
            szPath, szAppName,
            stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
            stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
            GetCurrentProcessId(), GetCurrentThreadId());
    else
        StringCchPrintfW(szFileName, MAX_PATH, L"%s%s\\%d.%d.%d-%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp",
            szPath, szAppName, v_major, v_minor, v_patch,
            stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
            stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
            GetCurrentProcessId(), GetCurrentThreadId());


    hDumpFile = CreateFileW(szFileName, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

    ExpParam.ThreadId = GetCurrentThreadId();
    ExpParam.ExceptionPointers = pExceptionPointers;
    ExpParam.ClientPointers = TRUE;

    bMiniDumpSuccessful = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
        hDumpFile, MiniDumpWithDataSegs, &ExpParam, NULL, NULL);

    if (!bMiniDumpSuccessful)
        _tprintf(_T("MiniDumpWriteDump failed. Error: %lu \n"), GetLastError());
    else
        _tprintf(_T("Minidump created: %ws\n"), szFileName);

    CloseHandle(hDumpFile);

    //std::exit(1);
    //return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

}

#endif
