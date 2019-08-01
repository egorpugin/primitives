// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/emitter.h>

#include <primitives/exceptions.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

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

namespace primitives
{

namespace detail
{

Line::Line(const Text &t, int n)
    : text(t), n_indents(n)
{
}

Line::~Line()
{
}

Line& Line::operator+=(const Text &t)
{
    text += t;
    return *this;
}

void Line::setText(const Text &t)
{
    text = t;
}

Line::Text Line::getText() const
{
    return text;
}

}

Emitter::Emitter(const Text &indent, const Text &newline)
    : indent(indent), newline(newline)
{
}

Emitter::~Emitter()
{
}

void Emitter::addText(const Text &s)
{
    if (lines.empty())
        lines.emplace_back();
    if (!s.empty() && lines.back()->n_indents == 0)
        lines.back()->n_indents = n_indents;
    lines.back()->setText(lines.back()->getText() += s);
}

void Emitter::addText(const char* str, int n)
{
    addText(Text(str, str + n));
}

void Emitter::addNoNewLine(const Text &s)
{
    addLineWithIndent(s);
}

void Emitter::addLineWithIndent(const Text &s)
{
    addLineWithIndent(s, 0);
}

void Emitter::addLineWithIndent(const Text &text, int n)
{
    for (auto &s : splitWithIndent(text, newline))
        addLine(std::make_unique<Line>(s, n + n_indents));
}

void Emitter::addLineNoSpace(const Text &s)
{
    addLineWithIndent(s, 0);
}

void Emitter::addLine(LinePtr &&l)
{
    lines.emplace_back(std::move(l));
}

void Emitter::addLine(const Text &s)
{
    if (s.empty())
        addLine(std::make_unique<Line>());
    else
        addLineWithIndent(s);
}

void Emitter::removeLine()
{
    removeLines(1);
}

void Emitter::removeLines(int n)
{
    n = std::max(0, (int)lines.size() - n);
    lines.resize(n);
}

void Emitter::increaseIndent(int n)
{
    n_indents += n;
}

void Emitter::decreaseIndent(int n)
{
    n_indents -= n;
}

void Emitter::increaseIndent(const Text &s, int n)
{
    addLine(s);
    increaseIndent(n);
}

void Emitter::decreaseIndent(const Text &s, int n)
{
    decreaseIndent(n);
    addLine(s);
}

void Emitter::trimEnd(size_t n)
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

Emitter::Text Emitter::getText() const
{
    Text s;
    for (auto &line : getStrings())
        s += line;
    return s;
}

Strings Emitter::getStrings() const
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
        for (auto &p : splitWithIndent(t, newline))
            s.push_back(space + p + newline);
    }
    return s;
}

void Emitter::emptyLines(int n)
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

Emitter &Emitter::operator+=(const Emitter &rhs)
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

void CppEmitter::beginBlock(const Text &s, bool indent)
{
    if (!s.empty())
        addLine(s);
    addLine("{");
    if (indent)
        increaseIndent();
}

void CppEmitter::endBlock(bool semicolon)
{
    decreaseIndent();
    emptyLines(0);
    addLine(semicolon ? "};" : "}");
}

void CppEmitter::beginFunction(const Text &s)
{
    beginBlock(s);
}

void CppEmitter::endFunction()
{
    endBlock();
    addLine();
}

void CppEmitter::beginNamespace(const Text &s)
{
    addLineNoSpace("namespace " + s);
    addLineNoSpace("{");
    addLine();
    namespaces.push(s);
}

void CppEmitter::endNamespace(const Text &ns)
{
    Text s = ns;
    if (!namespaces.empty() && ns.empty())
    {
        s = namespaces.top();
        namespaces.pop();
    }
    addLineNoSpace("} // namespace " + s);
    addLine();
}

void CppEmitter::ifdef(const Text &s)
{
    addLineNoSpace("#ifdef " + s);
}

void CppEmitter::endif()
{
    addLineNoSpace("#endif");
}

BinaryStream::BinaryStream()
{
}

BinaryStream::BinaryStream(size_t size)
    : buf_(new std::vector<uint8_t>())
{
    buf_->reserve(size);
    size_ = buf_->size();
    skip(0);
}

BinaryStream::BinaryStream(const std::string &s)
    : buf_(new std::vector<uint8_t>(&s[0], &s[s.size()]))
{
    skip(0);
    size_ = buf_->size();
    end_ = index_ + size_;
}

BinaryStream::BinaryStream(const std::vector<uint8_t> &buf, size_t data_offset)
    : buf_(new std::vector<uint8_t>(buf)), data_offset(data_offset)
{
    skip(0);
    size_ = buf_->size();
    end_ = index_ + size_;
}

BinaryStream::BinaryStream(const BinaryStream &rhs, size_t size)
    : buf_(rhs.buf_)
{
    index_ = rhs.index_;
    data_offset = rhs.data_offset;
    ptr = rhs.ptr;
    size_ = size;
    end_ = index_ + size_;
    rhs.skip((int)size);
}

BinaryStream::BinaryStream(const BinaryStream &rhs, size_t size, size_t offset)
    : buf_(rhs.buf_)
{
    index_ = offset;
    data_offset = offset;
    size_ = size;
    ptr = (uint8_t *)buf_->data() + index_;
    end_ = index_ + size_;
}

size_t BinaryStream::_read(void *dst, size_t size, size_t offset) const
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

bool BinaryStream::has(size_t size) const
{
    return index_ + size <= end_;
}

size_t BinaryStream::_write(const void *src, size_t size)
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

size_t BinaryStream::read(std::string &s)
{
    s.clear();
    while (*ptr)
        s += *ptr++;
    skip((int)(s.size() + 1));
    return s.size();
}

void BinaryStream::skip(int n) const
{
    if (!buf_)
        throw std::logic_error("BinaryContext: not initialized");
    index_ += n;
    data_offset += n;
    ptr = (uint8_t *)buf_->data() + index_;
}

void BinaryStream::reset() const
{
    index_ = 0;
    data_offset = 0;
    ptr = nullptr;
    if (buf_)
        ptr = (uint8_t *)buf_->data();
}

void BinaryStream::seek(size_t size) const
{
    reset();
    skip((int)size);
}

bool BinaryStream::check(int index) const
{
    return index_ == index;
}

bool BinaryStream::eof() const
{
    return index_ == end_;
}

size_t BinaryStream::index() const
{
    return index_;
}

size_t BinaryStream::size() const
{
    return size_;
}

bool BinaryStream::empty() const
{
    return size_ == 0;
}

const std::vector<uint8_t> &BinaryStream::buf() const
{
    if (!buf_)
        throw std::logic_error("BinaryContext: not initialized");
    return *buf_;
}

void BinaryStream::load(const path &fn)
{
    *this = read_file(fn);
}

void BinaryStream::save(const path &fn)
{
    ScopedFile f(fn, "wb");
    fwrite(buf_->data(), 1, data_offset, f.getHandle());
}

}
