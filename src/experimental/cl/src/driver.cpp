// Copyright (C) 2020 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "driver.h"

#include <primitives/cl.h>

MY_PARSER::MY_PARSER(const ::primitives::cl::ProgramDescription &pd)
    : pd(pd)
{
}

MY_LEXER_FUNCTION_SIGNATURE_CLASS(MY_PARSER)
{
    if (argi == 0)
    {
        if (pd.args[argi].empty())
            throw SW_RUNTIME_ERROR("Empty program");
        print_argument_delimeter = true;
        LEXER_RETURN_VALUE ret = MAKE_VALUE(STRING, pd.args[argi++]);
        MY_LEXER_FUNCTION_EPILOGUE;
    }

    if (print_argument_delimeter)
    {
        if (argi == pd.args.size())
        {
            LEXER_RETURN_VALUE ret = MAKE(EOQ);
            MY_LEXER_FUNCTION_EPILOGUE;
        }

        if (argi < pd.args.size())
            set_next_buf();

        print_argument_delimeter = false;
        loc->argument++;
        LEXER_RETURN_VALUE ret = MAKE(ARGUMENT_DELIMETER);
        MY_LEXER_FUNCTION_EPILOGUE;
    }

    LEXER_RETURN_VALUE ret = ll_cllex(bb.scanner, *loc);
    if (std::get<0>(ret) == THIS_PARSER2::token::EOQ && argi < pd.args.size())
    {
        set_next_buf();
        loc->argument++;
        ret = MAKE(ARGUMENT_DELIMETER);
    }
    MY_LEXER_FUNCTION_EPILOGUE;
}

int MY_PARSER::parse()
{
    //set_debug_level(1);
    MY_PARSER_PARSE_FUNCTION_PROLOGUE;
    MY_PARSER_PARSE_FUNCTION_EPILOGUE;
}

void MY_PARSER::set_next_buf()
{
    LEXER_NAME(_scan_string)(pd.args[argi++].c_str(), bb.scanner);
}

std::ostream &operator<<(std::ostream &o, const MyLocation &loc)
{
    o << loc.argument << ":" << (yy_cl::location&)loc;
    return o;
}
