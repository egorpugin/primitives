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
    CurlWrapper(CurlWrapper &&rhs)
    {
        curl = rhs.curl; rhs.curl = nullptr;
        headers = rhs.headers; rhs.headers = nullptr;
        post_data = std::move(rhs.post_data);
        escaped_strings = std::move(rhs.escaped_strings);
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
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    if (!request.agent.empty())
        curl_easy_setopt(curl, CURLOPT_USERAGENT, request.agent.c_str());

    if (!request.username.empty())
        curl_easy_setopt(curl, CURLOPT_USERNAME, request.username.c_str());
    if (!request.password.empty())
        curl_easy_setopt(curl, CURLOPT_USERPWD, request.password.c_str());

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    if (request.connect_timeout != -1)
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)request.connect_timeout);
    if (request.timeout != -1)
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)request.timeout);

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
            // also reset back to basic, helps in connection reuse if any
            curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_BASIC);
        }
        if (!httpSettings.proxy.user.empty())
            curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, httpSettings.proxy.user.c_str());
    }

    // ssl
    if (!httpSettings.ca_certs_file.empty())
        curl_easy_setopt(curl, CURLOPT_CAINFO, httpSettings.ca_certs_file.c_str());
#ifndef _WIN32
    // capath does not work on windows
    // https://curl.haxx.se/libcurl/c/CURLOPT_CAPATH.html
    else if (!httpSettings.ca_certs_dir.empty())
        curl_easy_setopt(curl, CURLOPT_CAPATH, httpSettings.ca_certs_dir.c_str());
#endif

    if (request.ignore_ssl_checks)
    {
        // we always verify host
        // we disable only peer check
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    }

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

static std::tuple<std::unique_ptr<CurlWrapper>, ScopedFile> setup_download_request(
    const String &url,
    const path &fn,
    int64_t &file_size_limit // reference!!! to pass pointer into function
)
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

    return { std::move(w), std::move(ofile) };
}

void download_file(const String &url, const path &fn, int64_t file_size_limit)
{
    auto [w,ofile] = setup_download_request(url, fn, file_size_limit);
    auto curl = w->curl;

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

String read_file_or_download_file(const String &target)
{
    if (fs::exists(target))
        return read_file(target);
    return download_file(target);
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

namespace primitives::http
{

std::optional<path> getCaCertificatesBundleFileName()
{
    // for CURLOPT_CAINFO

#ifdef _WIN32
    return {};
#endif

    const char *files[] =
    {
        "/etc/ssl/certs/ca-certificates.crt",
        "/etc/ssl/certs/ca-bundle.crt",
    };

    for (auto f : files)
    {
        if (fs::exists(f))
            return f;
    }
    return {};
}

std::optional<path> getCaCertificatesDirectory()
{
    // for CURLOPT_CAPATH

#ifdef _WIN32
    return {};
#endif

    return "/etc/ssl/certs";
}

String getDefaultCaBundleUrl()
{
    return "https://curl.haxx.se/ca/cacert.pem";
}

bool curlGlobalSslSet(int ssl_id)
{
    auto code = curl_global_sslset((curl_sslbackend)ssl_id, 0, 0);
    return code == CURLSSLSET_OK;
}

bool curlUseNativeTls()
{
    bool ok =
#if defined(_WIN32)
        curlGlobalSslSet(CURLSSLBACKEND_SCHANNEL)
#elif defined(__APPLE__)
        curlGlobalSslSet(CURLSSLBACKEND_SECURETRANSPORT)
#else
        false
#endif
        ;
    if (ok)
    {
        // native cannot be used with provided certs
        httpSettings.ca_certs_file.clear();
        httpSettings.ca_certs_dir.clear();
    }
    return ok;
}

bool downloadOrUpdateCaCertificatesBundle(const path &destfn, String url)
{
    if (url.empty())
        url = getDefaultCaBundleUrl();
    auto fn = path(destfn) += ".tmp";
    int64_t file_size_limit = 1_MB;
    bool old_file_exists = fs::exists(destfn);

    auto [w, ofile] = setup_download_request(url, fn, file_size_limit);
    auto curl = w->curl;

    //
    time_t lwt = 0;
    if (old_file_exists)
    {
#ifdef _WIN32
        struct _stat64 fileInfo;
        if (_wstati64(destfn.wstring().c_str(), &fileInfo) != 0)
            throw SW_RUNTIME_ERROR("Failed to get last write time.");
        lwt = fileInfo.st_mtime;
#else
        auto lwt2 = fs::last_write_time(destfn);
        lwt = file_time_type2time_t(lwt2);
#endif
    }
    curl_easy_setopt(curl, CURLOPT_TIMEVALUE_LARGE, (curl_off_t)lwt);
    curl_easy_setopt(curl, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
    //

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
        //if (old_file_exists)
        return false; // return silently
    //throw SW_RUNTIME_ERROR("File '" + url + "' is too big. Limit is " + std::to_string(file_size_limit) + " bytes.");
    }
    if (res != CURLE_OK)
    {
        on_error();
        //if (old_file_exists)
        return false; // return silently
    //throw SW_RUNTIME_ERROR("url = " + url + ", curl error: "s + curl_easy_strerror(res));
    }

    if (http_code / 100 != 2)
    {
        on_error();
        if (http_code == 304) // 304 Not Modified
            return true;
        //if (old_file_exists)
        return false; // return silently
    //throw SW_RUNTIME_ERROR("url = " + url + ", http returned " + std::to_string(http_code));
    }

    // ok, now rename from .tmp
    fs::rename(fn, destfn);
    return true;
}

bool safelyDownloadCaCertificatesBundle(const path &destfn, String url)
{
    if (url.empty())
        url = getDefaultCaBundleUrl();

    auto call = [&destfn, &url]()
    {
        return downloadOrUpdateCaCertificatesBundle(destfn, url);
    };

    // use dest itself, can we replace it during operation?
    if (fs::exists(destfn))
    {
        auto s = normalize_path(destfn);
        SwapAndRestore sr(httpSettings.ca_certs_file, s);
        if (call())
            return true;
    }

    // method for win/macos
    if (curlUseNativeTls())
    {
        if (call())
            return true;
    }

    // now *nix systems

    // ca bundle
    if (auto f = getCaCertificatesBundleFileName())
    {
        auto s = normalize_path(*f);
        SwapAndRestore sr(httpSettings.ca_certs_file, s);
        if (call())
            return true;
    }

    // and ca certs dir
    if (auto d = getCaCertificatesDirectory())
    {
        auto s = normalize_path(*d);
        SwapAndRestore sr(httpSettings.ca_certs_dir, s);
        if (call())
            return true;
    }

    //throw SW_RUNTIME_ERROR("Cannot download ca bundle safely");
    return false;
}

void setupSafeTls(bool prefer_native, bool strict, const path &ca_certs_fn, String url)
{
    // by default we want to trust remote certs
    // but someone may replace our certs file
    //
    // if we use native tls on win/mac, its certs may be outdated (win especially)
    //
    // use first method for now

    // if -k set, ignore everything
    if (httpSettings.ignore_ssl_checks)
    {
        download_file(url, ca_certs_fn);
        httpSettings.ca_certs_file = normalize_path(ca_certs_fn);
        return;
    }

    // non-builtin tls impls support ca cert files
    if (!prefer_native && fs::exists(ca_certs_fn))
        httpSettings.ca_certs_file = normalize_path(ca_certs_fn);

    if (prefer_native)
    {
        // win, mac
        if (curlUseNativeTls())
            return;

        // *nix

        if (auto f = getCaCertificatesBundleFileName())
        {
            auto s = normalize_path(*f);
            httpSettings.ca_certs_file = s;
            return;
        }

        // and ca certs dir
        if (auto d = getCaCertificatesDirectory())
        {
            auto s = normalize_path(*d);
            httpSettings.ca_certs_dir = s;
            return;
        }

        // no success :(

        if (strict)
            throw SW_RUNTIME_ERROR("Cannot set native TLS settings.");
    }


    if (safelyDownloadCaCertificatesBundle(ca_certs_fn, url))
    {
        httpSettings.ca_certs_file = normalize_path(ca_certs_fn);
        return;
    }

    if (strict)
        throw SW_RUNTIME_ERROR("Cannot set TLS settings.");

    // just simple download
    SwapAndRestore sr(httpSettings.ignore_ssl_checks, true);
    download_file(url, ca_certs_fn);
    httpSettings.ca_certs_file = normalize_path(ca_certs_fn);
}

}
