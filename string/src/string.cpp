#include <primitives/string.h>

#include <boost/algorithm/string.hpp>

#include <codecvt>
#include <locale>

Strings split_string(const String &s, const String &delims)
{
    std::vector<String> v, lines;
    boost::split(v, s, boost::is_any_of(delims));
    for (auto &l : v)
    {
        boost::trim(l);
        if (!l.empty())
            lines.push_back(l);
    }
    return lines;
}

Strings split_lines(const String &s)
{
    return split_string(s, "\r\n");
}

#ifdef _WIN32
void normalize_string(String &s)
{
    std::replace(s.begin(), s.end(), '\\', '/');
}

String normalize_string_copy(String s)
{
    normalize_string(s);
    return s;
}
#endif

String trim_double_quotes(String s)
{
    boost::trim(s);
    while (!s.empty())
    {
        if (s.front() == '"')
        {
            s = s.substr(1);
            continue;
        }
        if (s.back() == '"')
        {
            s.resize(s.size() - 1);
            continue;
        }
        break;
    }
    boost::trim(s);
    return s;
}

static auto& get_string_converter()
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter;
}

std::wstring to_wstring(const std::string &s)
{
    auto &converter = get_string_converter();
    return converter.from_bytes(s.c_str());
}

std::string to_string(const std::wstring &s)
{
    auto &converter = get_string_converter();
    return converter.to_bytes(s.c_str());
}
