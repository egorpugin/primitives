#include <primitives/lock.h>

#define private protected // WOW!!!
#include <boost/interprocess/sync/file_lock.hpp>
#undef private
#include <boost/nowide/fstream.hpp>

#ifdef BOOST_INTERPROCESS_WINDOWS
#include <windows.h>
#define STRING_MODE wstring
#else
#define STRING_MODE string
#endif

class file_lock : public FileLock
{
public:
    // for other than windows systems
    using FileLock::FileLock;

#ifdef BOOST_INTERPROCESS_WINDOWS
    inline file_lock(const wchar_t *name)
    {
        m_file_hnd = open_existing_file(name, boost::interprocess::read_write);

        if (m_file_hnd == boost::interprocess::ipcdetail::invalid_file()) {
            boost::interprocess::error_info err(boost::interprocess::system_error_code());
            throw boost::interprocess::interprocess_exception(err);
        }
    }

private:
    struct interprocess_security_attributes
    {
        unsigned long nLength;
        void *lpSecurityDescriptor;
        int bInheritHandle;
    };

    static const unsigned long error_sharing_violation_tries = 3L;
    static const unsigned long error_sharing_violation_sleep_ms = 250L;

    inline boost::interprocess::file_handle_t open_existing_file(const wchar_t *name, boost::interprocess::mode_t mode = boost::interprocess::read_write, bool temporary = false)
    {
        unsigned long attr = temporary ? boost::interprocess::winapi::file_attribute_temporary : 0;
        return create_file(name, (unsigned int)mode, boost::interprocess::winapi::open_existing, attr, 0);
    }

    inline void *create_file(const wchar_t *name, unsigned long access, unsigned long creation_flags, unsigned long attributes, interprocess_security_attributes *psec)
    {
        for (unsigned int attempt(0); attempt < error_sharing_violation_tries; ++attempt) {
            void * const handle = CreateFileW(name, access,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                (LPSECURITY_ATTRIBUTES)psec, creation_flags, attributes, 0);
            bool const invalid(INVALID_HANDLE_VALUE == handle);
            if (!invalid) {
                return handle;
            }
            if (ERROR_SHARING_VIOLATION != GetLastError()) {
                return handle;
            }
            Sleep(error_sharing_violation_sleep_ms);
        }
        return INVALID_HANDLE_VALUE;
    }
#endif
};

static std::STRING_MODE prepare_lock_file(const path &fn)
{
    fs::create_directories(fn.parent_path());
    auto lock_file = fn.parent_path() / (fn.filename().string() + ".lock");
    if (!fs::exists(lock_file))
        boost::nowide::ofstream(lock_file.string());
    return lock_file.STRING_MODE();
}

////////////////////////////////////////

ScopedFileLock::ScopedFileLock(const path &fn)
{
    lock_ = std::make_unique<file_lock>(prepare_lock_file(fn).c_str());
    lock_->lock();
    locked = true;
}

ScopedFileLock::ScopedFileLock(const path &fn, std::defer_lock_t)
{
    lock_ = std::make_unique<file_lock>(prepare_lock_file(fn).c_str());
}

ScopedFileLock::~ScopedFileLock()
{
    if (locked)
        lock_->unlock();
}

bool ScopedFileLock::try_lock()
{
    return locked = lock_->try_lock();
}

void ScopedFileLock::lock()
{
    lock_->lock();
    locked = true;
}

////////////////////////////////////////

ScopedShareableFileLock::ScopedShareableFileLock(const path &fn)
{
    lock_ = std::make_unique<file_lock>(prepare_lock_file(fn).c_str());
    lock_->lock_sharable();
}

ScopedShareableFileLock::~ScopedShareableFileLock()
{
    lock_->unlock_sharable();
}
