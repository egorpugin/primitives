// Copyright (C) 2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

class debugging_symbols : boost::noncopyable {
    static void try_init_com(boost::stacktrace::detail::com_holder< ::IDebugSymbols>& idebug,
        const boost::stacktrace::detail::com_global_initer& com) BOOST_NOEXCEPT {
        boost::stacktrace::detail::com_holder< ::IDebugClient> iclient(com);
        if (S_OK != ::DebugCreate(__uuidof(IDebugClient), iclient.to_void_ptr_ptr())) {
            return;
        }

        boost::stacktrace::detail::com_holder< ::IDebugControl> icontrol(com);
        const bool res0 = (S_OK == iclient->QueryInterface(
            __uuidof(IDebugControl),
            icontrol.to_void_ptr_ptr()
        ));
        if (!res0) {
            return;
        }

        const bool res1 = (S_OK == iclient->AttachProcess(
            0,
            ::GetCurrentProcessId(),
            DEBUG_ATTACH_NONINVASIVE | DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND
        ));
        if (!res1) {
            return;
        }

        if (S_OK != icontrol->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE)) {
            return;
        }

        // No cheking: QueryInterface sets the output parameter to NULL in case of error.
        iclient->QueryInterface(__uuidof(IDebugSymbols), idebug.to_void_ptr_ptr());
    }

#ifndef BOOST_STACKTRACE_USE_WINDBG_CACHED

    boost::stacktrace::detail::com_global_initer com_;
    boost::stacktrace::detail::com_holder< ::IDebugSymbols> idebug_;
public:
    debugging_symbols() BOOST_NOEXCEPT
        : com_()
        , idebug_(com_)
    {
        try_init_com(idebug_, com_);

        if (gSymbolPath.empty())
            return;

        // maybe AppendSymbolPath?
        bool res = (S_OK == idebug_->SetSymbolPath(gSymbolPath.c_str()));
        if (!res)
            printf("cannot set symbol path\n");
    }

#else

#ifdef BOOST_NO_CXX11_THREAD_LOCAL
#   error Your compiler does not support C++11 thread_local storage. It`s impossible to build with BOOST_STACKTRACE_USE_WINDBG_CACHED.
#endif

    static com_holder< ::IDebugSymbols>& get_thread_local_debug_inst() BOOST_NOEXCEPT {
        // [class.mfct]: A static local variable or local type in a member function always refers to the same entity, whether
        // or not the member function is inline.
        static thread_local boost::stacktrace::detail::com_global_initer com;
        static thread_local com_holder< ::IDebugSymbols> idebug(com);

        if (!idebug.is_inited()) {
            try_init_com(idebug, com);
        }

        return idebug;
    }

    com_holder< ::IDebugSymbols>& idebug_;
public:
    debugging_symbols() BOOST_NOEXCEPT
        : idebug_( get_thread_local_debug_inst() )
    {}

#endif // #ifndef BOOST_STACKTRACE_USE_WINDBG_CACHED

    bool is_inited() const BOOST_NOEXCEPT {
        return idebug_.is_inited();
    }

    std::string get_name_impl(const void* addr, std::string* module_name = 0) const {
        std::string result;
        if (!is_inited()) {
            return result;
        }
        const ULONG64 offset = reinterpret_cast<ULONG64>(addr);

        char name[256];
        name[0] = '\0';
        ULONG size = 0;
        bool res = (S_OK == idebug_->GetNameByOffset(
            offset,
            name,
            sizeof(name),
            &size,
            0
        ));

        if (!res && size != 0) {
            result.resize(size);
            res = (S_OK == idebug_->GetNameByOffset(
                offset,
                &result[0],
                static_cast<ULONG>(result.size()),
                &size,
                0
            ));
        } else if (res) {
            result = name;
        }

        if (!res) {
            result.clear();
            return result;
        }

        const std::size_t delimiter = result.find_first_of('!');
        if (module_name) {
            *module_name = result.substr(0, delimiter);
        }

        if (delimiter == std::string::npos) {
            // If 'delimiter' is equal to 'std::string::npos' then we have only module name.
            result.clear();
            return result;
        }

        result = boost::stacktrace::detail::mingw_demangling_workaround(
            result.substr(delimiter + 1)
        );

        return result;
    }

    std::size_t get_line_impl(const void* addr) const BOOST_NOEXCEPT {
        ULONG result = 0;
        if (!is_inited()) {
            return result;
        }

        const bool is_ok = (S_OK == idebug_->GetLineByOffset(
            reinterpret_cast<ULONG64>(addr),
            &result,
            0,
            0,
            0,
            0
        ));

        return (is_ok ? result : 0);
    }

    std::pair<std::string, std::size_t> get_source_file_line_impl(const void* addr) const {
        std::pair<std::string, std::size_t> result;
        if (!is_inited()) {
            return result;
        }
        const ULONG64 offset = reinterpret_cast<ULONG64>(addr);

        char name[256];
        name[0] = 0;
        ULONG size = 0;
        ULONG line_num = 0;
        bool res = (S_OK == idebug_->GetLineByOffset(
            offset,
            &line_num,
            name,
            sizeof(name),
            &size,
            0
        ));

        if (res) {
            result.first = name;
            result.second = line_num;
            return result;
        }

        if (!res && size == 0) {
            return result;
        }

        result.first.resize(size);
        res = (S_OK == idebug_->GetLineByOffset(
            offset,
            &line_num,
            &result.first[0],
            static_cast<ULONG>(result.first.size()),
            &size,
            0
        ));
        result.second = line_num;

        if (!res) {
            result.first.clear();
            result.second = 0;
        }

        return result;
    }

    void to_string_impl(const void* addr, std::string& res) const {
        if (!is_inited()) {
            return;
        }

        std::string module_name;
        std::string name = this->get_name_impl(addr, &module_name);
        if (!name.empty()) {
            res += name;
        } else {
            res += boost::stacktrace::detail::to_hex_array(addr).data();
        }

        std::pair<std::string, std::size_t> source_line = this->get_source_file_line_impl(addr);
        if (!source_line.first.empty() && source_line.second) {
            res += " at ";
            res += source_line.first;
            res += ':';
            res += boost::stacktrace::detail::to_dec_array(source_line.second).data();
        } else if (!module_name.empty()) {
            res += " in ";
            res += module_name;
        }
    }
};

static std::string frame_to_string(const boost::stacktrace::frame* frames, std::size_t size) {
    debugging_symbols idebug;
    if (!idebug.is_inited()) {
        return std::string();
    }

    std::string res;
    res.reserve(64 * size);
    for (std::size_t i = 0; i < size; ++i) {
        if (i < 10) {
            res += ' ';
        }
        res += boost::stacktrace::detail::to_dec_array(i).data();
        res += '#';
        res += ' ';
        idebug.to_string_impl(frames[i].address(), res);
        res += '\n';
    }

    return res;
}

template <class Allocator>
static std::string frame_to_string(const boost::stacktrace::basic_stacktrace<Allocator>& bt)
{
    if (!bt)
        return {};
    return frame_to_string(&bt.as_vector()[0], bt.size());
}
