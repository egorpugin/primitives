// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/http.h>

#include <primitives/exceptions.h>

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

static auto curl_write_file(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    auto n = fwrite(ptr, size, nmemb, (FILE *)userdata);
    if (n != nmemb)
        perror("Cannot write to disk");
    return n;
}

static int curl_transfer_info(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    int64_t file_size_limit = *(int64_t*)clientp;
    if (dlnow > file_size_limit)
        return 1;
    return 0;
}

static size_t curl_write_string(char *ptr, size_t size, size_t nmemb, String *s)
{
    auto read = size * nmemb;
    try
    {
        s->append(ptr, ptr + read);
    }
    catch (...)
    {
        return 0;
    }
    return read;
}

struct CurlWrapper
{
    CURL *curl;
    struct curl_slist *headers = nullptr;
    std::string post_data;
    std::vector<char *> escaped_strings;

    CurlWrapper()
    {
        curl = curl_easy_init();
    }
    ~CurlWrapper()
    {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        for (auto &e : escaped_strings)
            curl_free(e);
    }
};

static std::unique_ptr<CurlWrapper> setup_curl_request(const HttpRequest &request)
{
    auto wp = std::make_unique<CurlWrapper>();
    auto &w = *wp;
    auto curl = w.curl;

    curl_easy_setopt(curl, CURLOPT_URL, request.url.c_str());

    if (request.verbose)
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

    if (!request.agent.empty())
        curl_easy_setopt(curl, CURLOPT_USERAGENT, request.agent.c_str());

    if (!request.username.empty())
        curl_easy_setopt(curl, CURLOPT_USERNAME, request.username.c_str());
    if (!request.password.empty())
        curl_easy_setopt(curl, CURLOPT_USERPWD, request.password.c_str());

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
    if (!httpSettings.proxy.host.empty())
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, httpSettings.proxy.host.c_str());
        curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
        if (httpSettings.proxy.host.find("socks5") == 0)
        {
            curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5);
            curl_easy_setopt(curl, CURLOPT_SOCKS5_AUTH, CURLAUTH_BASIC);
        }
        if (!httpSettings.proxy.user.empty())
            curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, httpSettings.proxy.user.c_str());
    }

    // ssl
#ifdef _WIN32
    if (!httpSettings.certs_file.empty())
        curl_easy_setopt(curl, CURLOPT_CAINFO, httpSettings.certs_file.c_str());
    else if (request.url.find("https") == 0)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2);
        if (request.ignore_ssl_checks)
        {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
            //curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 0);
        }
    }
#else
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2);
#endif

    // headers
    auto ct = "Content-Type: " + request.content_type;
    if (!request.content_type.empty())
    {
        w.headers = curl_slist_append(w.headers, ct.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, w.headers);

    //
    switch (request.type)
    {
    case HttpRequest::Post:
        if (!request.data_kv.empty())
        {
            for (auto &a : request.data_kv)
            {
                w.escaped_strings.push_back(curl_easy_escape(curl, a.first.c_str(), (int)a.first.size()));
                w.post_data += w.escaped_strings.back() + std::string("=");
                w.escaped_strings.push_back(curl_easy_escape(curl, a.second.c_str(), (int)a.second.size()));
                w.post_data += w.escaped_strings.back() + std::string("&");
            }
            w.post_data.resize(w.post_data.size() - 1);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, w.post_data.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)w.post_data.size());
        }
        else
        {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.data.c_str());
        }
        break;
    case HttpRequest::Delete:
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        break;
    }

    return wp;
}

void download_file(const String &url, const path &fn, int64_t file_size_limit)
{
    auto parent = fn.parent_path();
    if (!parent.empty() && !fs::exists(parent))
        fs::create_directories(parent);

    ScopedFile ofile(fn, "wb");

    HttpRequest req(httpSettings);
    req.url = url;
    auto w = setup_curl_request(req);
    auto curl = w->curl;

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_file); // must be set! see CURLOPT_WRITEDATA api desc
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, ofile.getHandle());
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curl_transfer_info);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &file_size_limit);

    auto res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // manual close
    ofile.close();

    auto on_error = [&fn]()
    {
        fs::remove(fn);
    };

    if (res == CURLE_ABORTED_BY_CALLBACK)
    {
        on_error();
        throw SW_RUNTIME_ERROR("File '" + url + "' is too big. Limit is " + std::to_string(file_size_limit) + " bytes.");
    }
    if (res != CURLE_OK)
    {
        on_error();
        throw SW_RUNTIME_ERROR("url = " + url + ", curl error: "s + curl_easy_strerror(res));
    }

    if (http_code / 100 != 2)
    {
        on_error();
        throw SW_RUNTIME_ERROR("url = " + url + ", http returned " + std::to_string(http_code));
    }
}

HttpResponse url_request(const HttpRequest &request)
{
    auto w = setup_curl_request(request);
    auto curl = w->curl;

    HttpResponse response;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_string);

    auto res = curl_easy_perform(curl);

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.http_code);

    if (res != CURLE_OK)
        throw SW_RUNTIME_ERROR("url = " + request.url + ", curl error: "s + curl_easy_strerror(res));

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
        // could be dangerous in case of vulnerabilities on client side? why?
        //s.find("ssh://") == 0 ||
        0
        )
    {
        return true;
    }
    return false;
}
