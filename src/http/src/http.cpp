// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/http.h>

#include <curl/curl.h>

#ifdef _WIN32
#include <windows.h>
#include <Winhttp.h>
#endif

HttpSettings httpSettings;

String getAutoProxy()
{
    String proxy_addr;
    std::wstring wproxy_addr;
#ifdef _WIN32
    WINHTTP_PROXY_INFO proxy = { 0 };
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxy2 = { 0 };
    if (WinHttpGetDefaultProxyConfiguration(&proxy) && proxy.lpszProxy)
        wproxy_addr = proxy.lpszProxy;
    else if (WinHttpGetIEProxyConfigForCurrentUser(&proxy2) && proxy2.lpszProxy)
        wproxy_addr = proxy2.lpszProxy;
    proxy_addr = to_string(wproxy_addr);
#endif
    return proxy_addr;
}

size_t curl_write_file(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    auto read = size * nmemb;
    fwrite(ptr, read, 1, (FILE *)userdata);
    return read;
}

int curl_transfer_info(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    int64_t file_size_limit = *(int64_t*)clientp;
    if (dlnow > file_size_limit)
        return 1;
    return 0;
}

size_t curl_write_string(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    String &s = *(String *)userdata;
    auto read = size * nmemb;
    s.append(ptr, ptr + read);
    return read;
}

void download_file(const String &url, const path &fn, int64_t file_size_limit)
{
    auto parent = fn.parent_path();
    if (!parent.empty() && !fs::exists(parent))
        fs::create_directories(parent);

    ScopedFile ofile(fn, "wb");

    // set up curl request
    auto curl = curl_easy_init();

    if (httpSettings.verbose)
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

    // proxy settings
    auto proxy_addr = getAutoProxy();
    if (!proxy_addr.empty())
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxy_addr.c_str());
        curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
    }
    if (!httpSettings.proxy.host.empty())
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, httpSettings.proxy.host.c_str());
        curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
        if (!httpSettings.proxy.user.empty())
            curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, httpSettings.proxy.user.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, ofile.getHandle());
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curl_transfer_info);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &file_size_limit);
    if (url.find("https") == 0)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2);
        if (httpSettings.ignore_ssl_checks)
        {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
            //curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 0);
        }
    }

    auto res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (res == CURLE_ABORTED_BY_CALLBACK)
    {
        fs::remove(fn);
        throw std::runtime_error("File '" + url + "' is too big. Limit is " + std::to_string(file_size_limit) + " bytes.");
    }
    if (res != CURLE_OK)
        throw std::runtime_error("curl error: "s + curl_easy_strerror(res));

    if (http_code / 100 != 2)
        throw std::runtime_error("Http returned " + std::to_string(http_code));
}

HttpResponse url_request(const HttpRequest &request)
{
    auto curl = curl_easy_init();

    if (request.verbose)
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

    if (!request.agent.empty())
        curl_easy_setopt(curl, CURLOPT_USERAGENT, request.agent.c_str());
    if (!request.username.empty())
        curl_easy_setopt(curl, CURLOPT_USERNAME, request.username.c_str());
    if (!request.password.empty())
        curl_easy_setopt(curl, CURLOPT_USERPWD, request.password.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, request.url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    if (request.connect_timeout != -1)
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, request.connect_timeout);
    if (request.timeout != -1)
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, request.timeout);

    // proxy settings
    auto proxy_addr = getAutoProxy();
    if (!proxy_addr.empty())
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxy_addr.c_str());
        curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
    }
    if (!request.proxy.host.empty())
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, request.proxy.host.c_str());
        curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
        if (!request.proxy.user.empty())
            curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, request.proxy.user.c_str());
    }

    switch (request.type)
    {
    case HttpRequest::Post:
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.data.c_str());
        break;
    case HttpRequest::Delete:
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        break;
    }

    if (request.url.find("https") == 0)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2);
        if (request.ignore_ssl_checks)
        {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
            //curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 0);
        }
    }

    HttpResponse response;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_string);

    struct curl_slist *list = NULL;

    auto ct = "Content-Type: " + request.content_type;
    if (!request.content_type.empty())
    {
        list = curl_slist_append(list, ct.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    auto res = curl_easy_perform(curl);

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.http_code);
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        throw std::runtime_error("curl error: "s + curl_easy_strerror(res));

    return response;
}


String download_file(const String &url)
{
    auto fn = fs::temp_directory_path() / unique_path();
    download_file(url, fn, 1_GB);
    auto s = read_file(fn);
    fs::remove(fn);
    return s;
}

bool isUrl(const String &s)
{
    if (s.find("http://") == 0 ||
        s.find("https://") == 0 ||
        s.find("ftp://") == 0 ||
        s.find("git://") == 0 ||
        // could be dangerous in case of vulnerabilities on client side?
        //s.find("ssh://") == 0 ||
        0
        )
    {
        return true;
    }
    return false;
}
