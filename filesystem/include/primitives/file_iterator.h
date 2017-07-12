#pragma once

#include <primitives/filesystem.h>

#include <boost/nowide/fstream.hpp>

#include <vector>

struct FileIterator
{
    using Buffer = std::vector<uint8_t>;
    using BuffersRef = std::vector<std::reference_wrapper<Buffer>>;

    struct File
    {
        path fn;
        std::unique_ptr<boost::nowide::ifstream> ifile;
        uint64_t size;
        FileIterator::Buffer buf;
        uint64_t read = 0;
    };

    std::vector<File> files;
    int buffer_size = 8192;

    FileIterator(const std::vector<path> &fns);

    bool iterate(std::function<bool(const BuffersRef &, uint64_t)> f);
    bool is_same_size() const;
    bool is_same_read_size() const;
};
