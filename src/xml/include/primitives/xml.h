// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/string.h>
#include <primitives/exceptions.h>

#include <boost/algorithm/string.hpp>
#include <libxml/tree.h>

#include <iomanip>
#include <sstream>

void checkNullptr(const void *from)
{
    if (!from)
        throw SW_RUNTIME_ERROR("nullptr");
}

PRIMITIVES_XML_API
String getName(xmlNode *from)
{
    checkNullptr(from);
    checkNullptr(from->name);
    return (char *)from->name;
}

PRIMITIVES_XML_API
String getName(xmlAttr *from)
{
    checkNullptr(from);
    checkNullptr(from->name);
    return (char *)from->name;
}

PRIMITIVES_XML_API
String getContent(xmlNode *from)
{
    checkNullptr(from);
    if (from->type == XML_ELEMENT_NODE)
    {
        from = &from->children[0];
        if (!from)
            return {};
        return getContent(from);
    }
    if (from->type == XML_TEXT_NODE)
        return (char*)from->content;
    throw SW_RUNTIME_ERROR("unknown node type");
}

static xmlNode *getNextRaw(xmlNode *from, const String &name)
{
    if (!from)
        return nullptr;

    for (auto cur_node = from; cur_node; cur_node = cur_node->next)
    {
        if (cur_node->type == XML_ELEMENT_NODE)
        {
            if (getName(cur_node) == name)
                return cur_node;
        }
        auto n = getNextRaw(cur_node->children, name);
        if (n)
            return n;
    }
    return nullptr;
}

PRIMITIVES_XML_API
xmlNode *getNext(xmlNode *from, const String &name, int skip = 0)
{
    while (skip--)
        from = getNext(from, name);
    checkNullptr(from);
    if (getName(from) == name)
    {
        if (from->children)
        {
            auto n = getNextRaw(from->children, name);
            if (n)
                return n;
        }
        if (from->next)
        {
            auto n = getNextRaw(from->next, name);
            if (n)
                return n;
        }
        return nullptr;
    }
    auto n = getNextRaw(from, name);
    if (n)
        return n;
    throw SW_RUNTIME_ERROR(name + ": not found");
}

PRIMITIVES_XML_API
xmlNode *findNodeWithText(xmlNode *from, const String &text)
{
    if (!from)
        return nullptr;

    for (auto cur_node = from; cur_node; cur_node = cur_node->next)
    {
        //if (cur_node->type == XML_ELEMENT_NODE)
        {
            if (cur_node->content && String((char*)cur_node->content).find(text) != String::npos)
                return cur_node;
        }
        auto n = findNodeWithText(cur_node->children, text);
        if (n)
            return n;
    }
    return nullptr;
}

PRIMITIVES_XML_API
void removeNode(xmlNode *n)
{
    xmlUnlinkNode(n);
    xmlFreeNode(n);
}

PRIMITIVES_XML_API
void removeTags(xmlNode *from, const String &tag)
{
    while (auto n = getNextRaw(from, tag))
        removeNode(n);
}

PRIMITIVES_XML_API
void removeComments(xmlNode *from)
{
    if (!from)
        return;

    if (from->content)
    {
        String s = (const char *)from->content;
        boost::trim(s);
        memcpy(from->content, s.data(), s.size() + 1);
    }

    for (auto cur_node = from; cur_node;)
    {
        removeComments(cur_node->children);

        auto n = cur_node;
        cur_node = cur_node->next;

        if (n->type == XML_COMMENT_NODE)
            removeNode(n);
    }
}

/*
String convertTimestamp(const String &s)
{
    std::tm t = {};
    std::istringstream ss(s);
    ss >> std::get_time(&t, "%d.%m.%Y %H:%M:%S");
    if (ss.fail())
        throw SW_RUNTIME_ERROR("Date parse failed");
    char mbstr[100];
    if (std::strftime(mbstr, sizeof(mbstr), "%F %T", &t)) {
        return mbstr;
    }
    throw SW_RUNTIME_ERROR("strftime error");
}
*/
