#include <primitives/string.h>

namespace primitives
{

struct Person
{
    String name;
    String email;
};

struct Email
{
    Person to;
    std::vector<Person> cc;
    Person from;
    String title;
    String body;

    bool custom_email = false;
};

struct Smtp
{
    enum
    {
        SSL_NONE,
        SSL_TRY,
        SSL_CONTROL,
        SSL_ALL,
    };

    String server;
    String user;
    String password;
    int ssl = SSL_ALL;
    String auth = "AUTH=LOGIN";
    int verbose = 0;

    String sendEmail(const Email &) const;
    void setAuth(const String &);
};

}
