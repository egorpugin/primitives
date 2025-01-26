#pragma once

#include <primitives/http.h>
#include <primitives/templates2/sqlite.h>

#include <boost/algorithm/string.hpp>
#include <boost/container_hash/hash.hpp>
#include <nlohmann/json.hpp>

struct rate_limit : std::runtime_error {
    using std::runtime_error::runtime_error;
};
struct sheet_does_not_exist : std::runtime_error {
    using std::runtime_error::runtime_error;
};

int j2i(auto &&j) {
    std::string s = j;
    boost::trim(s);
    if (s.empty()) {
        return 0;
    }
    return std::stoi(s);
}
int j2if(auto &&j, int mult = 1000) {
    std::string s = j;
    auto b = s.contains(',');
    boost::replace_all(s, ",", ".");
    boost::trim(s);
    if (s.empty()) {
        return 0;
    }
    auto f = std::stof(s);
    return b ? f * mult : f;
}
bool is_number(auto &&j, auto &&i) {
    std::string s = j;
    boost::trim(s);
    auto r = std::from_chars(s.data(), s.data() + s.size(), i);
    return r == decltype(r){};
}
bool is_number(auto &&j) {
    int i;
    return is_number(j,i);
}

// https://developers.google.com/sheets/api/reference/rest
// https://sheets.googleapis.com/$discovery/rest
struct google_sheets {
    std::string api_key;
    std::string bearer;

    google_sheets(){}
    auto base_url() {
        return std::format("https://sheets.googleapis.com/v4/spreadsheets");
    }
    auto make_url(auto &&q) {
        return std::format("{}{}", base_url(), q);
    }
    auto make_request() {
        HttpRequest req{httpSettings};
        req.connect_timeout = 10;
        req.timeout = 60;
        if (!api_key.empty()) {
            req.data_kv["access_token"] = api_key;
        }
        if (!bearer.empty()) {
            req.headers.push_back("Authorization: Bearer " + bearer);
        }
        return req;
    }
    auto set_params(auto &&req, const auto & ... params) {
        auto add_get_param = [&, has_params = req.url.contains('?')](auto &&k, auto &&v) mutable {
            std::string x{!has_params ? '?' : '&'};
            req.url += x + k + "="s + v;
            has_params = true;
        };
        auto add_get_param2 = [&](this auto &&f, auto &&k, auto &&v, auto && ... params) {
            add_get_param(k, v);
            if constexpr (sizeof...(params)) {
                f(params...);
            }
        };
        if constexpr (sizeof...(params)) {
            add_get_param2(params...);
        }
    }
    auto make_query(auto &&q, std::string cache_key, auto && ... params) {
        auto req = make_request();
        req.url = make_url(q);
        set_params(req, params...);
        if (!api_key.empty()) {
            set_params(req, "key", api_key);
        }

        auto h = std::hash<std::decay_t<decltype(req.url)>>()(req.url);
        boost::hash_combine(h, std::hash<std::decay_t<decltype(cache_key)>>()(cache_key));
        auto add_get_param3 = [&](this auto &&f, const std::string &k, const std::string &v, auto &&...params) {
            boost::hash_combine(h, std::hash<std::decay_t<decltype(k)>>()(k));
            boost::hash_combine(h, std::hash<std::decay_t<decltype(v)>>()(v));
            if constexpr (sizeof...(params)) {
                f(params...);
            }
        };
        if constexpr (sizeof...(params)) {
            add_get_param3(params...);
        }

        if (cache_key.empty()) {
            auto resp = url_request(req);
            return process_response(req, resp);
        }

        struct cached_url_request : primitives::sqlite::kv<size_t, std::string> {};
        static primitives::sqlite::cache<cached_url_request> c{"cache.db"};
        auto r = c.find<cached_url_request>(h, [&](auto &&set) {
            nlohmann::json j;
            while (1) {
                try {
                    auto resp = url_request(req);
                    j = process_response(req, resp);
                    break;
                } catch (rate_limit &) {
                    std::this_thread::sleep_for(1min);
                }
            }
            set(j.dump());
        });
        return nlohmann::json::parse(r);
    }
    nlohmann::json process_response(auto &&req, auto &&resp) {
        auto try_parse = [](auto &&s) {
            try {
                auto j = nlohmann::json::parse(s);
                return j;
            } catch (std::exception &) {
                return nlohmann::json{};
            }
        };

        if (resp.http_code == 404) {
            throw std::runtime_error{"not found: "s + req.url + ": " + resp.response};
        } else if (resp.http_code >= 500) {
            throw std::runtime_error{"server error: "s + req.url + ": " + resp.response};
        } else if (resp.http_code != 200) {
            auto j = try_parse(resp.response);
            if (j.empty()) {
                throw std::runtime_error{"url request error: "s + req.url + ": " + resp.response};
            }
            if (resp.http_code == 429 && j["error"]["details"][0]["reason"] == "RATE_LIMIT_EXCEEDED"s) {
                throw rate_limit{""};
            }
            if (resp.http_code == 400 && j["error"]["status"] == "INVALID_ARGUMENT"s) {
                throw sheet_does_not_exist{""};
            }
            throw std::runtime_error{"cant make request: "s + req.url + ": "s +
                                     j["error"]["message"].template get<std::string>()};
        }
        return nlohmann::json::parse(resp.response);
    }

    auto get_raw(auto &&id) {
        auto now = std::chrono::system_clock::now();
        auto date = std::format("{:%F}", now);
        auto ret = make_query("/"s + id, date);
        return ret;
    }
    struct sheet get_default_sheet(auto &&spreadsheet_id);
};

struct sheet : google_sheets {
    struct point {
        int row;
        int col;

        point() = default;
        point(int r, int c) : row{r}, col{c} {}
        point(const std::string &s) {
            if (s.empty()) {
                throw std::runtime_error{"bad cell"};
            }
            auto p = s.data();
            while (isalpha(*p)) ++p;
            std::string_view col{s.data(), p};
            std::string_view row{p,s.data()+s.size()};

            if (col.empty() || col.size() > 2 || !isupper(col[0])) {
                throw std::runtime_error{"bad col"};
            }
            if (col.size() == 2) {
                this->col = 26 + (col[0] - 'A') * 26 + (col[1] - 'A');
            }
            if (col.size() == 1) {
                this->col = col[0] - 'A';
            }
            this->row = std::stoi(std::string{row}) - 1;
        }
        std::string col2string() const {
            if (col >= 26) {
                SW_UNIMPLEMENTED;
            }
            return std::format("{}", (char)(col + 'A'));
        }
        operator std::string() const {
            return std::format("{}{}", col2string(), row + 1);
        }
    };
    // stored as [from,to)
    // printed as [from,to(-1,-1)]
    struct range {
        point from{};
        point to{};
        //std::string page;

        range() = default;
        range(const point &from, const point &to) : from{from}, to{to} {}
        template <auto N>
        range(const char (&s)[N]) : range{std::string{s}} {
        }
        range(const std::string &s) {
            if (s.empty()) {
                throw std::runtime_error{"bad range"};
            }
            auto range = split_string(s, ":");
            if (range.size() == 1) {
                from = to = point{range[0]};
            } else {
                from = point{range[0]};
                to = point{range[1]};
            }
            // make it past the end
            ++to.row;
            ++to.col;
        }
        auto width() const {
            return to.col - from.col;
        }
        auto height() const {
            return to.row - from.row;
        }
        operator std::string() const {
            auto to2 = to;
            --to2.row;
            --to2.col;
            std::string f = from;
            std::string t = to2;
            if (f == t) {
                return f;
            }
            return std::format("{}:{}", f, t);
        }
        auto operator++(int) {
            auto r = *this;
            ++from.row;
            ++to.row;
            return r;
        }
    };
    struct value {
        std::vector<std::vector<std::string>> j;
        range r;

        value() = default;
        value(const nlohmann::json &in_j) {
            j = in_j;
        }
        value(const decltype(j) &in_j, const decltype(r) &range) : r{range} {
            j = in_j;
            j.resize(r.height());
            for (auto &&row : j) {
                row.resize(r.width());
            }
        }
        operator int() const {
            std::string v = j[0][0];
            return std::stoi(v);
        }
        auto begin() {return j.begin();}
        auto end() {return j.end();}

        decltype(auto) operator[](int row) {
            return j[row];
        }
        decltype(auto) operator[](const std::string &cell) {
            range r2{cell};
            return j[r2.from.row - r.from.row][r2.from.col - r.from.col];
        }
        decltype(auto) operator[](const char *cell) {
            return operator[](std::string{cell});
        }
        // subrange with corner in cell
        value operator()(const char *cell) const {
            range r2{cell};
            value v2{j};
            v2.r.from = r2.from;
            v2.r.to = r.to;
            v2.j.assign(v2.j.begin() + r2.from.row - r.from.row, v2.j.end());
            for (auto &&j2 : v2.j) {
                j2.assign(j2.begin() + r2.from.col - r.from.col, j2.end());
            }
            return v2;
        }
        auto json(std::string_view page = {}) const {
            nlohmann::json j;
            j["range"] = page.empty() ? (std::string)r : std::format("'{}'!{}", page, (std::string)r);
            for (int r{}; auto &&jrow : this->j) {
                for (int c{}; auto &&jv : jrow) {
                    j["values"][r][c++] = jv;
                }
                ++r;
            }
            return j;
        }
    };

    std::string spreadsheet_id;
    std::string page;

    auto values(auto &&id, const std::string &range) {
        auto r = std::format("'{}'!{}", page, range);
        r = primitives::http::url_encode(r);
        auto ret = make_query(std::format("/{}/values/{}", id, r), ""s);
        return value{ret["values"], range};
    }
    auto operator[](const std::string &range) {
        return values(spreadsheet_id, range);
    }
    auto operator[](const range &r) {
        return values(spreadsheet_id, r);
    }

    //
    auto set_one(const value &v, const std::string &value_input_option = "RAW"s) {
        auto r = std::format("'{}'!{}", page, (std::string)v.r);
        r = primitives::http::url_encode(r);
        auto req = make_request();
        req.url = make_url(std::format("/{}/values/{}", spreadsheet_id, r));
        req.type = HttpRequest::Put;
        req.data = v.json(page).dump();
        req.headers.push_back("Content-Type: application/json");
        set_params(req, "valueInputOption", value_input_option);
        auto resp = url_request(req);
        auto j = process_response(req, resp);
        return j;
    }
    struct set_many_type {
        std::vector<value> values;
        std::string value_input_option{"RAW"};

        auto json(const std::string &page) const {
            nlohmann::json j;
            j["valueInputOption"] = value_input_option;
            for (auto &&v : values) {
                j["data"].push_back(v.json(page));
            }
            return j;
        }
    };
    auto set_many(const set_many_type &v) {
        auto req = make_request();
        req.url = make_url(std::format("/{}/values:batchUpdate", spreadsheet_id));
        req.type = HttpRequest::Post;
        req.data = v.json(page).dump();
        req.headers.push_back("Content-Type: application/json");
        auto resp = url_request(req);
        auto j = process_response(req, resp);
        return j;
    }

    auto create_sheet(const std::string &name, int idx) {
        auto req = make_request();
        req.url = make_url(std::format("/{}:batchUpdate", spreadsheet_id));
        req.type = HttpRequest::Post;
        nlohmann::json jd;
        auto &js = jd["requests"]["addSheet"]["properties"];
        js["sheetId"] = idx;
        js["title"] = name;
        req.data = jd.dump();
        req.headers.push_back("Content-Type: application/json");
        auto resp = url_request(req);
        process_response(req, resp);

        auto s = *this;
        s.page = name;
        return s;
    }
    auto get_or_create_sheet(const std::string &name, int idx) {
        try {
            auto j = make_query("/"s + spreadsheet_id, {});
            for (auto &&jp : j["sheets"]) {
                if (idx && jp["properties"]["sheedId"] == idx || jp["properties"]["title"] == name) {
                    auto s = *this;
                    s.page = name;
                    return s;
                }
            }
        } catch (std::exception &) {
        }
        return create_sheet(name, idx);
    }
    void set_rows(int rows) {
        auto req = make_request();
        req.url = make_url(std::format("/{}:batchUpdate", spreadsheet_id));
        req.type = HttpRequest::Post;
        nlohmann::json jd;
        jd["requests"]["updateSheetProperties"]["fields"] = "gridProperties";
        jd["requests"]["updateSheetProperties"]["properties"]["gridProperties"]["rowCount"] = rows;
        jd["requests"]["updateSheetProperties"]["properties"]["gridProperties"]["columnCount"] = 26;
        req.data = jd.dump();
        req.headers.push_back("Content-Type: application/json");
        auto resp = url_request(req);
        process_response(req, resp);
    }
};

sheet google_sheets::get_default_sheet(auto &&spreadsheet_id) {
    auto j = get_raw(spreadsheet_id);

    sheet ms;
    ms.api_key = api_key;
    ms.bearer = bearer;
    ms.spreadsheet_id = spreadsheet_id;
    ms.page = j["sheets"][0]["properties"]["title"];
    return ms;
}
