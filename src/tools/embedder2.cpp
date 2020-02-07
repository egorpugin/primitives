// Copyright (C) 2016-2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/filesystem.h>
#include <primitives/sw/main.h>
#include <primitives/sw/settings.h>
#include <primitives/sw/cl.h>

#include <lzma.h>

String compress_file(const String &s, int type)
{
    if (type == 0)
        return s;
    if (type == 1)
    {
        // xz
        lzma_stream strm = LZMA_STREAM_INIT;
        SCOPE_EXIT
        {
            lzma_end(&strm);
        };
        lzma_ret ret = lzma_easy_encoder(&strm, 9 /*| LZMA_PRESET_EXTREME*/, LZMA_CHECK_CRC64);
        if (ret != LZMA_OK)
            throw SW_RUNTIME_ERROR("lzma error");

        String t;
        uint8_t outbuf[BUFSIZ];

        lzma_action action = LZMA_RUN;

        strm.next_in = (const uint8_t *)s.c_str();
        strm.avail_in = s.size();
        strm.next_out = outbuf;
        strm.avail_out = sizeof(outbuf);

        // Loop until the file has been successfully compressed or until
        // an error occurs.
        while (1)
        {
            if (strm.avail_in == 0)
                action = LZMA_FINISH;

            lzma_ret ret = lzma_code(&strm, action);

            if (strm.avail_out == 0 || ret == LZMA_STREAM_END)
            {
                size_t write_size = sizeof(outbuf) - strm.avail_out;
                auto pos = t.size();
                t.resize(pos + write_size);
                memcpy((char *)t.c_str() + pos, outbuf, write_size);

                // Reset next_out and avail_out.
                strm.next_out = outbuf;
                strm.avail_out = sizeof(outbuf);
            }

            if (ret != LZMA_OK)
            {
                if (ret == LZMA_STREAM_END)
                    return t;
            }
        }
        throw SW_RUNTIME_ERROR("lzma error");
    }
    SW_UNIMPLEMENTED;
}

String preprocess_file(const String &s)
{
    String o;
    int i = 0;
    for (auto &c : s)
    {
        String h(2, 0);
        sprintf(&h[0], "%02x", c);
        o += "0x" + h + ",";
        if (++i % 25 == 0)
            o += "\n";
        else
            o += " ";
    }
    o += "0x00,";
    if (++i % 25 == 0)
        o += "\n";
    return o;
}

int main(int argc, char *argv[])
{
    cl::opt<path> InputFilename(cl::Positional, cl::desc("<input file>"), cl::Required);
    cl::opt<path> OutputFilename(cl::Positional, cl::desc("<output file>"), cl::Required);
    cl::opt<int> CompressType(cl::Positional, cl::desc("<compress algorithm>"), cl::Required);

    cl::ParseCommandLineOptions(argc, argv);

    write_file(OutputFilename, preprocess_file(compress_file(read_file(InputFilename), CompressType)));

    return 0;
}
