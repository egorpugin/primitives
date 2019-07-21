// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/win32helpers/service.h>

#include <primitives/exceptions.h>
#include <primitives/filesystem.h>

#include <windows.h>

Service::~Service()
{
}

void Service::reportError(const std::string &msg) const
{
    fprintf(stderr, "%s\n", msg.c_str());
}

struct ServiceDesc : Service
{
    ServiceDesc(Service &s) : s(s) {}

    std::string getName() const override { return s.getName(); }
    void start() override { s.start(); }
    void stop() override { s.stop(); }
    void reportError(const std::string &e) const override { s.reportError(e); }

    void registerService(SERVICE_STATUS_HANDLE h) { status_handle = h; }

    void setStatus(DWORD status)
    {
        SERVICE_STATUS status_ = { 0 };
        status_.dwServiceType = SERVICE_WIN32;
        status_.dwCurrentState = status;

        if (status == SERVICE_RUNNING)
        {
            status_.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_SESSIONCHANGE;
        }

        if (status != SERVICE_RUNNING && status != SERVICE_STOPPED)
            ++check_point;
        else
            check_point = 0;
        status_.dwCheckPoint = check_point;

        if (!SetServiceStatus(status_handle, &status_))
        {
            s.reportError("SetServiceStatus failed");
            return;
        }
    }

private:
    Service &s;
    SERVICE_STATUS_HANDLE status_handle = nullptr;
    DWORD check_point = 0;
};

static DWORD WINAPI serviceControlHandler(DWORD control_code, DWORD event_type, LPVOID event_data, LPVOID ctx)
{
    auto svc = (ServiceDesc*)ctx;
    switch (control_code)
    {
    case SERVICE_CONTROL_INTERROGATE:
        return NO_ERROR;
    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_STOP:
        if (control_code == SERVICE_CONTROL_STOP)
            svc->setStatus(SERVICE_STOP_PENDING);
        svc->stop();
        return NO_ERROR;
    default:
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

static std::atomic_size_t current_startup_service;
static std::vector<ServiceDesc> running_services;

static void WINAPI serviceMain(DWORD /* argc */, LPWSTR* /* argv */)
{
    auto instance = &running_services[current_startup_service++];

    auto sname = to_wstring(instance->getName());
    auto sh = RegisterServiceCtrlHandlerExW(sname.c_str(), serviceControlHandler, instance);
    instance->registerService(sh);

    instance->setStatus(SERVICE_RUNNING);

    try
    {
        instance->start();
    }
    catch (std::exception &e)
    {
        instance->reportError(e.what());
    }

    instance->setStatus(SERVICE_STOPPED);
}

void runServices(const std::vector<Service*> &services)
{
    auto sz = services.size();

    std::vector<std::wstring> service_names;
    service_names.reserve(sz);

    std::vector<SERVICE_TABLE_ENTRYW> service_table;
    service_table.reserve(sz + 1);

    for (size_t i = 0; i < sz; i++)
    {
        service_names.push_back(to_wstring(services[i]->getName()));
        service_table.emplace_back();
        service_table.back().lpServiceName = service_names.back().data();
        service_table.back().lpServiceProc = serviceMain;
        running_services.emplace_back(*services[i]);
    }
    // indicate end
    service_table.emplace_back();
    service_table.back().lpServiceName = nullptr;
    service_table.back().lpServiceProc = nullptr;

    if (!StartServiceCtrlDispatcherW(service_table.data()))
        throw SW_RUNTIME_ERROR("StartServiceCtrlDispatcherW failed");
}
