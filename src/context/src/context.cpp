// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/context.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

primitives::Context::Lines &operator+=(primitives::Context::Lines &s1, const primitives::Context::Lines &s2)
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

Line::Line(const Context &ctx, int n)
    : context(&ctx), n_indents(n)
{
    context->parent_ = this;
}

Line::Line(Line &&l)
{
    text = std::move(l.text);
    n_indents = l.n_indents;
    context = l.context;
    l.context = nullptr;
}

Line::~Line()
{
    if (context)
        context->parent_ = nullptr;
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
    if (context)
        return context->lines.back().back();
    return text;
}

Line::Text Line::getText() const
{
    if (context)
        return context->getText();
    return text;
}

}

Context::Context(const Text &indent, const Text &newline)
    : indent(indent), newline(newline)
{
}

/*Context::Context(const Context &ctx)
{
    copy_from(ctx);
}

Context &Context::operator=(const Context &ctx)
{
    copy_from(ctx);
    return *this;
}*/

Context::~Context()
{
    if (parent_)
    {
        parent_->setText(getText());
        parent_->context = nullptr;
    }
}

/*void Context::copy_from(const Context &ctx)
{
    lines = ctx.lines;
    n_indents = ctx.n_indents;
    indent = ctx.indent;
    newline = ctx.newline;
}*/

void Context::initFromString(const std::string &s)
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

void Context::addText(const Text &s)
{
    if (lines.empty())
        lines.emplace_back();
    lines.back() += s;
}

void Context::addText(const char* str, int n)
{
    addText(Text(str, str + n));
}

void Context::addNoNewLine(const Text &s)
{
    addLineWithIndent(s);
}

void Context::addLineWithIndent(const Text &s)
{
    addLineWithIndent(s, n_indents);
}

void Context::addLineWithIndent(const Text &text, int n)
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

void Context::addLineNoSpace(const Text &s)
{
    addLineWithIndent(s, 0);
}

void Context::addLine(Line &&l)
{
    lines.emplace_back(std::move(l));
}

void Context::addLine(const Text &s)
{
    if (s.empty())
        addLine(Line{});
    else
        addLineWithIndent(s);
}

void Context::addLine(const Context &ctx)
{
    addContext(ctx);
}

void Context::addContext(const Context &ctx)
{
    addLine(Line{ ctx, n_indents });
}

void Context::removeLine()
{
    removeLines(1);
}

void Context::removeLines(int n)
{
    n = std::max(0, (int)lines.size() - n);
    lines.resize(n);
}

void Context::increaseIndent(int n)
{
    n_indents += n;
}

void Context::decreaseIndent(int n)
{
    n_indents -= n;
}

void Context::increaseIndent(const Text &s, int n)
{
    addLine(s);
    increaseIndent(n);
}

void Context::decreaseIndent(const Text &s, int n)
{
    decreaseIndent(n);
    addLine(s);
}

void Context::trimEnd(size_t n)
{
    if (lines.empty())
        return;
    auto &t = lines.back().back();
    auto sz = t.size();
    if (n > sz)
        n = sz;
    t.resize(sz - n);
}

Context::Text Context::getText() const
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
        else if (line.context)
            continue; // we do not add empty line on empty existing context
        s += space + line.getText() + newline;
    }
    return s;
}

Context::Lines Context::getLines() const
{
    Lines lines;
    lines += this->lines;
    for (auto i = lines.begin(); i != lines.end(); i++)
    {
        if (i->context)
        {
            auto l2 = i->context->getLines();
            for (auto &l : l2)
                l.n_indents += i->n_indents;
            i = lines.erase(i);
            i = lines.insert(i, l2.begin(), l2.end());
            i--;
        }
    }
    return lines;
}

/*void Context::setLines(const Lines &lines)
{
    this->lines = lines;
}*/

void Context::setMaxEmptyLines(int n)
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

void Context::emptyLines(int n)
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

Context &Context::operator+=(const Context &rhs)
{
    lines += rhs.lines;
    return *this;
}

void Context::addWithRelativeIndent(const Context &rhs)
{
    auto addWithRelativeIndent = [this](Lines &l1, Lines l2)
    {
        for (auto &l : l2)
            l.n_indents += n_indents;
        l1 += l2;
    };

    addWithRelativeIndent(lines, rhs.lines);
}

void CppContext::beginBlock(const Text &s, bool indent)
{
    if (!s.empty())
        addLine(s);
    addLine("{");
    if (indent)
        increaseIndent();
}

void CppContext::endBlock(bool semicolon)
{
    decreaseIndent();
    emptyLines(0);
    addLine(semicolon ? "};" : "}");
}

void CppContext::beginFunction(const Text &s)
{
    beginBlock(s);
}

void CppContext::endFunction()
{
    endBlock();
    addLine();
}

void CppContext::beginNamespace(const Text &s)
{
    addLineNoSpace("namespace " + s);
    addLineNoSpace("{");
    addLine();
    namespaces.push(s);
}

void CppContext::endNamespace(const Text &ns)
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

void CppContext::ifdef(const Text &s)
{
    addLineNoSpace("#ifdef " + s);
}

void CppContext::endif()
{
    addLineNoSpace("#endif");
}

BinaryContext::BinaryContext()
{
}

BinaryContext::BinaryContext(size_t size)
    : buf_(new std::vector<uint8_t>())
{
    buf_->reserve(size);
    size_ = buf_->size();
    skip(0);
}

BinaryContext::BinaryContext(const std::string &s)
    : buf_(new std::vector<uint8_t>(&s[0], &s[s.size()]))
{
    skip(0);
    size_ = buf_->size();
    end_ = index_ + size_;
}

BinaryContext::BinaryContext(const std::vector<uint8_t> &buf, size_t data_offset)
    : buf_(new std::vector<uint8_t>(buf)), data_offset(data_offset)
{
    skip(0);
    size_ = buf_->size();
    end_ = index_ + size_;
}

BinaryContext::BinaryContext(const BinaryContext &rhs, size_t size)
    : buf_(rhs.buf_)
{
    index_ = rhs.index_;
    data_offset = rhs.data_offset;
    ptr = rhs.ptr;
    size_ = size;
    end_ = index_ + size_;
    rhs.skip((int)size);
}

BinaryContext::BinaryContext(const BinaryContext &rhs, size_t size, size_t offset)
    : buf_(rhs.buf_)
{
    index_ = offset;
    data_offset = offset;
    size_ = size;
    ptr = (uint8_t *)buf_->data() + index_;
    end_ = index_ + size_;
}

size_t BinaryContext::_read(void *dst, size_t size, size_t offset) const
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

size_t BinaryContext::_write(const void *src, size_t size)
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

size_t BinaryContext::read(std::string &s)
{
    s.clear();
    while (*ptr)
        s += *ptr++;
    skip((int)(s.size() + 1));
    return s.size();
}

void BinaryContext::skip(int n) const
{
    if (!buf_)
        throw std::logic_error("BinaryContext: not initialized");
    index_ += n;
    data_offset += n;
    ptr = (uint8_t *)buf_->data() + index_;
}

void BinaryContext::reset() const
{
    index_ = 0;
    data_offset = 0;
    ptr = (uint8_t *)buf_->data();
}

void BinaryContext::seek(size_t size) const
{
    reset();
    skip((int)size);
}

bool BinaryContext::check(int index) const
{
    return index_ == index;
}

bool BinaryContext::eof() const
{
    return index_ == end_;
}

size_t BinaryContext::index() const
{
    return index_;
}

size_t BinaryContext::size() const
{
    return size_;
}

const std::vector<uint8_t> &BinaryContext::buf() const
{
    if (!buf_)
        throw std::logic_error("BinaryContext: not initialized");
    return *buf_;
}

void BinaryContext::load(const path &fn)
{
    *this = read_file(fn);
}

void BinaryContext::save(const path &fn)
{
    ScopedFile f(fn, "wb");
    fwrite(buf_->data(), 1, data_offset, f.getHandle());
}

}
