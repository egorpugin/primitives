// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <list>
#include <memory>
#include <string>
#include <stack>
#include <vector>

#include <primitives/filesystem.h>

struct PRIMITIVES_CONTEXT_API Context
{
    using Text = std::string;

    struct Line
    {
        Text text;
        int n_indents = 0;

        Line() {}
        Line(const Text &t, int n = 0)
            : text(t), n_indents(n)
        {}

        Line& operator+=(const Text &t)
        {
            text += t;
            return *this;
        }
    };

    using Lines = std::list<Line>;

    //
    Context(const Text &indent = "    ", const Text &newline = "\n");
    Context(const Context &ctx);
    Context &operator=(const Context &ctx);

    void initFromString(const std::string &s);

    void addLine(const Text &s = Text());
    void addNoNewLine(const Text &s);
    void addLineNoSpace(const Text &s);
    void removeLine();
    void removeLines(int n);

    void addText(const Text &s);
    void addText(const char* str, int n);

    void increaseIndent(int n = 1);
    void decreaseIndent(int n = 1);
    void increaseIndent(const Text &s, int n = 1);
    void decreaseIndent(const Text &s, int n = 1);

    void trimEnd(size_t n);

    Text getText() const;

    void setLines(const Lines &lines);
    Lines getLines() const;
    Lines &getLinesRef() { return lines; }
    void mergeBeforeAndAfterLines();

    void splitLines();
    void setMaxEmptyLines(int n);

    Context &before()
    {
        if (!before_)
            before_ = std::make_shared<Context>();
        return *before_;
    }
    Context &after()
    {
        if (!after_)
            after_ = std::make_shared<Context>();
        return *after_;
    }

    void emptyLines(int n = 1);

    // add with "as is" indent
    Context &operator+=(const Context &rhs);
    // add with relative indent
    void addWithRelativeIndent(const Context &rhs);

    bool empty() const
    {
        bool e = false;
        if (before_)
            e |= before_->empty();
        e |= lines.empty();
        if (after_)
            e |= after_->empty();
        return e;
    }

    void printToFile(FILE* out) const;

    void clear()
    {
        lines.clear();
        before_.reset();
        after_.reset();
        while (!namespaces.empty())
            namespaces.pop();
    }

protected:
    Lines lines;
    std::shared_ptr<Context> before_;
    std::shared_ptr<Context> after_;

    int n_indents = 0;
    Text indent;
    Text newline;
    std::stack<Text> namespaces;

    void copy_from(const Context &ctx);
};

struct PRIMITIVES_CONTEXT_API CppContext : Context
{
    void beginBlock(const Text &s = "", bool indent = true);
    void endBlock(bool semicolon = false);
    void beginFunction(const Text &s = "");
    void endFunction();
    void beginNamespace(const Text &s);
    void endNamespace(const Text &s = Text());

    void ifdef(const Text &s);
    void endif();
};

struct PRIMITIVES_CONTEXT_API BinaryContext
{
    BinaryContext();
    BinaryContext(size_t size);
    BinaryContext(const std::string &s);
    BinaryContext(const std::vector<uint8_t> &buf, size_t data_offset = 0);
    BinaryContext(const BinaryContext &rhs, size_t size);
    BinaryContext(const BinaryContext &rhs, size_t size, size_t offset);

    template <typename T>
    size_t read(T &dst, size_t size = 1) const
    {
        return _read((void *)&dst, size * sizeof(T), 0);
    }
    template <typename T>
    size_t readfrom(T *dst, size_t size, size_t offset) const
    {
        return _read(dst, size * sizeof(T), offset);
    }
    size_t read(std::string &s);

    template <typename T>
    size_t write(const T &src)
    {
        return _write(&src, sizeof(T));
    }
    template <typename T>
    size_t write(const T *src, size_t size)
    {
        return _write(src, size * sizeof(T));
    }
    template <typename T>
    size_t write(const T *src)
    {
        return _write(src, sizeof(T));
    }
    size_t write(const std::string &s)
    {
        auto sz = s.size() + 1;
        _write(s.c_str(), sz);
        return sz;
    }

    template <class T, class SizeType = size_t>
    void read_vector(std::vector<T> &v, int n) const
    {
        v.clear();
        v.reserve(n);
        for (int i = 0; i < n; i++)
        {
            T t;
            t.load(*this);
            v.push_back(t);
        }
    }

    template <class T, class SizeType = size_t>
    void read_vector(std::vector<T> &v) const
    {
        SizeType n = 0;
        read(n);
        read_vector<T, SizeType>(v, n);
    }

    void seek(size_t size) const; // setpos
    void skip(int n) const;
    size_t end() const { return end_; }
    bool eof() const;
    bool check(int index) const;
    void reset() const;

    size_t index() const;
    size_t size() const;
    const std::vector<uint8_t> &buf() const;

    const uint8_t *getPtr() const { return ptr; }

    size_t _read(void *dst, size_t size, size_t offset = 0) const;
    size_t _write(const void *src, size_t size);

    void load(const path &fn);
    void save(const path &fn);

protected:
    std::shared_ptr<std::vector<uint8_t>> buf_;
    mutable size_t index_ = 0;
    mutable uint8_t *ptr = 0;
    mutable size_t data_offset = 0;
    mutable size_t size_ = 0;
    size_t end_;
};
