// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/win32helpers/service_controller.h>

#include <primitives/exceptions.h>
#include <primitives/filesystem.h>

struct ScopedScHandle
{
    SC_HANDLE h;

    ScopedScHandle(SC_HANDLE h) : h(h)
    {
    }

    ~ScopedScHandle()
    {
        CloseServiceHandle(h);
    }

    operator SC_HANDLE() const
    {
        return h;
    }

    explicit operator bool() const
    {
        return h;
    }
};

void installService(const std::string &name, const std::string &display_name, const std::string &command_line)
{
    ServiceController controller;
    if (controller.isInstalled(name))
        return;

    controller.install(name, display_name, command_line);

    printf("Service has been successfully installed. Starting...\n");
    if (!controller.start(name))
        throw SW_RUNTIME_ERROR("Service could not be started");

    printf("Done.\n");
}

void removeService(const std::string &name)
{
    ServiceController controller;
    if (controller.isRunning(name))
    {
        printf("Service is started. Stopping...\n");
        if (!controller.stop(name))
            throw SW_RUNTIME_ERROR("Service could not be stopped");
        printf("Done.\n");
    }

    printf("Remove the service...\n");
    if (!controller.remove(name))
        throw SW_RUNTIME_ERROR("Service could not be removed");

    printf("Done.\n");
}

ServiceController::ServiceController()
{
    manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!manager)
        throw SW_RUNTIME_ERROR("OpenSCManagerW failed");
}

ServiceController::~ServiceController()
{
    CloseServiceHandle(manager);
}

void ServiceController::install(const std::string& name, const std::string& display_name,
    const std::string& command_line)
{
    ScopedScHandle service_ =
        CreateServiceW(manager, to_wstring(name).c_str(), to_wstring(display_name).c_str(), SERVICE_ALL_ACCESS,
            SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
            to_wstring(command_line).c_str(), nullptr, nullptr, nullptr, nullptr, nullptr);

    if (!service_)
        throw SW_RUNTIME_ERROR("CreateServiceW failed");

    SC_ACTION action;
    action.Type = SC_ACTION_RESTART;
    action.Delay = 60000;

    SERVICE_FAILURE_ACTIONS actions;
    actions.dwResetPeriod = 0;
    actions.lpRebootMsg = nullptr;
    actions.lpCommand = nullptr;
    actions.cActions = 1;
    actions.lpsaActions = &action;

    if (!ChangeServiceConfig2W(service_, SERVICE_CONFIG_FAILURE_ACTIONS, &actions))
        throw SW_RUNTIME_ERROR("ChangeServiceConfig2W failed");
}

bool ServiceController::remove(const std::string& name)
{
    ScopedScHandle service_ = OpenServiceW(manager, to_wstring(name).c_str(), SERVICE_ALL_ACCESS);
    if (!service_)
        throw SW_RUNTIME_ERROR("OpenServiceW failed");

    if (!DeleteService(service_))
    {
        return false;
    }

    service_ = nullptr;

    static const int kMaxAttempts = 15;
    static const int kAttemptInterval = 100;

    for (int i = 0; i < kMaxAttempts; ++i)
    {
        if (!isInstalled(name))
            return true;

        Sleep(kAttemptInterval);
    }

    return false;
}

bool ServiceController::isInstalled(const std::string& name)
{
    ScopedScHandle service_ = OpenServiceW(manager, to_wstring(name).c_str(), SERVICE_QUERY_CONFIG);

    if (!service_)
    {
        if (GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST)
        {
            throw SW_RUNTIME_ERROR("OpenServiceW failed");
        }
        return false;
    }
    return true;
}

bool ServiceController::isRunning(const std::string& name) const
{
    ScopedScHandle service_ = OpenServiceW(manager, to_wstring(name).c_str(), SERVICE_QUERY_STATUS);

    SERVICE_STATUS status;
    if (!QueryServiceStatus(service_, &status))
    {
        return false;
    }
    return status.dwCurrentState != SERVICE_STOPPED;
}

bool ServiceController::start(const std::string& name)
{
    ScopedScHandle service_ = OpenServiceW(manager, to_wstring(name).c_str(), SERVICE_ALL_ACCESS);
    if (!service_)
        throw SW_RUNTIME_ERROR("OpenServiceW failed");
    if (!StartServiceW(service_, 0, nullptr))
    {
        return false;
    }
    return true;
}

bool ServiceController::stop(const std::string& name)
{
    ScopedScHandle service_ = OpenServiceW(manager, to_wstring(name).c_str(), SERVICE_ALL_ACCESS);
    if (!service_)
        throw SW_RUNTIME_ERROR("OpenServiceW failed");

    SERVICE_STATUS status;
    if (!ControlService(service_, SERVICE_CONTROL_STOP, &status))
    {
        return false;
    }

    bool is_stopped = status.dwCurrentState == SERVICE_STOPPED;
    int number_of_attempts = 0;

    while (!is_stopped && number_of_attempts < 15)
    {
        Sleep(250);

        if (!QueryServiceStatus(service_, &status))
            break;

        is_stopped = status.dwCurrentState == SERVICE_STOPPED;
        ++number_of_attempts;
    }

    return is_stopped;
}
