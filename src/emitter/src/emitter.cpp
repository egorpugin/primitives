// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/emitter.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

primitives::Emitter::Lines &operator+=(primitives::Emitter::Lines &s1, const primitives::Emitter::Lines &s2)
{
    s1.insert(s1.end(), s2.begin(), s2.end());
    return s1;
}

namespace primitives
{

namespace detail
{

Line::Line(const Text &t, int n)
    : text(t), n_indents(n)
{}

Line::Line(const Emitter &ctx, int n)
    : emitter(&ctx), n_indents(n)
{
    emitter->parent_ = this;
}

Line::Line(Line &&l)
{
    text = std::move(l.text);
    n_indents = l.n_indents;
    emitter = l.emitter;
    l.emitter = nullptr;
}

Line::~Line()
{
    if (emitter)
        emitter->parent_ = nullptr;
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

Line::Text &Line::back()
{
    if (emitter)
        return emitter->lines.back().back();
    return text;
}

Line::Text Line::getText() const
{
    if (emitter)
        return emitter->getText();
    return text;
}

}

Emitter::Emitter(const Text &indent, const Text &newline)
    : indent(indent), newline(newline)
{
}

/*Emitter::Emitter(const Emitter &ctx)
{
    copy_from(ctx);
}

Emitter &Emitter::operator=(const Emitter &ctx)
{
    copy_from(ctx);
    return *this;
}*/

Emitter::~Emitter()
{
    if (parent_)
    {
        parent_->setText(getText());
        parent_->emitter = nullptr;
    }
}

/*void Emitter::copy_from(const Emitter &ctx)
{
    lines = ctx.lines;
    n_indents = ctx.n_indents;
    indent = ctx.indent;
    newline = ctx.newline;
}*/

void Emitter::initFromString(const std::string &s)
{
    size_t p = 0;
    while (1)
    {
        size_t p2 = s.find(newline, p);
        if (p2 == s.npos)
            break;
        auto line = s.substr(p, p2 - p);
        int space = 0;
        for (auto i = line.rbegin(); i != line.rend(); ++i)
        {
            if (isspace(*i))
                space++;
            else
                break;
        }
        if (space)
            line.resize(line.size() - space);
        lines.push_back({ line });
        p = p2 + 1;
    }
}

void Emitter::addText(const Text &s)
{
    if (lines.empty())
        lines.emplace_back();
    if (!s.empty() && lines.back().n_indents == 0)
        lines.back().n_indents = n_indents;
    lines.back() += s;
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
    addLineWithIndent(s, n_indents);
}

void Emitter::addLineWithIndent(const Text &text, int n)
{
    auto p = text.find(newline);
    if (p == text.npos)
    {
        addLine(Line{ text, n });
        return;
    }

    size_t old_pos = 0;
    Lines ls;
    while (1)
    {
        ls.push_back(Line{text.substr(old_pos, p - old_pos), n});
        p++;
        old_pos = p;
        p = text.find(newline, p);
        if (p == text.npos)
        {
            ls.push_back(Line{text.substr(old_pos), n});
            break;
        }
    }
    lines.insert(lines.end(), ls.begin(), ls.end());
}

void Emitter::addLineNoSpace(const Text &s)
{
    addLineWithIndent(s, 0);
}

void Emitter::addLine(Line &&l)
{
    lines.emplace_back(std::move(l));
}

void Emitter::addLine(const Text &s)
{
    if (s.empty())
        addLine(Line{});
    else
        addLineWithIndent(s);
}

void Emitter::addLine(const Emitter &ctx)
{
    addEmitter(ctx);
}

void Emitter::addEmitter(const Emitter &ctx)
{
    addLine(Line{ ctx, n_indents });
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
    auto &t = lines.back().back();
    auto sz = t.size();
    if (n > sz)
        n = sz;
    t.resize(sz - n);
}

Emitter::Text Emitter::getText() const
{
    Text s;
    auto lines = getLines();
    for (auto &line : lines)
    {
        Text space;
        if (!line.getText().empty())
        {
            for (int i = 0; i < line.n_indents; i++)
                space += indent;
        }
        else if (line.emitter)
            continue; // we do not add empty line on empty existing emitter
        s += space + line.getText() + newline;
    }
    return s;
}

Emitter::Lines Emitter::getLines() const
{
    Lines lines;
    lines += this->lines;
    for (auto i = lines.begin(); i != lines.end(); i++)
    {
        if (i->emitter)
        {
            auto l2 = i->emitter->getLines();
            for (auto &l : l2)
                l.n_indents += i->n_indents;
            i = lines.erase(i);
            i = lines.insert(i, l2.begin(), l2.end());
            i--;
        }
    }
    return lines;
}

/*void Emitter::setLines(const Lines &lines)
{
    this->lines = lines;
}*/

void Emitter::setMaxEmptyLines(int n)
{
    // remove all empty lines at begin
    while (1)
    {
        auto line = lines.begin();
        if (line == lines.end())
            break;
        bool empty = true;
        for (auto &c : line->getText())
        {
            if (!isspace(c))
            {
                empty = false;
                break;
            }
        }
        if (empty)
            lines.erase(line);
        else
            break;
    }

    int el = 0;
    for (auto line = lines.begin(); line != lines.end(); ++line)
    {
        bool empty = true;
        for (auto &c : line->getText())
        {
            if (!isspace(c))
            {
                empty = false;
                break;
            }
        }
        if (empty)
            el++;
        else
            el = 0;
        if (el > n)
        {
            line = lines.erase(line);
            --line;
        }
    }
}

void Emitter::emptyLines(int n)
{
    int e = 0;
    for (auto i = lines.rbegin(); i != lines.rend(); ++i)
    {
        if (i->getText().empty())
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
    addWithRelativeIndent(rhs);
    return *this;
}

void Emitter::addWithRelativeIndent(const Emitter &rhs)
{
    auto addWithRelativeIndent = [this](Lines &l1, Lines l2)
    {
        for (auto &l : l2)
            l.n_indents += n_indents;
        l1 += l2;
    };

    addWithRelativeIndent(lines, rhs.lines);
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
