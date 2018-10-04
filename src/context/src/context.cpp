// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/context.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

Context::Lines &operator+=(Context::Lines &s1, const Context::Lines &s2)
{
    s1.insert(s1.end(), s2.begin(), s2.end());
    return s1;
}

Context::Context(const Text &indent, const Text &newline)
    : indent(indent), newline(newline)
{
}

Context::Context(const Context &ctx)
{
    copy_from(ctx);
}

Context &Context::operator=(const Context &ctx)
{
    copy_from(ctx);
    return *this;
}

void Context::copy_from(const Context &ctx)
{
    lines = ctx.lines;
    if (ctx.before_)
        before_ = std::make_shared<Context>(*ctx.before_.get());
    if (ctx.after_)
        after_ = std::make_shared<Context>(*ctx.after_.get());

    n_indents = ctx.n_indents;
    indent = ctx.indent;
    newline = ctx.newline;
    namespaces = ctx.namespaces;
}

void Context::initFromString(const std::string &s)
{
    size_t p = 0;
    while (1)
    {
        size_t p2 = s.find('\n', p);
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
    lines.push_back(Line{ s, n_indents });
}

void Context::addLineNoSpace(const Text & s)
{
    lines.push_back(Line{ s });
}

void Context::addLine(const Text &s)
{
    if (s.empty())
        lines.push_back(Line{});
    else
        lines.push_back({ s, n_indents });
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
    auto &t = lines.back().text;
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
        if (!line.text.empty())
            for (int i = 0; i < line.n_indents; i++)
                space += indent;
        s += space + line.text + newline;
    }
    return s;
}

Context::Lines Context::getLines() const
{
    Lines lines;
    if (before_)
        lines += before_->getLines();
    lines += this->lines;
    if (after_)
        lines += after_->getLines();
    return lines;
}

void Context::setLines(const Lines &lines)
{
    before_.reset();
    after_.reset();
    this->lines = lines;
}

void Context::mergeBeforeAndAfterLines()
{
    if (before_)
    {
        before_->mergeBeforeAndAfterLines();
        lines.insert(lines.begin(), before_->getLinesRef().begin(), before_->getLinesRef().end());
        before_.reset();
    }
    if (after_)
    {
        after_->mergeBeforeAndAfterLines();
        lines.insert(lines.end(), after_->getLinesRef().begin(), after_->getLinesRef().end());
        after_.reset();
    }
}

void Context::setMaxEmptyLines(int n)
{
    // remove all empty lines at begin
    while (1)
    {
        auto line = lines.begin();
        if (line == lines.end())
            break;
        bool empty = true;
        for (auto &c : line->text)
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
        for (auto &c : line->text)
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

void Context::splitLines()
{
    for (auto line = lines.begin(); line != lines.end(); ++line)
    {
        auto &text = line->text;
        auto p = text.find('\n');
        if (p == text.npos)
            continue;

        size_t old_pos = 0;
        Lines ls;
        while (1)
        {
            ls.push_back(Line{ text.substr(old_pos, p - old_pos), line->n_indents });
            p++;
            old_pos = p;
            p = text.find('\n', p);
            if (p == text.npos)
            {
                ls.push_back(Line{ text.substr(old_pos), line->n_indents });
                break;
            }
        }
        lines.insert(line, ls.begin(), ls.end());
        line = lines.erase(line);
        line--;
    }
}

void Context::emptyLines(int n)
{
    int e = 0;
    for (auto i = lines.rbegin(); i != lines.rend(); ++i)
    {
        if (i->text.empty())
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
    if (before_ && rhs.before_)
        before_->lines += rhs.before_->lines;
    else if (rhs.before_)
    {
        before().lines += rhs.before_->lines;
    }
    lines += rhs.lines;
    if (after_ && rhs.after_)
        after_->lines += rhs.after_->lines;
    else if (rhs.after_)
    {
        after().lines += rhs.after_->lines;
    }
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

    if (before_ && rhs.before_)
        addWithRelativeIndent(before_->lines, rhs.before_->lines);
    else if (rhs.before_)
    {
        addWithRelativeIndent(before().lines, rhs.before_->lines);
    }
    addWithRelativeIndent(lines, rhs.lines);
    if (after_ && rhs.after_)
        addWithRelativeIndent(after_->lines, rhs.after_->lines);
    else if (rhs.after_)
    {
        addWithRelativeIndent(after().lines, rhs.after_->lines);
    }
}

void Context::printToFile(FILE* out) const
{
    fprintf(out, "%s", getText().c_str());
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
    rhs.skip(size);
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
    skip(size + offset);
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
    skip(size);
    return size;
}

size_t BinaryContext::read(std::string &s)
{
    s.clear();
    while (*ptr)
        s += *ptr++;
    skip(s.size() + 1);
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
    skip(size);
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
