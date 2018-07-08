%{
// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <settings_parser.h>

static String remove_delims(const String &s)
{
    String s2;
    for (auto &c : s)
        if (isxdigit(c))
            s2 += c;
    return s2;
}

static int64_t str2int(const String &s, int base, bool has_delims = false)
{
    int has_sign = s[0] == '-' || s[0] == '+';
    int64_t v;
    switch (base)
    {
    case 10:
        v = std::stoll(remove_delims(s));
        break;
    case 16:
        v = std::stoll(remove_delims(s.substr(2 + has_sign)), 0, 16);
        break;
    case 8:
    {
        int has_o = has_sign ? s[2] == 'o' : s[1] == '0';
        if (has_delims)
            v = std::stoll(remove_delims(s.substr(1 + has_sign + has_o)), 0, 8);
        else
            v = std::stoll(s.substr(1 + has_sign + has_o), 0, 8);
    }
        break;
    case 2:
        if (has_delims)
            v = std::stoll(remove_delims(s.substr(2 + has_sign)), 0, 2);
        else
            v = std::stoll(s.substr(2 + has_sign), 0, 2);
        break;
    }
    return s[0] == '-' ? -v : v;
}

%}

%option nounistd
%option nonoline
%option yylineno
%option nounput
%option batch
%option never-interactive
%option reentrant
%option noyywrap

bare [[:alnum:]_-]*

bare_rhs_beg [\x00-\xFF]{-}[[:cntrl:] \"\']
bare_rhs_end [\x00-\xFF]{-}[[:cntrl:] ]
bare_rhs_any [\x00-\xFF]{-}[[:cntrl:]]{+}[[:blank:]]

newline \r?\n
space [[:blank:]]
space_or_newline {space}|{newline}

bin [0-1]
oct [0-7]
dec [[:digit:]]
hex [[:xdigit:]]

hex_prefix 0[xX]

delim [_\']
bin_delim {bin}({delim}?{bin})*
oct_delim {oct}({delim}?{oct})*
dec_delim {dec}({delim}?{dec})*
hex_delim {hex}({delim}?{hex})*

float_dec_point1    \.{dec}+
float_dec_point2    {dec}+(\.{dec}*)?
float_dec_point     {float_dec_point1}|{float_dec_point2}
float_dec           {float_dec_point}([eE][\+-]?{dec}+)?

float_hex_point1    \.{hex}+
float_hex_point2    {hex}+(\.{hex}*)?
float_hex_point     {float_hex_point1}|{float_hex_point2}
float_hex           {hex_prefix}{float_hex_point}([pP][\+-]?{dec}+)?

inf                 (?i:inf)|(?i:infinite)
nan                 (?i:nan)(\([[:alnum:]_]+\))?
float               [\+-]?({float_dec}|{float_hex}|{inf}|{nan})

%x LL_BASIC LL_LITERAL
%x LL_MULTILINE_BASIC LL_MULTILINE_LITERAL
%x LL_VALUE

%%

%{
    // Code run each time yylex is called.
    loc.step();
%}

<INITIAL,LL_VALUE>{space}+  { loc.step(); }
{space_or_newline}+         { loc.lines(yyleng); return MAKE(NEWLINE); }

"["                     { return MAKE(LEFT_SQUARE_BRACKET); }
"]"                     { return MAKE(RIGHT_SQUARE_BRACKET); }
<LL_VALUE>"{"           { BEGIN(0); return MAKE(LEFT_CURLY_BRACKET); }
"}"                     { return MAKE(RIGHT_CURLY_BRACKET); }

\=|:                    { BEGIN(LL_VALUE); return MAKE(KV_DELIMETER); }
\.                      { return MAKE(POINT); }

#.*                     ;

<LL_VALUE>{

    [\+-]?0b{bin}+{space}*            { BEGIN(0); return MAKE_VALUE(VALUE_INTEGER, str2int({yytext, yytext + yyleng}, 2)); }
    [\+-]?0b{bin_delim}{space}*       { BEGIN(0); return MAKE_VALUE(VALUE_INTEGER, str2int({yytext, yytext + yyleng}, 2, true)); }
    [\+-]?0o?{oct}+{space}*           { BEGIN(0); return MAKE_VALUE(VALUE_INTEGER, str2int({yytext, yytext + yyleng}, 8)); }
    [\+-]?0o?{oct_delim}{space}*      { BEGIN(0); return MAKE_VALUE(VALUE_INTEGER, str2int({yytext, yytext + yyleng}, 8, true)); }
    [\+-]?{dec}+{space}*              { BEGIN(0); return MAKE_VALUE(VALUE_INTEGER, std::stoll(yytext)); }
    [\+-]?{dec_delim}{space}*         { BEGIN(0); return MAKE_VALUE(VALUE_INTEGER, str2int({yytext, yytext + yyleng}, 10, true)); }
    [\+-]?{hex_prefix}{hex}+{space}*       { BEGIN(0); return MAKE_VALUE(VALUE_INTEGER, std::stoll(yytext, 0, 16)); }
    [\+-]?{hex_prefix}{hex_delim}{space}*  { BEGIN(0); return MAKE_VALUE(VALUE_INTEGER, str2int({yytext, yytext + yyleng}, 16, true)); }

    {float}               { BEGIN(0); return MAKE_VALUE(VALUE_DOUBLE, std::stod(yytext)); }

    true{space}*          { BEGIN(0); return MAKE_VALUE(VALUE_BOOL, true); }
    false{space}*         { BEGIN(0); return MAKE_VALUE(VALUE_BOOL, false); }

    {bare_rhs_beg}({bare_rhs_any}*{bare_rhs_end})? { BEGIN(0); return MAKE_VALUE(BARE_RHS_STRING, yytext); }

}

{bare}                  { return MAKE_VALUE(BARE_STRING, yytext); }

<INITIAL,LL_VALUE>\"    { BEGIN(LL_BASIC); driver.str.clear(); return MAKE(START_BASIC_STRING); }
<LL_BASIC,LL_MULTILINE_BASIC>{
    [\x00-\xFF]{-}[[:cntrl:]\"] { driver.str += *yytext; }
    \\[^uU\r\n] {
        auto c = primitives::detail::unescapeSettingChar(*(yytext + 1));
        if (c == 0)
            return MAKE(ERROR_SYMBOL);
        driver.str += c;
    }
    \\u[[:xdigit:]]{4} {
        int num = 0;
        sscanf(yytext + 2, "%04x", &num);
        driver.str += primitives::detail::cp2utf8(num);
    }
    \\U[[:xdigit:]]{8} {
        int num = 0;
        sscanf(yytext + 2, "%08x", &num);
        driver.str += primitives::detail::cp2utf8(num);
    }
}
<LL_BASIC>\" {
    BEGIN(0);
    return MAKE_VALUE(BASIC_STRING, driver.str);
}

<INITIAL,LL_VALUE>\"\"\"{newline}?  { BEGIN(LL_MULTILINE_BASIC); driver.str.clear(); return MAKE(START_MULTILINE_BASIC_STRING); }
<LL_MULTILINE_BASIC>{
    {newline}       { driver.str += '\n'; }
    \"              { driver.str += *yytext; }
    \\{newline}({space_or_newline}*) ;
    \"\"\" {
        BEGIN(0);
        return MAKE_VALUE(MULTILINE_BASIC_STRING, driver.str);
    }
}

<INITIAL,LL_VALUE>\'    { BEGIN(LL_LITERAL); driver.str.clear(); return MAKE(START_LITERAL_STRING); }
<LL_LITERAL,LL_MULTILINE_LITERAL>{
    ([\x00-\xFF]{-}[[:cntrl:]\'])* { driver.str += yytext; }
    \t { driver.str += *yytext; }
}
<LL_LITERAL>\'          { BEGIN(0); return MAKE_VALUE(LITERAL_STRING, driver.str); }

<INITIAL,LL_VALUE>\'\'\'{newline}?  { BEGIN(LL_MULTILINE_LITERAL); driver.str.clear(); return MAKE(START_MULTILINE_LITERAL_STRING); }
<LL_MULTILINE_LITERAL>{
    {newline}     { driver.str += '\n'; }
    \'            { driver.str += *yytext; }
    \'\'\'        { BEGIN(0); return MAKE_VALUE(MULTILINE_LITERAL_STRING, driver.str); }
}

<*>(?s:.)               { driver.bb.bad_symbol = yytext; return MAKE(ERROR_SYMBOL); }
<*><<EOF>>              return MAKE(EOQ);

%%
