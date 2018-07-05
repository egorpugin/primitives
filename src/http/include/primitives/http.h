// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/constants.h>
#include <primitives/filesystem.h>

struct ProxySettings
{
    String host;
    String user;
};

PRIMITIVES_HTTP_API
String getAutoProxy();

struct HttpSettings
{
    bool verbose = false;
    bool ignore_ssl_checks = false;
    ProxySettings proxy;
};

extern PRIMITIVES_HTTP_API HttpSettings httpSettings;

struct HttpRequest : public HttpSettings
{
    enum Type
    {
        Get,
        Post,
        Delete
    };

    String url;
    String agent;
    String username;
    String password;
    int type = Get;
    String content_type;
    String data;
    std::unordered_map<String, String> data_kv;
    int timeout = -1;
    int connect_timeout = -1;

    HttpRequest(const HttpSettings &parent)
        : HttpSettings(parent)
    {}
};

struct HttpResponse
{
    long http_code = 0;
    String response;
};

PRIMITIVES_HTTP_API
HttpResponse url_request(const HttpRequest &settings);

PRIMITIVES_HTTP_API
void download_file(const String &url, const path &fn, int64_t file_size_limit = 1_MB);

PRIMITIVES_HTTP_API
String download_file(const String &url);

PRIMITIVES_HTTP_API
bool isUrl(const String &s);
