#pragma once

#include <primitives/filesystem.h>

struct Hasher
{
    String hash;

#define ADD_OPERATOR(t)  \
    Hasher operator|(t); \
    Hasher &operator|=(t)

    ADD_OPERATOR(bool);
    ADD_OPERATOR(const String &);
    ADD_OPERATOR(const path &);

#undef ADD_OPERATOR

private:
    void do_hash();
};
