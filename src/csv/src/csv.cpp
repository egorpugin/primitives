// Copyright (C) 2020 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/csv.h>

#include <primitives/exceptions.h>

namespace primitives::csv
{

namespace
{

struct Parser
{
    enum class Token
    {
        Delimeter,
        Eol,
        Quote,
        Escape,
        Character,
    };

    Parser(const std::string &in, char delimeter = ',', char quote = '"', char escape = '\\')
        : s(in), delimeter(delimeter), quote(quote), escape(escape)
    {
        p = s.data() - 1;
        nextsym();
    }

    Columns parse()
    {
        fields();
        expect(Token::Eol);
        return cols;
    }

private:
    Columns cols;
    const std::string &s;
    char delimeter;
    char quote;
    char escape;
    const char *p;
    Token sym;

    // grammar
    void fields()
    {
        field();
        while (accept(Token::Delimeter))
            field();
    }

    void field()
    {
        if (accept(Token::Quote))
        {
            quoted_field();
            expect(Token::Quote);
        }
        else
            normal_field();
    }

    void normal_field()
    {
        std::string buf;
        bool has_chars = false;
        while (accept(Token::Character))
        {
            buf += *(p - 1);
            has_chars = true;
        }
        if (has_chars)
            cols.push_back(buf);
        else
            cols.push_back({});
    }

    void quoted_field()
    {
        std::string buf;
        while (0
            || accept(Token::Character)
            || accept(Token::Delimeter)
            || accept_quoted_symbol()
            )
        {
            buf += *(p - 1);
        }
        cols.push_back(buf);
    }

    // lexer and misc
    bool accept(Token s)
    {
        if (sym == s)
        {
            nextsym();
            return true;
        }
        return false;
    }

    bool expect(Token s)
    {
        if (accept(s))
            return true;
        throw SW_RUNTIME_ERROR("Unexpected token: " + std::to_string((int)sym));
    }

    bool accept_quoted_symbol()
    {
        auto tok_escape = Token::Escape;
        if (quote == escape)
            tok_escape = Token::Quote;
        if (sym != tok_escape)
        {
            return false;
        }
        if (nextsym1() != Token::Quote)
        {
            p--;
            return false;
        }
        nextsym();
        return true;
    }

    void nextsym()
    {
        sym = nextsym1();
    }

    Token nextsym1()
    {
        p++;
        if (*p == '\0')
            return Token::Eol;
        if (*p == delimeter)
            return Token::Delimeter;
        if (*p == quote)
            return Token::Quote;
        if (*p == escape)
            return Token::Escape;
        return Token::Character;
    }
};

}

std::vector<std::optional<std::string>> parse_line(const std::string &s,
    char delimeter, char quote, char escape)
{
    Parser p(s, delimeter, quote, escape);
    return p.parse();
}

}
