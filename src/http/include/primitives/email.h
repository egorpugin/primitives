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
    String server;
    String user;
    String password;

    String sendEmail(const Email &);
    void setAuth(const String &);
};

}
