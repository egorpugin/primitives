#include <primitives/pack.h>

#include <libarchive/archive.h>
#include <libarchive/archive_entry.h>

#if !defined(_WIN32) && !defined(__APPLE__)
#include <linux/limits.h>
#endif

#define BLOCK_SIZE 8192

bool pack_files(const path &fn, const Files &files, const path &root_dir)
{
    bool result = true;
    auto a = archive_write_new();
    archive_write_add_filter_gzip(a);
    archive_write_set_format_pax_restricted(a);
    archive_write_open_filename(a, fn.string().c_str());
    for (auto &f : files)
    {
        if (!fs::exists(f))
        {
            result = false;
            continue;
        }

        // skip symlinks too
        if (!fs::is_regular_file(f))
            continue;

        auto sz = fs::file_size(f);
        auto e = archive_entry_new();
        archive_entry_set_pathname(e, fs::relative(f, root_dir).string().c_str());
        archive_entry_set_size(e, sz);
        archive_entry_set_filetype(e, AE_IFREG);
        archive_entry_set_perm(e, 0644);
        archive_write_header(a, e);
        auto fp = fopen(f.string().c_str(), "rb");
        if (!fp)
        {
            archive_entry_free(e);
            result = false;
            continue;
        }
        char buff[BLOCK_SIZE];
        while (auto len = fread(buff, 1, sizeof(buff), fp))
            archive_write_data(a, buff, len);
        fclose(fp);
        archive_entry_free(e);
    }
    archive_write_close(a);
    archive_write_free(a);
    return result;
}

Files unpack_file(const path &fn, const path &dst)
{
    if (!fs::exists(dst))
        fs::create_directories(dst);

    Files files;

    auto a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    auto r = archive_read_open_filename(a, fn.string().c_str(), BLOCK_SIZE);
    if (r != ARCHIVE_OK)
        throw std::runtime_error(archive_error_string(a));
    archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        // act on regular files only
        auto type = archive_entry_filetype(entry);
        if (type != AE_IFREG)
            continue;

        path f = dst / archive_entry_pathname(entry);
        path fdir = f.parent_path();
        if (!fs::exists(fdir))
            fs::create_directories(fdir);
        path filename = f.filename();
        if (filename == "." || filename == "..")
            continue;

        auto fn = fs::absolute(f).string();
        // do not use fopen() because MSVC's file streams support UTF-16 characters
        // and we won't be able to unpack files under some tricky dir names
        std::ofstream o(fn, std::ios::out | std::ios::binary);
        if (!o)
        {
            // TODO: probably remove this and linux/limit.h header when server will be using hash paths
#ifdef _WIN32
            if (fn.size() >= MAX_PATH)
                continue;
#elif defined(__APPLE__)
#else
            if (fn.size() >= PATH_MAX)
                continue;
#endif
            throw std::runtime_error("Cannot open file: " + f.string());
        }
        while (1)
        {
            const void *buff;
            size_t size;
            int64_t offset;
            auto r = archive_read_data_block(a, &buff, &size, &offset);
            if (r == ARCHIVE_EOF)
                break;
            if (r < ARCHIVE_OK)
                throw std::runtime_error(archive_error_string(a));
            o.write((const char *)buff, size);
        }
        files.insert(f);
    }
    archive_read_close(a);
    archive_read_free(a);

    return files;
}
