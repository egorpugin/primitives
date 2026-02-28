/*
c++: 23
deps:
    #- org.sw.demo.badger.curl.libcurl
    - pub.egorpugin.primitives.http
    - org.sw.demo.nlohmann.json
*/

#include <nlohmann/json.hpp>
#include <primitives/templates2/timeout.h>
#include <primitives/http.h>
#include <primitives/exceptions.h>

#include <algorithm>
#include <format>
#include <chrono>
#include <thread>

namespace primitives::webdriver {

namespace keys {
    constexpr inline auto tab = "\xee\x80\x84";
    constexpr inline auto enter = "\xee\x80\x87";
}

struct no_such_element_exception : ::std::runtime_error { using ::std::runtime_error::runtime_error; };
struct element_not_interactable_exception : ::std::runtime_error { using ::std::runtime_error::runtime_error; };
struct javascript_error_exception : ::std::runtime_error { using ::std::runtime_error::runtime_error; };
struct stale_element_reference_exception : ::std::runtime_error { using ::std::runtime_error::runtime_error; };
struct unexpected_alert_open_exception : ::std::runtime_error { using ::std::runtime_error::runtime_error; };
struct no_such_alert_exception : ::std::runtime_error { using ::std::runtime_error::runtime_error; };
struct timeout_exception : ::std::runtime_error { using ::std::runtime_error::runtime_error; };
struct invalid_argument : ::std::runtime_error { using ::std::runtime_error::runtime_error; };
struct session_not_created_exception : ::std::runtime_error {
    using ::std::runtime_error::runtime_error;
    std::string actual_chrome_version;
};

struct by {
    std::string strategy;
    std::string value;

    static auto css(auto&& v) { return by{ "css selector", v }; }
    static auto class_(auto&& v) { return css("."s + v); }
    static auto id(auto &&v) { return css("*[id=\""s + v + "\"]"); }
    static auto name(auto &&v) { return css("*[name=\""s + v + "\"]"); }
    //static auto value(auto &&v) { return css("*[value=\""s + v + "\"]"); }
    static auto xpath(auto&& v) { return by{ "xpath", v }; }

    /*
By ByLinkText(const std::string& value) {
	WEBDRIVERXX_ISEMPTY_THROW(value, "Cannot find elements when link text is null.");
	return By("link text", value);
}
By ByPartialLinkText(const std::string& value) {
	WEBDRIVERXX_ISEMPTY_THROW(value, "Cannot find elements when partial link text is null.");
	return By("partial link text", value);
}
By ByLink(const std::string& value) {
	WEBDRIVERXX_ISEMPTY_THROW(value, "Cannot find elements when href link is null.");
	const std::string selector = "*[href=\"" + value + "\"]";
	return ByCss(selector);
}
By ByPartialLink(const std::string& value) {
	WEBDRIVERXX_ISEMPTY_THROW(value, "Cannot find elements when partial href link is null.");
	const std::string selector = "*[href*=\"" + value + "\"]";
	return ByCss(selector);
}
By ByTag(const std::string& value) {
	WEBDRIVERXX_ISEMPTY_THROW(value, "Cannot find elements when name tag name is null.");
	return ByCss(value);
}

*/
};

template <typename Webdriver>
struct element {
    Webdriver *d_{nullptr};
    std::string ref;
    std::string element_; // msvc bug if rename to 'element'

    explicit operator bool() const { return !!d_; }
    auto& d() { return *d_; }
    auto make_session_url(auto &&u) {
        return d().make_session_url() / "element" / element_ / u;
    }
    template <auto v = HttpRequest::Get>
    auto create_req(auto &&u) {
        return d().template create_req<v>(make_session_url(u));
    }
    decltype(auto) send_keys(this auto &&self, const std::string &s) {
        if (s.empty()) {
            return std::forward<decltype(self)>(self);
        }
        auto req = self.template create_req<HttpRequest::Post>("value");
        nlohmann::json j;
        j["text"] = s;
        self.d().make_req(req, j);
        return std::forward<decltype(self)>(self);
    }
    decltype(auto) send_keys_if_not_equal(this auto &&self, const std::string &s) {
        if (self.text() == s) {
            return std::forward<decltype(self)>(self);
        }
        return self.send_keys(s);
    }
    decltype(auto) clear_and_send_keys_if_not_equal(this auto &&self, const std::string &s) {
        if (self.text() == s) {
            return std::forward<decltype(self)>(self);
        }
        return std::forward<decltype(self)>(self.clear().send_keys(s));
    }
    decltype(auto) set_value_and_wait_for_it(this auto&& self, const std::string& s) {
        self.d().wait_full_load();
        auto tm = primitives::date_time::make_timeout();
        while (self.text() != s) {
            tm();
            self.d().wait_full_load();
        }
        return std::forward<decltype(self)>(self);
    }
    decltype(auto) clear(this auto&& self) {
        /*std::string s;
        s += u'\ue009';
        s += u'\ue00e';
        return self.send_keys(s);*/
        auto req = self.template create_req<HttpRequest::Post>("clear");
        nlohmann::json j;
        j["element"] = self.ref;
        j[self.ref] = self.element_;
        self.d().make_req(req, j);
        return std::forward<decltype(self)>(self);
    }
    std::string text() {
        auto req = create_req("text");
        auto j = d().make_req(req);
        return j["value"];
    }
    std::string attribute(auto &&name) {
        auto req = create_req(path{ "attribute" } / name);
        auto j = d().make_req(req);
        return j["value"].is_null() ? "" : j["value"];
    }
    /*std::string wait_for_non_empty_text() {
        auto t = primitives::date_time::make_timeout(10ms, 1s);
        while (1) {
            try {
                auto te = text();
                if (!te.empty()) {
                    return te;
                }
            } catch (std::exception &) {
                // element can be stale, so we ignore this exception
            }
            t();
        }
    }*/
    decltype(auto) click(this auto &&self) {
        auto req = self.template create_req<HttpRequest::Post>("click");
        self.d().make_req(req, nlohmann::json::object());
        return std::forward<decltype(self)>(self);
    }
    // not w3c standard
    /*decltype(auto) submit(this auto &&self) {
        auto req = self.create_req<HttpRequest::Post>("submit");
        self.d().make_req(req, nlohmann::json::object());
        return std::forward<decltype(self)>(self);
    }*/
    auto wait_for_element(auto&& v, std::chrono::duration<float> tm = 10ms, std::chrono::duration<float> max = 1s, auto && ... args) {
        auto t = primitives::date_time::make_timeout(tm, max, args...);
        while (1) {
            try {
                return find_element(v);
            } catch (element_not_interactable_exception &) {
                t.simple_wait();
            } catch (stale_element_reference_exception &) {
                t.simple_wait();
            } catch (no_such_element_exception &e) {
                d().log(e.what());
                t();
            }
            d().source();
        }
    }
    auto find_element(auto &&v) {
        return d().find_element(v, create_req<HttpRequest::Post>("element"))[0];
    }
    auto find_elements(auto &&v) {
        return d().find_element<true>(v, create_req<HttpRequest::Post>("elements"));
    }
    auto select(auto &&v) {
        return find_element(v);
    }
    std::optional<element> has_element(auto &&v) {
        try {
            return find_element(v);
        } catch (std::exception &) {
        }
        return {};
    }
};

template <typename Webdriver>
struct window {
    Webdriver *d_{nullptr};
    std::string id;
    std::string type;

    explicit operator bool() const { return !!d_; }
    auto& d() { return *d_; }

    auto operator<=>(const window& rhs) const = default;

    void switch_() {
        auto req = d().create_req<HttpRequest::Post>(d().make_session_url() / "window");
        nlohmann::json j;
        j["handle"] = id;
        j["type"] = type;
        d().make_req(req, j);
        d().wait_full_load();
    }
    void maximize() {
        auto req = d().create_req<HttpRequest::Post>(d().make_session_url() / "window" / "maximize");
        nlohmann::json j;
        j["handle"] = id;
        j["type"] = type;
        d().make_req(req, j);
    }
};

struct simple_logger {
    std::ostringstream ss;
    ~simple_logger() {
        std::cerr << "webdriver: " << ss.str() << "\n";
    }
    auto &operator<<(auto &&v) {
        return ss << v;
    }
};

// https://www.w3.org/TR/webdriver
template <typename Logger = simple_logger>
struct webdriver {
    static inline constexpr auto selenium_host = "http://localhost:4444";
    static inline constexpr auto chromedriver_host = "http://localhost:9515";

    struct options {
        std::string url;
        bool headless{};
        path download_dir;
    };

    path url_;
    nlohmann::json session;
    std::string session_id;
    Logger logger;

    template <auto v = HttpRequest::Get>
    auto create_req(auto &&url) {
        HttpRequest req{ httpSettings };
        req.type = v;
        if constexpr (requires {url.string(); }) {
            req.url = normalize_path(url.string()).string();
        } else {
            req.url = normalize_path(url).string();
        }
        return req;
    }
    auto make_req(auto &&req, bool print_response = true) {
        static const char *arr[] = {"get", "post", "delete"};
        log(std::format("request (type {}): {}", (req.type < std::size(arr) ? arr[req.type] : std::to_string(req.type)), req.url));
        if (!req.data.empty()) {
            log(std::format("data {}", req.data));
        } else if (!req.data_kv.empty()) {
            //for (auto &&)
            //log(std::format("data {}", req.data));
        }
        auto resp = url_request(req);
        auto jr = nlohmann::json::parse(resp.response);
        log(std::format("response (code = {})", resp.http_code));
        if (print_response) {
            log(jr.dump());
        }
        if (resp.http_code != 200) {
            std::string err = "error for request:\n" + req.url + "\n" + req.data + "\n";
            err += "error:\n";
            if (jr.contains("value") && jr["value"].contains("message"))
                err += jr["value"]["message"];
            if (!req.data.empty() && req.data != "{}") {
                err += "\n";
                err += "data " + req.data;
            } else if (!req.data_kv.empty()) {
                err += "\n";
                err += "data\n";
                for (auto &&[k,v] : req.data_kv) {
                    err += k + ": " + v;
                }
            }
            if (jr.contains("value") && jr["value"].contains("error")) {
                if (jr["value"]["error"].get<std::string_view>() == "no such element"sv) {
                    throw no_such_element_exception(err);
                }
                if (jr["value"]["error"].get<std::string_view>() == "element not interactable"sv) {
                    throw element_not_interactable_exception(err);
                }
                if (jr["value"]["error"].get<std::string_view>() == "javascript error"sv) {
                    throw javascript_error_exception(err);
                }
                if (jr["value"]["error"].get<std::string_view>() == "stale element reference"sv) {
                    throw stale_element_reference_exception(err);
                }
                if (jr["value"]["error"].get<std::string_view>() == "unexpected alert open"sv) {
                    throw unexpected_alert_open_exception(err);
                }
                if (jr["value"]["error"].get<std::string_view>() == "no such alert"sv) {
                    throw no_such_alert_exception(err);
                }
                if (jr["value"]["error"].get<std::string_view>() == "timeout"sv) {
                    throw timeout_exception(err);
                }
                if (jr["value"]["error"].get<std::string_view>() == "invalid argument"sv) {
                    throw invalid_argument(err);
                }
                if (jr["value"]["error"].get<std::string_view>() == "session not created"sv) {
                    session_not_created_exception s{err};
                    std::string m = jr["value"]["message"];
                    auto what = "Current browser version is "sv;
                    auto p = m.find(what);
                    if (p != -1) {
                        auto b = p + what.size();
                        auto e = m.find(' ', b);
                        s.actual_chrome_version = m.substr(b, e-b);
                    }
                    throw s;
                }
            }
            throw SW_RUNTIME_ERROR(err);
        }
        return jr;
    }
    auto make_req(auto &&req, auto &&json, bool print_response = true) {
        req.data = json.dump();
        return make_req(req, print_response);
    }
    path make_session_url() {
        return url_ / "session" / session_id;
    }

    webdriver() {}
    webdriver(const options &opts) {
        init(opts);
    }
    webdriver(const webdriver&) = delete;
    webdriver &operator=(const webdriver&) = delete;
    webdriver(webdriver&&) = default;
    webdriver &operator=(webdriver&&) = default;
    ~webdriver() {
        //logger() << "dtor";
        if (*this) {
            try {
                make_req(create_req<HttpRequest::Delete>(make_session_url()));
            } catch (std::exception &e) {
                log(e.what());
            }
        }
        //logger() << "dtor end";
    }
    void log(auto &&x) {
        logger << x;
    }
    void init(const options &opts) {
        url_ = opts.url;
        primitives::http::setupSafeTls();
        auto req = create_req<HttpRequest::Post>(opts.url + "/session");
        req.timeout = 120;
        req.connect_timeout = 120;
        nlohmann::json j;
        auto &jc = j["capabilities"];
        nlohmann::json jfm;
        jfm["browserName"] = "chrome";
        //jfm["browserVersion"] = "103.0.5060.53";
        jfm["browserVersion"] = "102";
        if (opts.headless) {
            jfm["goog:chromeOptions"]["args"].push_back("--headless");
        }
        if (!opts.download_dir.empty()) {
            jfm["goog:chromeOptions"]["prefs"]["download.default_directory"] = opts.download_dir.string();
        }
        //jfm["goog:chromeOptions"]["w3c"] = false;
        //jfm["chromeOptions"]["w3c"] = false;
        //jfm["w3c"] = false;
        jfm["platformName"] = "any";
        jc["firstMatch"].push_back(jfm);
        j["requiredCapabilities"] = jc;
        session = make_req(req, j);
        session_id = session["value"]["sessionId"];
        if (session_id.empty()) {
            throw SW_RUNTIME_ERROR("empty session id");
        }
    }
    explicit operator bool() const { return !session_id.empty(); }
    void navigate(const std::string &url) {
        auto req = create_req<HttpRequest::Post>(make_session_url() / "url");
        nlohmann::json j;
        j["url"] = url;
        make_req(req, j);
    }
    template <bool multi = false>
    auto find_element_base(auto&& v, auto&& req) {
        log(std::format(multi ? "find_elements: {}" : "find_element: {}", v.value));
        nlohmann::json j;
        j["using"] = v.strategy;
        j["value"] = v.value;
        return make_req(req, j);
    }
    template <bool multi = false>
    auto find_element(auto&& v, auto&& req) {
        auto resp = find_element_base<multi>(v, req);
        std::vector<element<webdriver>> els;
        if constexpr (multi) {
            for (auto &&el : resp["value"]) {
                for (auto &&[k,v] : el.items()) {
                    els.emplace_back(this, k, v);
                }
            }
        } else {
            for (auto &&[k,v] : resp["value"].items()) {
                els.emplace_back(this, k, v);
            }
        }
        return els;
    }
    auto find_element(auto &&v) {
        return find_element(v, create_req<HttpRequest::Post>(make_session_url() / "element"))[0];
    }
    auto find_elements(auto &&v) {
        return find_element<true>(v, create_req<HttpRequest::Post>(make_session_url() / "elements"));
    }
    std::optional<element<webdriver>> has_element(auto &&v) {
        try {
            return find_element(v);
        } catch (std::exception &) {
        }
        return {};
    }
    std::optional<std::vector<element<webdriver>>> has_elements(auto &&v) {
        try {
            return find_elements(v);
        } catch (std::exception &) {
        }
        return {};
    }
    auto wait_for_element(auto &&v) {
        auto t = primitives::date_time::make_timeout(10ms, 1s);
        while (1) {
            try {
                return find_element(v);
            } catch (std::exception &e) {
                log(e.what());
            }
            t();
            source();
        }
    }
    std::string url() {
        auto req = create_req<HttpRequest::Get>(make_session_url() / "url");
        return make_req(req)["value"];
    }
    void close() {
        auto req = create_req<HttpRequest::Delete>(make_session_url() / "window");
        make_req(req);
    }
    void back() {
        auto req = create_req<HttpRequest::Post>(make_session_url() / "back");
        nlohmann::json j;
        j["url"] = url();
        j["value"] = nullptr;
        make_req(req, j);
    }
    auto refresh() {
        auto req = create_req<HttpRequest::Post>(make_session_url() / "refresh");
        nlohmann::json j;
        j["url"] = url();
        j["value"] = nullptr;
        make_req(req, j);
    }
    // sync
    auto execute(const std::string &script) {
        auto req = create_req<HttpRequest::Post>(make_session_url() / "execute" / "sync");
        nlohmann::json j;
        j["script"] = script;
        j["args"] = std::vector<std::string>{};
        auto resp = make_req(req, j);
        return resp["value"];
    }
    auto source() {
        auto req = create_req(make_session_url() / "source");
        auto resp = make_req(req, false);
        return resp.dump();
    }
    /*auto page_source() {
        return source();
    }*/
    void focus_frame(auto &&id) {
        log("focus_frame"sv);
        auto req = create_req<HttpRequest::Post>(make_session_url() / "frame");
        nlohmann::json j;
        if constexpr (requires { id.element_; }) {
            j["id"][id.ref] = id.element_;
            j["id"]["ELEMENT"] = id.element_;
        } else if constexpr (std::same_as<std::decay_t<decltype(id)>, int>) {
            j["id"] = id;
        } else {
            j["id"] = nullptr; // go to main frame (window)
        }
        make_req(req, j);
    }
    void wait_full_load() {
        log("waiting for load"sv);
        while (execute("return document.readyState") != "complete" && execute("return jQuery.active") != 0);
    }
    template <template <typename ...> typename T = std::vector>
    auto windows() {
        auto req = create_req(make_session_url() / "window" / "handles");
        auto resp = make_req(req);
        T<primitives::webdriver::window<webdriver>> ww;
        for (auto &&w : resp["value"].get<std::vector<std::string>>()) {
            if constexpr (requires { ww.emplace_back(this, w, "tab"); }) {
                ww.emplace_back(this, w, "tab");
            } else {
                ww.emplace({ this, w, "tab" });
            }
        }
        if (ww.empty())
            throw SW_RUNTIME_ERROR("windows cannot be empty");
        return ww;
    }
    auto window() {
        /*auto wnds = windows();
        if (wnds.size() == 1) {
            return wnds[0];
        }*/
        auto req = create_req<HttpRequest::Post>(make_session_url() / "window" / "new");
        nlohmann::json j;
        j["type"] = "tab";
        auto resp = make_req(req, j);
        return primitives::webdriver::window<webdriver>{this, resp["value"]["handle"], "tab"};
    }
    template <template <typename ...> typename T = std::vector>
    auto get_new_windows(auto &&func_in_between) {
        auto windows0 = windows<T>();
        func_in_between();
        wait_full_load();
        auto windows1 = windows<T>();
        T<primitives::webdriver::window<webdriver>> result;
        std::ranges::set_difference(windows1, windows0, std::inserter(result, result.end()));
        return result;
    }
    template <template <typename ...> typename T = std::vector>
    auto wait_until_new_windows(auto &&func_in_between) {
        auto windows0 = windows<T>();
        func_in_between();
        wait_full_load();
        while (1) {
            auto windows1 = windows<T>();
            T<primitives::webdriver::window<webdriver>> result;
            std::ranges::set_difference(windows1, windows0, std::inserter(result, result.end()));
            if (!result.empty()) {
                return result;
            }
            wait_full_load();
            primitives::date_time::sleep(500ms);
        }
    }
    auto cookies() {
        struct cooki {
            std::string name;
            std::string value;
        };
        auto req = create_req<HttpRequest::Post>(make_session_url() / "cookie");
        auto jr = make_req(req);
        std::vector<cooki> cs;
        for (auto &&jc : jr["value"]) {
            cooki c;
            c.name = jc["name"];
            c.value = jc["value"];
            cs.emplace_back(c);
        }
        return cs;
    }
    auto cookie(auto &&name) {
        auto req = create_req<HttpRequest::Get>(make_session_url() / "cookie" / name);
        auto jr = make_req(req);
        return jr["value"];
    }
    std::optional<std::string> has_alert() {
        try {
            auto req = create_req<HttpRequest::Get>(make_session_url() / "alert" / "text");
            auto j = make_req(req);
            return j["value"].is_null() ? ""s : j["value"].get<std::string>();
        } catch (no_such_alert_exception &) {
            return {};
        }
    }
    void accept_alert() {
        auto req = create_req<HttpRequest::Post>(make_session_url() / "alert" / "accept");
        make_req(req);
    }
};

} // namespace primitives::webdriver
