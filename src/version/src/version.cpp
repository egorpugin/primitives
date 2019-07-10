#include <primitives/version.h>

#include <range_parser.h>

#include <fmt/format.h>
#include <primitives/constants.h>
#include <primitives/exceptions.h>
#include <primitives/hash_combine.h>
#include <primitives/string.h>

#include <pystring.h>

#include <algorithm>
#include <map>

static void throw_bad_version(const std::string &s)
{
    throw SW_RUNTIME_ERROR("Invalid version: " + s);
}

static void throw_bad_version_extra(const std::string &s)
{
    throw SW_RUNTIME_ERROR("Invalid version extra: " + s);
}

namespace primitives::version
{

namespace detail
{

std::string preprocess_input(const std::string &in)
{
    return pystring::strip(in);
}

/*static std::string preprocess_input_extra(const std::string &in)
{
    return pystring::lower(preprocess_input(in));
}*/

bool Extra::operator<(const Extra &rhs) const
{
    // this one is tricky
    // 1.2.3.rc1 is less than 1.2.3!
    if (empty() && rhs.empty())
        return false;
    if (rhs.empty())
        return true;
    if (empty())
        return false;
    return ::std::operator<(*this, rhs);
}

}

GenericNumericVersion::GenericNumericVersion()
{
    numbers.resize(minimum_level - 1);
    numbers.push_back(1);
}

GenericNumericVersion::GenericNumericVersion(const std::initializer_list<Number> &list, Level level)
    : numbers(list)
{
    if (std::any_of(numbers.begin(), numbers.end(), [](const auto &n) {return n < 0; }))
        throw SW_RUNTIME_ERROR("Version number cannot be less than 0");
    auto l = checkAndNormalizeLevel(level);
    if (numbers.size() < l)
        numbers.resize(l);
    setFirstVersion();
}

void GenericNumericVersion::fill(Number n)
{
    std::fill(numbers.begin(), numbers.end(), n);
}

GenericNumericVersion::Number GenericNumericVersion::get(Level level) const
{
    if (numbers.size() <= level)
        return 0;
        //throw SW_RUNTIME_ERROR("There is no that much numbers");
    return numbers[level];
}

std::string GenericNumericVersion::printVersion() const
{
    return printVersion(".");
}

std::string GenericNumericVersion::printVersion(Level level) const
{
    return printVersion(".", level);
}

std::string GenericNumericVersion::printVersion(const std::string &delimeter) const
{
    return printVersion(".", (Level)numbers.size());
}

std::string GenericNumericVersion::printVersion(const String &delimeter, Level level) const
{
    std::string s;
    Level i = 0;
    auto until = std::min(level, (Level)numbers.size());
    for (; i < until; i++)
        s += std::to_string(numbers[i]) + delimeter;
    for (; i < level; i++)
        s += std::to_string(0) + delimeter;
    if (!s.empty())
        s.resize(s.size() - delimeter.size());
    return s;
}

GenericNumericVersion::Level GenericNumericVersion::getLevel() const
{
    return (Level)numbers.size();
}

void GenericNumericVersion::setLevel(Level level, Number fill)
{
    numbers.resize(level, fill);
}

void GenericNumericVersion::setFirstVersion()
{
    if (std::all_of(numbers.begin(), numbers.end(), [](const auto &n) {return n == 0; }))
        numbers.back() = 1;
}

GenericNumericVersion::Level GenericNumericVersion::checkAndNormalizeLevel(Level in)
{
    if (in <= 0)
        throw SW_RUNTIME_ERROR("Bad version level (<= 0): " + std::to_string(in));
    return std::max(in, minimum_level);
}

Version::Version()
{
}

Version::Version(const Version &v, const Extra &e)
    : Version(v)
{
    extra = e;
}

Version::Version(const Extra &e)
    : Version()
{
    extra = e;
    checkExtra();
}

Version::Version(const std::string &s)
{
    numbers.clear();
    numbers.resize(minimum_level);

    if (!parse(*this, detail::preprocess_input(s)))
        throw_bad_version(s);
    if (isBranch() && branch.size() > 200)
        throw_bad_version(s + ", branch must have size <= 200");
}

Version::Version(const std::string &v, const std::string &e)
    : Version(v + "-" + e)
{
}

Version::Version(const Version &version, const std::string &extra)
    : Version(version)
{
    parseExtra(*this, extra);
}

Version::Version(const char *s)
    : Version(std::string(s))
{
}

Version::Version(const std::initializer_list<Number> &l)
    : GenericNumericVersion(l)
{
}

Version::Version(int ma)
    : GenericNumericVersion({ ma })
{
}

Version::Version(Number ma)
    : GenericNumericVersion({ ma })
{
}

Version::Version(Number ma, Number mi)
    : GenericNumericVersion({ ma, mi })
{
}

Version::Version(Number ma, Number mi, Number pa)
    : GenericNumericVersion({ ma, mi, pa })
{
}

Version::Version(Number ma, Number mi, Number pa, Number tw)
    : GenericNumericVersion({ ma, mi, pa, tw })
{
}

Version::Version(Number ma, const Extra &e)
    : Version(ma)
{
    extra = e;
    checkExtra();
}

Version::Version(Number ma, Number mi, const Extra &e)
    : Version(ma, mi)
{
    extra = e;
    checkExtra();
}

Version::Version(Number ma, Number mi, Number pa, const Extra &e)
    : Version(ma, mi, pa)
{
    extra = e;
    checkExtra();
}

Version::Version(Number ma, Number mi, Number pa, Number tw, const Extra &e)
    : Version(ma, mi, pa, tw)
{
    extra = e;
    checkExtra();
}

Version::Version(Number ma, const std::string &e)
    : Version(ma)
{
    if (!parseExtra(*this, detail::preprocess_input(e)))
        throw_bad_version_extra(e);
}

Version::Version(Number ma, Number mi, const std::string &e)
    : Version(ma, mi)
{
    if (!parseExtra(*this, detail::preprocess_input(e)))
        throw_bad_version_extra(e);
}

Version::Version(Number ma, Number mi, Number pa, const std::string &e)
    : Version(ma, mi, pa)
{
    if (!parseExtra(*this, detail::preprocess_input(e)))
        throw_bad_version_extra(e);
}

Version::Version(Number ma, Number mi, Number pa, Number tw, const std::string &e)
    : Version(ma, mi, pa, tw)
{
    if (!parseExtra(*this, detail::preprocess_input(e)))
        throw_bad_version_extra(e);
}

Version::Number Version::getMajor() const
{
    return get(0);
}

Version::Number Version::getMinor() const
{
    return get(1);
}

Version::Number Version::getPatch() const
{
    return get(2);
}

Version::Number Version::getTweak() const
{
    return get(3);
}

Version::Extra &Version::getExtra()
{
    return extra;
}

const Version::Extra &Version::getExtra() const
{
    return extra;
}

std::string Version::getBranch() const
{
    return branch;
}

std::string Version::toString() const
{
    return toString(std::string("."));
}

std::string Version::toString(Level level) const
{
    return toString(".", level);
}

std::string Version::toString(const std::string &delimeter) const
{
    return toString(delimeter, (Level)numbers.size());
}

std::string Version::toString(const String &delimeter, Level level) const
{
    if (!branch.empty())
        return branch;

    auto s = printVersion(delimeter, level);
    if (!extra.empty())
        s += "-" + printExtra();
    return s;
}

std::string Version::toString(const Version &v) const
{
    return toString((Level)v.numbers.size());
}

std::string Version::toString(const String &delimeter, const Version &v) const
{
    return toString(delimeter, (Level)v.numbers.size());
}

size_t Version::getHash() const
{
    if (isBranch())
        return std::hash<std::string>()(getBranch());
    size_t h = 0;
    for (auto &n : numbers)
        hash_combine(h, n);
    return h;
}

std::string Version::printExtra() const
{
    std::string s;
    for (auto &e : extra)
    {
        switch (e.index())
        {
        case 0:
            s += std::get<std::string>(e) + ".";
            break;
        case 1:
            s += std::to_string(std::get<Number>(e)) + ".";
            break;
        }
    }
    if (!s.empty())
        s.resize(s.size() - 1);
    return s;
}

void Version::checkExtra() const
{
    Version v;
    auto c = std::any_of(extra.begin(), extra.end(),
        [&v](const auto &e)
    {
        if (std::holds_alternative<Number>(e))
            return true;
        return !parseExtra(v, std::get<std::string>(e));
    });
    if (c || v.extra != extra)
        throw_bad_version(toString());
}

bool Version::isBranch() const
{
    return !isVersion();
}

bool Version::isVersion() const
{
    return branch.empty();
}

bool Version::hasExtra() const
{
    return !getExtra().empty();
}

bool Version::isRelease() const
{
    return !isPreRelease(); // && !branch()?
}

bool Version::isPreRelease() const
{
    return hasExtra();
}

Version::Level Version::getMatchingLevel(const Version &v) const
{
    int i = 0;
    if (isBranch() || v.isBranch())
        return i;

    auto m = std::min(getLevel(), v.getLevel());
    for (; i < m; i++)
    {
        if (numbers[i] != v.numbers[i])
            return i;
    }
    return i;
}

template <class F>
bool Version::cmp(const Version &v1, const Version &v2, F f)
{
    if (v1.numbers.size() != v2.numbers.size())
    {
        auto v3 = v1;
        auto v4 = v2;
        auto sz = std::max(v1.numbers.size(), v2.numbers.size());
        v3.numbers.resize(sz);
        v4.numbers.resize(sz);
        return cmp(v3, v4, f);
    }
    return f(std::tie(v1.numbers, v1.extra), std::tie(v2.numbers, v2.extra));
}

bool Version::operator<(const Version &rhs) const
{
    if (isBranch() && rhs.isBranch())
        return branch < rhs.branch;
    if (isBranch() || rhs.isBranch())
        return !(branch < rhs.branch);
    return cmp(*this, rhs, std::less<decltype(std::tie(numbers, extra))>());
}

bool Version::operator>(const Version &rhs) const
{
    if (isBranch() && rhs.isBranch())
        return branch > rhs.branch;
    if (isBranch() || rhs.isBranch())
        return !(branch > rhs.branch);
    return cmp(*this, rhs, std::greater<decltype(std::tie(numbers, extra))>());
}

bool Version::operator<=(const Version &rhs) const
{
    if (isBranch() && rhs.isBranch())
        return branch <= rhs.branch;
    if (isBranch() || rhs.isBranch())
        return !(branch <= rhs.branch);
    return cmp(*this, rhs, std::less_equal<decltype(std::tie(numbers, extra))>());
}

bool Version::operator>=(const Version &rhs) const
{
    if (isBranch() && rhs.isBranch())
        return branch >= rhs.branch;
    if (isBranch() || rhs.isBranch())
        return !(branch >= rhs.branch);
    return cmp(*this, rhs, std::greater_equal<decltype(std::tie(numbers, extra))>());
}

bool Version::operator==(const Version &rhs) const
{
    if (isBranch() || rhs.isBranch())
        return branch == rhs.branch;
    return cmp(*this, rhs, std::equal_to<decltype(std::tie(numbers, extra))>());
}

bool Version::operator!=(const Version &rhs) const
{
    return !operator==(rhs);
}

Version &Version::operator++()
{
    incrementVersion();
    return *this;
}

Version &Version::operator--()
{
    decrementVersion();
    return *this;
}

Version Version::operator++(int)
{
    auto v = *this;
    incrementVersion();
    return v;
}

Version Version::operator--(int)
{
    auto v = *this;
    decrementVersion();
    return v;
}

Version Version::min(Level level)
{
    level = checkAndNormalizeLevel(level);

    Version v;
    v.numbers.clear();
    v.numbers.resize(level - 1);
    v.numbers.push_back(1);
    return v;
}

Version Version::min(const Version &v)
{
    return min(v.getLevel());
}

Version Version::max(Level level)
{
    level = checkAndNormalizeLevel(level);

    Version v;
    v.numbers.clear();
    v.numbers.resize(level);
    v.fill(maxNumber());
    return v;
}

Version Version::max(const Version &v)
{
    return max(v.getLevel());
}

Version::Number Version::maxNumber()
{
    // currently set a limit up to 1 billion excluding
    return 999'999'999;
}

void Version::decrementVersion()
{
    decrementVersion(getLevel());
}

void Version::incrementVersion()
{
    incrementVersion(getLevel());
}

Version Version::getNextVersion() const
{
    return getNextVersion(getLevel());
}

Version Version::getPreviousVersion() const
{
    return getPreviousVersion(getLevel());
}

void Version::decrementVersion(Level l)
{
    if (*this == min(*this))
        return;

    if (l > getLevel())
        setLevel(l);
    auto current = &numbers[l - 1];
    while (*current == 0)
        *current-- = maxNumber();
    (*current)--;
}

void Version::incrementVersion(Level l)
{
    if (*this == max(*this))
        return;

    if (l > getLevel())
        setLevel(l);
    auto current = &numbers[l - 1];
    while (*current == maxNumber())
        *current-- = 0;
    (*current)++;
}

Version Version::getNextVersion(Level l) const
{
    auto v = *this;
    v.incrementVersion(l);
    return v;
}

Version Version::getPreviousVersion(Level l) const
{
    auto v = *this;
    v.decrementVersion(l);
    return v;
}

std::string Version::format(const std::string &s) const
{
    auto f = s;
    format(f);
    return f;
}

void Version::format(std::string &s) const
{
    // TODO: optimize

    auto create_latin_replacements = [](Number n, char base = 'a')
    {
        Number a = 26;
        std::string s;
        auto p = a;
        size_t c = 1;
        while (n >= p)
        {
            n -= p;
            p *= a;
            c++;
        }
        while (n >= a)
        {
            auto d = div(n, a);
            s += (char)(d.rem + base);
            n = d.quot;
        }
        s += (char)(n + base);
        std::reverse(s.begin(), s.end());
        if (s.size() < c)
            s = std::string(c - s.size(), base) + s;
        return s;
    };

    auto create_latin_replacements_capital = [&create_latin_replacements](Number n)
    {
        return create_latin_replacements(n, 'A');
    };

    auto get_optional_number = [this](Level level) -> std::optional<Number>
    {
        if (level > numbers.size())
            return {};

        level--;

        bool has_number = false;
        auto sz = numbers.size();
        for (auto i = sz - 1; i >= level; i--)
        {
            if (numbers[i] == 0)
                continue;
            has_number = true;
            break;
        }
        if (has_number)
        {
            return numbers[level];
        }
        return {};
    };

    auto get_optional = [this](const auto &n) -> String
    {
        if (n)
            return "." + std::to_string(n.value());
        return {};
    };

#define VAR(v, l, f) auto v ## l = f(v)
#define VAR_SMALL(v) VAR(v, l, create_latin_replacements)
#define VAR_CAP(v) VAR(v, L, create_latin_replacements_capital)

#define VAR_OPT(v, l, f) auto v ## l = v ## on ? ("." + f(v)) : "";
#define VAR_SMALL_OPT(v) VAR_OPT(v, l ## o, create_latin_replacements)
#define VAR_CAP_OPT(v) VAR_OPT(v, L ## o, create_latin_replacements_capital)

#define ARG(x) fmt::arg(#x, x)
#define ARG_OPT(x) ARG(x ## o)

    auto M = getMajor();
    auto m = getMinor();
    auto p = getPatch();
    auto t = getTweak();

    auto mon = get_optional_number(2);
    auto pon = get_optional_number(3);
    auto ton = get_optional_number(4);

    auto mo = get_optional(mon);
    auto po = get_optional(pon);
    auto to = get_optional(ton);

    VAR_CAP(M);
    VAR_CAP(m);
    VAR_CAP(p);
    VAR_CAP(t);

    VAR_SMALL(M);
    VAR_SMALL(m);
    VAR_SMALL(p);
    VAR_SMALL(t);

    // optional
    VAR_CAP_OPT(m);
    VAR_CAP_OPT(p);
    VAR_CAP_OPT(t);

    VAR_SMALL_OPT(m);
    VAR_SMALL_OPT(p);
    VAR_SMALL_OPT(t);

    s = fmt::format(s,
        ARG(M),
        ARG(m),
        ARG(p),
        ARG(t),

        // letter variants
        // captical
        ARG(ML),
        ARG(mL),
        ARG(pL),
        ARG(tL),

        // small
        ARG(Ml),
        ARG(ml),
        ARG(pl),
        ARG(tl),

        // optionals
        // no major!
        ARG(mo),
        ARG(po),
        ARG(to),

        // letter variants
        // captical
        ARG(mLo),
        ARG(pLo),
        ARG(tLo),

        // small
        ARG(mlo),
        ARG(plo),
        ARG(tlo),

        //
        fmt::arg("e", printExtra()),
        fmt::arg("b", branch),
        fmt::arg("v", toString()),
        fmt::arg("Mm", toString(2)) // equals to {M}.{m}
        //,

        // extended
        /*fmt::arg("5", get(4)),
        fmt::arg("5L", create_latin_replacements_capital(get(4))),
        fmt::arg("5l", create_latin_replacements(get(4))),
        fmt::arg("5o", "." + std::to_string(get(4))),
        fmt::arg("5Lo", "." + create_latin_replacements_capital(get(4))),
        fmt::arg("5lo", "." + create_latin_replacements(get(4)))*/
    );

#undef ARG
#undef ARG_OPT

#undef VAR
#undef VAR_SMALL
#undef VAR_CAP

#undef VAR_OPT
#undef VAR_SMALL_OPT
#undef VAR_CAP_OPT
}

}
