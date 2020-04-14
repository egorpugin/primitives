// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/constants.h>
#include <primitives/filesystem.h>

#include <optional>

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
    String ca_certs_file;
    String ca_certs_dir;
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

/// checks if file exists, then reads it, otherwise downloads it
PRIMITIVES_HTTP_API
String read_file_or_download_file(const String &target);

PRIMITIVES_HTTP_API
bool isUrl(const String &s);

namespace primitives::http
{

// CA Certificates Methods

PRIMITIVES_HTTP_API
String getDefaultCaBundleUrl();

PRIMITIVES_HTTP_API
bool curlGlobalSslSet(int ssl_id);

// returns true if set
PRIMITIVES_HTTP_API
bool curlUseNativeTls();

PRIMITIVES_HTTP_API
std::optional<path> getCaCertificatesBundleFileName();

PRIMITIVES_HTTP_API
std::optional<path> getCaCertificatesDirectory();

PRIMITIVES_HTTP_API
bool downloadOrUpdateCaCertificatesBundle(const path &destfn, String url = {});

// must be called before other curl commands
PRIMITIVES_HTTP_API
bool safelyDownloadCaCertificatesBundle(const path &destfn, String url = {});

PRIMITIVES_HTTP_API
void setupSafeTls(bool prefer_native, bool strict, const path &ca_certs_fn, String url = {});

}
