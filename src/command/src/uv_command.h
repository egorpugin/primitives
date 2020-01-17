// Copyright (C) 2020 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/command.h>

#include <uv.h>

// all strings in data structures are utf-8

namespace primitives::command::detail
{

struct UvCommand;

struct io_data
{
    enum
    {
        // with priority

        T_EMPTY,

        //
        T_IGNORE,

        // default action is to capture
        T_CAPTURE,

        // redirect to real file
        T_REDIRECT_TO_FILE,

        // sometimes we want to inherit things, highest priority
        // equals to redirect to fd
        T_INHERIT,

        T_REDIRECT_BETWEEN_PROCESSES,
    };

    UvCommand &c;
    int type = T_EMPTY;
    uv_fs_t file_req = { 0 }; // redirect
    uv_pipe_t p = { 0 }; // capture
    uv_pipe_t *p_provided = nullptr; // capture other's stream
    int fd; // inherit
    bool out = true; // direction, rename to read?

    // write (stdin) operations
    uv_buf_t wbuf = { 0 };
    uv_write_t wreq = { 0 };

public:
    io_data(UvCommand &c);

    void set_ignore(int fd);
    uv_pipe_t *get_redirect_pipe(bool out = true);
    void set_redirect_pipe(uv_pipe_t *p_provided, bool out = true);
    void set_capture(int fd, primitives::Command::Stream &dst, bool out = true);
    void set_redirect(const path &file, bool out = true, bool append = false);
    // add set redirect for fd r/w

    void set_inherit(int fd);
    void set_type(int t);
    void clear(int t);
    void clean() noexcept;

    uv_stdio_container_t get_child_stdio() const;
    void start_capture(Strings &errors);

    static void on_alloc(uv_handle_t *s, size_t suggested_size, uv_buf_t *buf);
    static void on_read(uv_stream_t *s, ssize_t nread, const uv_buf_t *rdbuf);
    static void on_write(uv_write_t *req, int status);
};

struct io_datas : std::vector<std::unique_ptr<io_data>>
{
    std::vector<uv_stdio_container_t> get_child_stdio() const;
};

struct UvCommand
{
    uv_loop_t *loop;
    primitives::Command &c;

    // data holders, all strings are utf-8
    String prog;
    String wdir;

    Strings args;
    std::vector<char*> uv_args;

    // i/o
    io_datas streams;
    std::vector<uv_stdio_container_t> child_stdio;

    // options
    uv_process_options_t options = { 0 };

    // env
    std::vector<char *> env;
    std::vector<String> env_data;

    // child handle
    uv_process_t child_req = { 0 };

public:
    UvCommand(uv_loop_t *loop, primitives::Command &c);

    // actually might throw in case of oom during errors.push_back, but let's ignore it for now
    void execute(Strings &errors) noexcept;

    void clean() noexcept;

    static void on_exit(uv_process_t *child_req, int64_t exit_status, int term_signal);
};

} // namespace primitives::command::detail
