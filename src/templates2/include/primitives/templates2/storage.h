// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2022 Egor Pugin <egor.pugin@gmail.com>

#pragma once

#include "helpers.h"

namespace sw {

struct basic_contents_hash {
    using hash_type = size_t;

    auto operator()(const path &fn) const {
        return std::hash<string>{}(read_file(fn));
    }
};

template <typename Hash>
struct physical_file_storage {
    path root;

    physical_file_storage(const path &root) : root{root} {
        fs::create_directories(root);
    }
    bool contains(auto &&hash) {
        return fs::exists(make_path(hash));
    }
    auto add(const path &fn) {
        auto hash = get_hash(fn);
        if (!contains(hash))
            fs::copy_file(fn, make_path(hash));
        return hash;
    }
    void remove(auto &&hash) {
        fs::remove(make_path(hash));
    }
    auto get_hash(const path &fn) {
        return Hash{}(fn);
    }

    struct iterator {
        fs::directory_iterator i;

        bool operator==(const iterator &) const = default;
        iterator &operator++() {
            ++i;
            return *this;
        }
        auto operator*() {
            struct obj {
                iterator &self;
                auto path() const {
                    return self.i->path();
                }
                auto hash() const {
                    return path().filename();
                }
            };
            obj d{*this};
            return d;
        }
    };
    auto begin() const {
        return iterator{fs::directory_iterator{root}};
    }
    auto end() const {
        return iterator{fs::directory_iterator{}};
    }

private:
    path make_path(auto &&hash) {
        return root / std::to_string(hash);
    }
};

template <typename Hash>
struct physical_file_storage_single_file {
    struct file {
        FILE *f;
        file(const path &name) {
            f = fopen(name.string().c_str(), "ab+");
            if (!f) {
                throw std::runtime_error{"cannot open storage file: " + name.string()};
            }
            fseek(f, 0, SEEK_END);
        }
        ~file() {
            fclose(f);
        }
        operator FILE *() const {
            return f;
        }

        file(const file &) = delete;
        file &operator=(const file &) = delete;
    };
    struct record {
        typename Hash::hash_type hash;
        uint64_t pos;
        uint64_t size;
    };

    file f;
    file fi;

    physical_file_storage_single_file(const path &name) : f(name), fi(path(name) += ".idx") {
    }

    bool contains(auto &&hash) {
        for (auto &&handle : *this) {
            if (hash == handle.hash()) {
                return true;
            }
        }
        return false;
    }
    auto add(const path &fn) {
        auto s = read_file(fn);
        record r{get_hash(fn), (uint64_t)ftell(f), s.size()};
        fwrite(s.data(), s.size(), 1, f);
        fflush(f);
        fwrite(&r, sizeof(r), 1, fi);
        fflush(fi);
        return r.hash;
    }
    void remove(auto &&) {
    }
    auto get_hash(const path &fn) {
        return Hash{}(fn);
    }

    struct sentinel {};
    struct iterator {
        const physical_file_storage_single_file &s;
        unsigned offset = 0;

        bool operator==(sentinel) const {
            return offset == ftell(s.fi);
        }
        iterator &operator++() {
            offset += sizeof(record);
            return *this;
        }
        auto operator*() {
            struct obj {
                const physical_file_storage_single_file &s;
                record r;

                auto hash() {
                    return r.hash;
                }
                auto size() {
                    return r.size;
                }
                void copy(const path &fn) {
                    std::vector<std::byte> buffer;
                    buffer.resize(r.size);
                    fseek(s.f, r.pos, SEEK_SET);
                    fread(buffer.data(), r.size, 1, s.f);
                    fseek(s.f, 0, SEEK_END);

                    auto f = fopen(fn.string().c_str(), "wb");
                    fwrite(buffer.data(), r.size, 1, f);
                    fclose(f);
                }
            };
            obj o{s};
            fseek(s.fi, offset, SEEK_SET);
            fread(&o.r, sizeof(o.r), 1, s.fi);
            fseek(s.fi, 0, SEEK_END);
            return o;
        }
    };
    auto begin() const {
        return iterator{*this};
    }
    auto end() const {
        return sentinel{};
    }
};

template <typename PhysicalStorage>
struct file_storage {
    PhysicalStorage ps;

    const auto &physical_storage() const {
        return ps;
    }
    //operator const auto &() const { return physical_storage(); }
    auto begin() const { return physical_storage().begin(); }
    auto end() const { return physical_storage().end(); }

    bool contains(auto &&hash) {
        return ps.contains(hash);
    }
    auto add(const path &fn) {
        auto hash = ps.get_hash(fn);
        if (!contains(hash)) {
            ps.add(fn);
        }
        return hash;
    }
    void remove(auto &&hash) {
        ps.remove(hash);
    }

    void add_r(auto &&range) {
        for (auto &&f : range) {
            add(f);
        }
    }
    auto &operator+=(auto &&f) {
        add_r(f);
        return *this;
    }
};

/*void add_file_to_storage(auto &&s, auto &&f) {
}
void add_transform_to_storage(auto &&s, auto &&f) {
    add_file_to_storage(s, f);
}*/

} // namespace sw

