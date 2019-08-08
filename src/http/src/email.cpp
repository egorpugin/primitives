#include <primitives/email.h>

#include <curl/curl.h>

#include <string.h>

namespace primitives
{

struct upload_status
{
    int lines_read = 0;
    Strings payload;
};

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
    upload_status *upload_ctx = (upload_status *)userp;

    if (size == 0 || nmemb == 0)
        return 0;

    if (upload_ctx->lines_read >= upload_ctx->payload.size())
        return 0;

    auto &s = upload_ctx->payload[upload_ctx->lines_read];
    memcpy(ptr, s.data(), s.size());
    upload_ctx->lines_read++;

    return s.size();
}

String Smtp::sendEmail(const Email &e) const
{
    CURL *curl;
    CURLcode res = CURLE_OK;
    struct curl_slist *recipients = NULL;

    curl = curl_easy_init();
    if (!curl)
        return {};

    curl_easy_setopt(curl, CURLOPT_URL, server.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, e.from.email.c_str());

    recipients = curl_slist_append(recipients, e.to.email.c_str());
    for (auto &s : e.cc)
        recipients = curl_slist_append(recipients, s.email.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);

    upload_status uctx;
    if (!e.custom_email)
    {
        uctx.payload.push_back("To: " + e.to.name + " <" + e.to.email + ">" + "\r\n");
        uctx.payload.push_back("From: " + e.from.name + " <" + e.from.email + ">" + "\r\n");
        for (auto &s : e.cc)
            uctx.payload.push_back("Cc: " + s.name + " <" + s.email + ">" + "\r\n");
        uctx.payload.push_back("Subject: " + e.title + "\r\n");
        uctx.payload.push_back("\r\n");
    }
    uctx.payload.push_back(e.body);

    curl_easy_setopt(curl, CURLOPT_READDATA, &uctx);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    auto pwd = user + ":" + password;
    curl_easy_setopt(curl, CURLOPT_USERNAME, pwd.c_str()); // must go twice!
    curl_easy_setopt(curl, CURLOPT_USERPWD, pwd.c_str());
    curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, auth.c_str());

    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)ssl);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    //curl_easy_setopt(curl, CURLOPT_CAINFO, "certs/roots.pem");

    curl_easy_setopt(curl, CURLOPT_VERBOSE, verbose);

    res = curl_easy_perform(curl);

    String error;
    if (res != CURLE_OK)
        error = curl_easy_strerror(res);

    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);

    return error;
}

void Smtp::setAuth(const String &s)
{
    auto p = s.find(':');
    user = s.substr(0, p);
    password = s.substr(p + 1);
}

}
