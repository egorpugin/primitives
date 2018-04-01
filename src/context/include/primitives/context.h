#pragma once

#include <list>
#include <memory>
#include <string>
#include <stack>
#include <sstream>

class Context
{
public:
    using Text = std::string;

public:
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

public:
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

    void beginBlock(const Text &s = "", bool indent = true);
    void endBlock(bool semicolon = false);
    void beginFunction(const Text &s = "");
    void endFunction();
    void beginNamespace(const Text &s);
    void endNamespace(const Text &s = Text());

    void ifdef(const Text &s);
    void endif();

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

private:
    Lines lines;
    std::shared_ptr<Context> before_;
    std::shared_ptr<Context> after_;

    int n_indents = 0;
    Text indent;
    Text newline;
    std::stack<Text> namespaces;

    void copy_from(const Context &ctx);
};
