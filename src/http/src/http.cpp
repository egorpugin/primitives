// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/http.h>

#include <primitives/exceptions.h>

#ifdef _WIN32
#include <windows.h>
#include <Winhttp.h>
#endif

HttpSettings httpSettings;

namespace primitives::http
{

static void setup_curl_ssl(CURL *curl, const HttpRequest &request);

}

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
    curl_mime *form = nullptr;
    struct curl_slist *headers = nullptr;
    std::string post_data;
    std::vector<char *> escaped_strings;
    //path_u8string ca_certs_file;
    //path_u8string ca_certs_dir;
    FILE *upload_file{nullptr};

    CurlWrapper()
    {
        curl = curl_easy_init();
    }
    CurlWrapper(CurlWrapper &&rhs) = delete;
    /*{
        curl = rhs.curl; rhs.curl = nullptr;
        headers = rhs.headers; rhs.headers = nullptr;
        post_data = std::move(rhs.post_data);
        escaped_strings = std::move(rhs.escaped_strings);
    }*/
    ~CurlWrapper()
    {
        if (form)
            curl_mime_free(form);
        curl_slist_free_all(headers);
        for (auto &e : escaped_strings)
            curl_free(e);
        curl_easy_cleanup(curl);

        if (upload_file)
            fclose(upload_file);
    }
};

static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
  return fread(ptr, size, nmemb, (FILE *)userdata);
}
static int seek_callback(void *userp, curl_off_t offset, int origin) {
    int a = 5;
    a++;
    return CURL_SEEKFUNC_OK;
}
static curlioerr ioctl_callback(CURL *handle, int cmd, void *clientp) {
    int a = 5;
    a++;
    return CURLIOE_OK;
}

static std::unique_ptr<CurlWrapper> setup_curl_request(const HttpRequest &request)
{
    auto wp = std::make_unique<CurlWrapper>();
    auto &w = *wp;
    auto curl = w.curl;

    /*curl_easy_setopt(curl, CURLOPT_SEEKFUNCTION, seek_callback);
    curl_easy_setopt(curl, CURLOPT_SEEKDATA, 0);
    curl_easy_setopt(curl, CURLOPT_IOCTLFUNCTION, ioctl_callback);
    curl_easy_setopt(curl, CURLOPT_IOCTLDATA, 0);*/

    curl_easy_setopt(curl, CURLOPT_URL, request.url.c_str());

    if (!request.cookie.empty())
        curl_easy_setopt(curl, CURLOPT_COOKIE, request.cookie.c_str());
    // enable the cookie engine
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");

    if (request.verbose)
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    if (!request.agent.empty())
        curl_easy_setopt(curl, CURLOPT_USERAGENT, request.agent.c_str());

    if (!request.username.empty())
        curl_easy_setopt(curl, CURLOPT_USERNAME, request.username.c_str());
    if (!request.password.empty())
        curl_easy_setopt(curl, CURLOPT_PASSWORD, request.password.c_str());

    if (request.follow_redirects)
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    if (request.connect_timeout != -1)
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)request.connect_timeout);
    if (request.timeout != -1)
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)request.timeout);

    if (!request.upload_file.empty()) {
        wp->upload_file = primitives::filesystem::fopen(request.upload_file);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        auto sz = (curl_off_t)fs::file_size(request.upload_file);
        if (sz < 2 * (1LL << 30)) {
            //curl_easy_setopt(curl, CURLOPT_INFILESIZE, sz);
        } else {
            //curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, sz);
        }
        curl_easy_setopt(curl, CURLOPT_READDATA, wp->upload_file);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
    }

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
    primitives::http::setup_curl_ssl(curl, request);

    // headers
    auto ct = "Content-Type: " + request.content_type;
    if (!request.content_type.empty()) {
        w.headers = curl_slist_append(w.headers, ct.c_str());
    }
    for (auto &&h : request.headers) {
        w.headers = curl_slist_append(w.headers, h.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, w.headers);

    //
    switch (request.type)
    {
    case HttpRequest::Put:
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    case HttpRequest::Post:
        if (!request.data_kv.empty()) {
            for (auto &a : request.data_kv) {
                w.escaped_strings.push_back(curl_easy_escape(curl, a.first.c_str(), (int)a.first.size()));
                w.post_data += w.escaped_strings.back() + std::string("=");
                w.escaped_strings.push_back(curl_easy_escape(curl, a.second.c_str(), (int)a.second.size()));
                w.post_data += w.escaped_strings.back() + std::string("&");
            }
            w.post_data.resize(w.post_data.size() - 1);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, w.post_data.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)w.post_data.size());
        } else if (!request.form_data.empty() || !request.form_data_vec.empty()) {
            wp->form = curl_mime_init(curl);
            auto add = [&](auto &&v) {
                for (auto &&[n,v] : v) {
                    auto field = curl_mime_addpart(wp->form);
                    curl_mime_name(field, n.c_str());
                    if (!v.filename.empty())
                        curl_mime_filedata(field, v.filename.c_str());
                    else if (!v.type.empty())
                        curl_mime_type(field, v.type.c_str());
                    else
                        curl_mime_data(field, v.data.c_str(), CURL_ZERO_TERMINATED);
                }
            };
            add(request.form_data);
            add(request.form_data_vec);
            curl_easy_setopt(curl, CURLOPT_MIMEPOST, wp->form);
        } else {
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
    auto tmpfn = path{fn} += ".dl";
    auto [w,ofile] = setup_download_request(url, tmpfn, file_size_limit);
    auto curl = w->curl;

    auto res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // manual close
    ofile.close();

    auto on_error = [&]()
    {
        fs::remove(tmpfn);
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

    fs::rename(tmpfn, fn);
}

HttpResponse url_request(const HttpRequest &request)
{
    auto w = setup_curl_request(request);
    auto curl = w->curl;

    HttpResponse response;
    std::swap(response.curl, w->curl);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_string);

    auto res = curl_easy_perform(curl);

    if (res == CURLE_PROXY) {
        long proxycode;
        res = curl_easy_getinfo(curl, CURLINFO_PROXY_ERROR, &proxycode);
        if (!res && proxycode) {
            throw primitives::http::curl_exception{
                "url = " + request.url +
                ", curl error: "s + curl_easy_strerror(res) +
                ", proxy error: "s + std::to_string(proxycode) };
        }
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.http_code);

    if (res != CURLE_OK) {
        throw primitives::http::curl_exception{ "url = " + request.url + ", curl error: "s + curl_easy_strerror(res) };
    }

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
        curlGlobalSslSet(CURLSSLBACKEND_OPENSSL);
        //false
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
    if (url.empty())
        url = getDefaultCaBundleUrl();

    auto proper_download_file = [](auto &url, auto &ca_certs_fn)
    {
        /*ScopedFileLock lk(ca_certs_fn, std::defer_lock);
        if (!lk.try_lock())
        {
            lk.lock();
            // someone else downloaded file?
            if (fs::exists(ca_certs_fn))
                return;
        }
        bool e = fs::exists(ca_certs_fn);
        try
        {*/
            auto dlpath = path(ca_certs_fn) += ".dl";
            download_file(url, dlpath);
            fs::copy_file(dlpath, ca_certs_fn, fs::copy_options::overwrite_existing);
        /*}
        catch (std::exception &)
        {
            // internet error
            if (e)
                return;
            throw;
        }*/
    };

    // by default we want to trust remote certs
    // but someone may replace our certs file
    //
    // if we use native tls on win/mac, its certs may be outdated (win especially)
    //
    // use first method for now

    // if -k set, ignore everything
    if (httpSettings.ignore_ssl_checks)
    {
        proper_download_file(url, ca_certs_fn);
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

#ifdef _WIN32
    // just simple download on windows + openssl + windows ca store
    try
    {
        decltype(httpSettings.ca_certs_file) empty;
        SwapAndRestore sr(httpSettings.ca_certs_file, empty);
        proper_download_file(url, ca_certs_fn);
        httpSettings.ca_certs_file = normalize_path(ca_certs_fn);
        return;
    }
    catch (std::exception &)
    {
        // ignore
    }
#endif

    if (strict)
        throw SW_RUNTIME_ERROR("Cannot set TLS settings.");

    // just simple download
    SwapAndRestore sr(httpSettings.ignore_ssl_checks, true);
    proper_download_file(url, ca_certs_fn);
    httpSettings.ca_certs_file = normalize_path(ca_certs_fn);
}

void setupSafeTls()
{
    setupSafeTls(true, true, {});
}

static void setup_curl_ssl(CURL *curl, const HttpRequest &request)
{
    auto ca_certs_file = to_path_string(normalize_path(request.ca_certs_file));
    auto ca_certs_dir = to_path_string(normalize_path(request.ca_certs_dir));

    if (!ca_certs_file.empty())
        curl_easy_setopt(curl, CURLOPT_CAINFO, ca_certs_file.c_str());
#ifdef _WIN32
    else
    {
        // by default on windows we use native ca store since curl 7.71.0
        curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
    }
#else
    // capath does not work on windows
    // https://curl.haxx.se/libcurl/c/CURLOPT_CAPATH.html
    else if (!ca_certs_dir.empty())
        curl_easy_setopt(curl, CURLOPT_CAPATH, ca_certs_dir.c_str());
#endif

    if (request.ignore_ssl_checks)
    {
        // we always verify host
        // we disable only peer check
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    }
}

void setup_curl_ssl(void *in)
{
    auto curl = (CURL *)in;
    setup_curl_ssl(curl, httpSettings);
}

static String charToHex(char c) {
    std::string result;
    char first, second;

    first = (c & 0xF0) / 16;
    first += first > 9 ? 'A' - 10 : '0';
    second = c & 0x0F;
    second += second > 9 ? 'A' - 10 : '0';

    result.append(1, first);
    result.append(1, second);

    return result;
}

String url_encode(const String &src)
{
    std::string result;
    for (auto &c : src)
    {
        if (c > 0 && c < 128 && isalnum(c))
        {
            result.append(1, c);
            continue;
        }
        switch (c)
        {
        case ' ':
            result.append(1, '+');
            break;
        case '-': case '_': case '.':
        //case '!':
        case '~':
        //case '*':
        //case '\'':
        //case '(': case ')':
            result.append(1, c);
            break;
            // escape
        default:
            result.append(1, '%');
            result.append(charToHex(c));
            break;
        }
    }
    return result;
}

static char hexToChar(char first, char second)
{
    int digit;

    digit = (first >= 'A' ? ((first & 0xDF) - 'A') + 10 : (first - '0'));
    digit *= 16;
    digit += (second >= 'A' ? ((second & 0xDF) - 'A') + 10 : (second - '0'));
    return static_cast<char>(digit);
}

String url_decode(const String &src)
{
    std::string result;
    char c;
    for (auto iter = src.begin(); iter != src.end(); ++iter)
    {
        switch (*iter)
        {
        case '+':
            result.append(1, ' ');
            break;
        case '%':
            // Don't assume well-formed input
            if (std::distance(iter, src.end()) >= 2
                && std::isxdigit(*(iter + 1)) && std::isxdigit(*(iter + 2)))
            {
                c = *++iter;
                result.append(1, hexToChar(c, *++iter));
            }
            // Just pass the % through untouched
            else
                result.append(1, '%');
            break;
        default:
            result.append(1, *iter);
            break;
        }
    }
    return result;
}

}
