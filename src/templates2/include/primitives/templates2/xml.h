#pragma once

#include <pugixml.hpp>
#include <tidy.h>
#include <tidybuffio.h>

auto tidy_html(auto &&s) {
    TidyDoc tidyDoc = tidyCreate();
    SCOPE_EXIT { tidyRelease(tidyDoc); };
    TidyBuffer tidyOutputBuffer = {0};
    1
        && tidyOptSetBool(tidyDoc, TidyXmlOut, yes)
        && tidyOptSetBool(tidyDoc, TidyQuiet, yes)
        //&& tidyOptSetBool(tidyDoc, TidyQuoteNbsp, yes)
        //&& tidyOptSetBool(tidyDoc, TidyNumEntities, yes)
        //&& tidyOptSetBool(tidyDoc, TidyXmlTags, yes)
        //&& tidyOptSetBool(tidyDoc, TidyStrictTagsAttr, yes)
        && tidyOptSetBool(tidyDoc, TidyShowWarnings, yes)
        && tidyOptSetBool(tidyDoc, TidyShowWarnings, no)
        && tidyOptSetInt(tidyDoc, TidyWrapLen, 0)
        //&& tidyOptSetBool(tidyDoc, TidyPreserveEntities, yes)
        //&& tidyOptSetInt(tidyDoc, TidyDoctype, TidyDoctypeHtml5)
        //&& tidyOptSetValue(tidyDoc, TidyInlineTags, "<p>")
        ;
    tidyParseString(tidyDoc, s.c_str());
    tidyCleanAndRepair(tidyDoc);
    tidySaveBuffer(tidyDoc, &tidyOutputBuffer);
    if (!tidyOutputBuffer.bp)
        throw SW_RUNTIME_ERROR("tidy: cannot convert from html to xhtml");
    std::string tidyResult;
    tidyResult = (char*)tidyOutputBuffer.bp;
    tidyBufFree(&tidyOutputBuffer);
    return tidyResult;
}
