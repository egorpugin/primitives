// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/filesystem.h>
#include <primitives/exceptions.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <list>
#include <memory>
#include <string>
#include <stack>
#include <vector>

namespace primitives
{

namespace detail
{

static Strings splitWithIndent(const String &text, const String &newline)
{
    Strings s;
    auto p = text.find(newline);
    if (p == text.npos)
    {
        s.push_back(text);
        return s;
    }

    size_t old_pos = 0;
    while (1)
    {
        s.push_back(text.substr(old_pos, p - old_pos));
        p += newline.size();
        old_pos = p;
        p = text.find(newline, p);
        if (p == text.npos)
        {
            s.push_back(text.substr(old_pos));
            break;
        }
    }
    return s;
}

struct Line
{
    using Text = String;

    Line() = default;
    Line(const Text &t, int n = 0)
        : n_indents(n), text(t)
    {
    }
    virtual ~Line(){}

    int n_indents = 0;

    Line &operator+=(const Text &t)
    {
        text += t;
        return *this;
    }

    virtual Text getText() const
    {
        return text;
    }
    void setText(const Text &t)
    {
        text = t;
    }

    virtual bool empty() const { return false; }

private:
    Text text;
};

template <class Emitter>
struct EmitterLine : Line
{
    EmitterLine(Emitter &&emitter)
        : emitter(std::move(emitter))
    {
    }

    template <class ... Args>
    EmitterLine(Args && ... args)
        : emitter(std::forward<Args>(args)...)
    {
    }

    Text getText() const override
    {
        auto t = getEmitter().getText();
        t.resize(t.size() - getEmitter().getNewline().size());
        return t;
    }

    bool empty() const override { return emitter.getLines().empty(); }

    Emitter &getEmitter() { return emitter; }
    const Emitter &getEmitter() const { return emitter; }

private:
    Emitter emitter;
};

}

struct Emitter
{
    using Line = detail::Line;
    using LinePtr = std::unique_ptr<Line>;
    using Text = Line::Text;
    using Lines = std::vector<LinePtr>;

    Emitter(const Text &indent = "    ", const Text &newline = "\n")
        : indent(indent), newline(newline)
    {
    }
    Emitter(const Emitter &) = delete;
    Emitter &operator=(const Emitter &) = delete;
    Emitter(Emitter &&) = default;
    Emitter &operator=(Emitter &&) = default;
    virtual ~Emitter(){}

    void addLine(const Text &s = {})
    {
        if (s.empty())
            addLineWithoutIndent(s);
        else
            addLineWithIndent(s);
    }
    void addLineWithIndent(const Text &s)
    {
        addLineWithIndent(s, n_indents);
    }
    // absolute indent
    void addLineWithIndent(const Text &text, int in_indent)
    {
        for (auto &s : detail::splitWithIndent(text, newline))
            addLine(std::make_unique<Line>(s, in_indent));
    }
    void addLineWithoutIndent(const Text &s)
    {
        addLineWithIndent(s, 0);
    }
    void removeLine()
    {
        removeLines(1);
    }
    void removeLines(int n)
    {
        n = std::max(0, (int)lines.size() - n);
        lines.resize(n);
    }

    // insertLine()

    void addText(const Text &s)
    {
        if (lines.empty())
            addLine();
        if (!s.empty() && lines.back()->n_indents == 0)
            lines.back()->n_indents = n_indents;
        lines.back()->setText(lines.back()->getText() += s);
    }
    void addText(const char* str, int n)
    {
        addText(Text(str, str + n));
    }

    template <class U = Emitter, class ... Args>
    U &createInlineEmitter(Args && ... args)
    {
        auto e = std::make_unique<detail::EmitterLine<U>>(std::forward<Args>(args)...);
        e->n_indents = n_indents;
        auto &ref = *e;
        addLine(std::move(e));
        return ref.getEmitter();
    }

    template <class U>
    void addEmitter(U &&emitter)
    {
        auto e = std::make_unique<detail::EmitterLine<U>>(std::forward<U>(emitter));
        e->n_indents = n_indents;
        auto &ref = *e;
        addLine(std::move(e));
    }

    void increaseIndent(int n = 1)
    {
        n_indents += n;
    }
    void decreaseIndent(int n = 1)
    {
        n_indents -= n;
    }
    void increaseIndent(const Text &s, int n = 1)
    {
        addLine(s);
        increaseIndent(n);
    }
    void decreaseIndent(const Text &s, int n = 1)
    {
        decreaseIndent(n);
        addLine(s);
    }

    void trimEnd(size_t n)
    {
        if (lines.empty())
            return;
        auto t = lines.back()->getText();
        auto sz = t.size();
        if (n > sz)
            n = sz;
        t.resize(sz - n);
        lines.back()->setText(t);
    }

    virtual Text getText() const
    {
        Text s;
        for (auto &line : getStrings())
            s += line;
        return s;
    }
    Strings getStrings() const
    {
        Strings s;
        for (auto &line : lines)
        {
            if (line->empty())
                continue;
            auto t = line->getText();
            Text space;
            if (!t.empty())
            {
                for (int i = 0; i < line->n_indents; i++)
                    space += indent;
            }
            for (auto &p : detail::splitWithIndent(t, newline))
                s.push_back(space + p + newline);
        }
        return s;
    }
    void emptyLines(int n = 1)
    {
        int e = 0;
        for (auto i = lines.rbegin(); i != lines.rend(); ++i)
        {
            if ((*i)->getText().empty())
                e++;
            else
                break;
        }
        if (e < n)
        {
            while (e++ != n)
                addLine();
        }
        else if (e > n)
        {
            lines.resize(lines.size() - (e - n));
        }
    }

    Emitter &operator+=(const Emitter &rhs)
    {
        auto addWithRelativeIndent = [this](Lines &l1, const Lines &l2)
        {
            for (auto &l : l2)
            {
                if (l->empty())
                    continue;
                l1.emplace_back(std::make_unique<detail::Line>(l->getText(), l->n_indents + n_indents));
            }
        };

        addWithRelativeIndent(lines, rhs.lines);
        return *this;
    }

    bool empty() const
    {
        return lines.empty();
    }

    virtual void clear()
    {
        lines.clear();
    }

    Lines &getLines() { return lines; }
    const Lines &getLines() const { return lines; }

    const Text &getNewline() const { return newline; }

protected:
    Lines lines;
    int insert_before = -1;

    int n_indents = 0;
    Text indent;
    Text newline;

private:
    void addLine(LinePtr &&l)
    {
        lines.emplace_back(std::move(l));
    }
};

struct CppEmitter : Emitter
{
    using Base = Emitter;

    void beginBlock(const Text &s = "", bool in_indent = true)
    {
        if (!s.empty())
            addLine(s);
        addLine("{");
        if (in_indent)
            increaseIndent();
    }
    void endBlock(bool semicolon = false)
    {
        decreaseIndent();
        emptyLines(0);
        addLine(semicolon ? "};" : "}");
    }
    void beginFunction(const Text &s = "")
    {
        beginBlock(s);
    }
    void endFunction()
    {
        endBlock();
        addLine();
    }
    void beginNamespace(const Text &s)
    {
        addLineWithoutIndent("namespace " + s);
        addLineWithoutIndent("{");
        addLine();
        namespaces.push(s);
    }
    void endNamespace(const Text &ns = Text())
    {
        Text s = ns;
        if (!namespaces.empty() && ns.empty())
        {
            s = namespaces.top();
            namespaces.pop();
        }
        addLineWithoutIndent("} // namespace " + s);
        addLine();
    }

    void ifdef(const Text &s)
    {
        addLineWithoutIndent("#ifdef " + s);
    }
    void endif()
    {
        addLineWithoutIndent("#endif");
    }

    virtual void clear()
    {
        namespaces = decltype(namespaces)();
        Base::clear();
    }

private:
    std::stack<Text> namespaces;
};

struct BinaryStream
{
    BinaryStream(){}
    BinaryStream(size_t size)
        : buf_(new std::vector<uint8_t>())
    {
        buf_->reserve(size);
        size_ = buf_->size();
        skip(0);
    }
    BinaryStream(const std::string &s)
        : buf_(new std::vector<uint8_t>(&s[0], &s[s.size()]))
    {
        skip(0);
        size_ = buf_->size();
        end_ = index_ + size_;
    }
    BinaryStream(const std::vector<uint8_t> &buf, size_t data_offset = 0)
        : buf_(new std::vector<uint8_t>(buf)), data_offset(data_offset)
    {
        skip(0);
        size_ = buf_->size();
        end_ = index_ + size_;
    }
    BinaryStream(const BinaryStream &rhs, size_t size)
        : buf_(rhs.buf_)
    {
        index_ = rhs.index_;
        data_offset = rhs.data_offset;
        ptr = rhs.ptr;
        size_ = size;
        end_ = index_ + size_;
        rhs.skip((int)size);
    }
    BinaryStream(const BinaryStream &rhs, size_t size, size_t offset)
        : buf_(rhs.buf_)
    {
        index_ = offset;
        data_offset = offset;
        size_ = size;
        ptr = (uint8_t *)buf_->data() + index_;
        end_ = index_ + size_;
    }

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
    size_t read(std::string &s)
    {
        s.clear();
        while (*ptr)
            s += *ptr++;
        skip((int)(s.size() + 1));
        return s.size();
    }

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

    bool has(size_t size) const
    {
        return index_ + size <= end_;
    }
    // setpos
    void seek(size_t size) const
    {
        reset();
        skip((int)size);
    }
    void skip(int n) const
    {
        if (!buf_)
            throw std::logic_error("BinaryContext: not initialized");
        index_ += n;
        data_offset += n;
        ptr = (uint8_t *)buf_->data() + index_;
    }
    size_t end() const { return end_; }
    bool eof() const
    {
        return index_ == end_;
    }
#pragma push_macro("check") // ue4 has this macro
#undef check
    bool check(int index) const
    {
        return index_ == index;
    }
#pragma pop_macro("check")
    void reset() const
    {
        index_ = 0;
        data_offset = 0;
        ptr = nullptr;
        if (buf_)
            ptr = (uint8_t *)buf_->data();
    }

    size_t index() const
    {
        return index_;
    }
    size_t size() const
    {
        return size_;
    }
    bool empty() const
    {
        return size_ == 0;
    }
    const std::vector<uint8_t> &buf() const
    {
        if (!buf_)
            throw std::logic_error("BinaryContext: not initialized");
        return *buf_;
    }

    const uint8_t *getPtr() const { return ptr; }

    size_t _read(void *dst, size_t size, size_t offset = 0) const
    {
        if (!buf_)
            throw std::logic_error("BinaryContext: not initialized");
        if (index_ >= end_)
            throw std::logic_error("BinaryContext: out of range");
        if (index_ + offset + size > end_)
            throw std::logic_error("BinaryContext: too much data");
        memcpy(dst, buf_->data() + index_ + offset, size);
        skip((int)(size + offset));
        return size;
    }
    size_t _write(const void *src, size_t size)
    {
        if (!buf_)
        {
            buf_ = std::make_shared<std::vector<uint8_t>>(size);
            end_ = size_ = buf_->size();
        }
        if (index_ > end_)
            throw std::logic_error("BinaryContext: out of range");
        if (index_ + size > end_)
        {
            buf_->resize(index_ + size);
            end_ = size_ = buf_->size();
        }
        memcpy((uint8_t *)buf_->data() + index_, src, size);
        skip((int)size);
        return size;
    }

    void load(const path &fn)
    {
        *this = read_file(fn);
    }
    void save(const path &fn)
    {
        ScopedFile f(fn, "wb");
        fwrite(buf_->data(), 1, data_offset, f.getHandle());
    }

protected:
    std::shared_ptr<std::vector<uint8_t>> buf_;
    mutable size_t index_ = 0;
    mutable uint8_t *ptr = 0;
    mutable size_t data_offset = 0;
    mutable size_t size_ = 0;
    size_t end_ = 0;
};

}
