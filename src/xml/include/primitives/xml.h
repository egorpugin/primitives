// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/string.h>

#include <libxml/tree.h>

PRIMITIVES_XML_API
void checkNullptr(const void *from);

PRIMITIVES_XML_API
String getName(xmlNode *from);

PRIMITIVES_XML_API
String getName(xmlAttr *from);

PRIMITIVES_XML_API
String getContent(xmlNode *from);

PRIMITIVES_XML_API
xmlNode *getNext(xmlNode *from, const String &name, int skip = 0);

PRIMITIVES_XML_API
xmlNode *findNodeWithText(xmlNode *from, const String &text);

PRIMITIVES_XML_API
void removeNode(xmlNode *n);

PRIMITIVES_XML_API
void removeTags(xmlNode *from, const String &tag);

PRIMITIVES_XML_API
void removeComments(xmlNode *from);
