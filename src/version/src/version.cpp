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

static std::string preprocess_input(const std::string &in)
{
    return pystring::strip(in);
}

static std::string preprocess_input_extra(const std::string &in)
{
    return pystring::lower(preprocess_input(in));
}

static void throw_bad_version(const std::string &s)
{
    throw SW_RUNTIME_ERROR("Invalid version: " + s);
}

static void throw_bad_version_extra(const std::string &s)
{
    throw SW_RUNTIME_ERROR("Invalid version extra: " + s);
}

static void throw_bad_version_range(const std::string &s)
{
    throw SW_RUNTIME_ERROR("Invalid version range: " + s);
}

namespace primitives::version
{

namespace detail
{

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

    if (!parse(*this, preprocess_input(s)))
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
    if (!parseExtra(*this, preprocess_input(e)))
        throw_bad_version_extra(e);
}

Version::Version(Number ma, Number mi, const std::string &e)
    : Version(ma, mi)
{
    if (!parseExtra(*this, preprocess_input(e)))
        throw_bad_version_extra(e);
}

Version::Version(Number ma, Number mi, Number pa, const std::string &e)
    : Version(ma, mi, pa)
{
    if (!parseExtra(*this, preprocess_input(e)))
        throw_bad_version_extra(e);
}

Version::Version(Number ma, Number mi, Number pa, Number tw, const std::string &e)
    : Version(ma, mi, pa, tw)
{
    if (!parseExtra(*this, preprocess_input(e)))
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

size_t Version::getStdHash() const
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

VersionRange::VersionRange()
    : VersionRange(Version::minimum_level)
{
}

VersionRange::VersionRange(Version::Level level)
    : VersionRange(Version::min(level), Version::max(level))
{
}

VersionRange::VersionRange(const Version &v)
    : VersionRange(v, v)
{
}

VersionRange::VersionRange(const Version &v1, const Version &v2)
{
    if (v1.isBranch() && v2.isBranch())
    {
        if (v1 != v2)
            throw SW_RUNTIME_ERROR("Cannot initialize version range from two different versions");
        branch = v1.toString();
        return;
    }
    if (v1.isBranch() || v2.isBranch())
    {
        throw SW_RUNTIME_ERROR("Cannot initialize version range from branch and non-branch versions");
    }
    if (v1 > v2)
        range.emplace_back(v2, v1);
    else
        range.emplace_back(v1, v2);
}

VersionRange::VersionRange(const std::string &s)
{
    auto in = preprocess_input(s);
    if (in.empty())
        range.emplace_back(Version::min(), Version::max());
    else if (auto r = parse(*this, in); r)
        throw_bad_version_range(in + ", error: " + r.value());
}

VersionRange::VersionRange(const char *v)
    : VersionRange(std::string(v))
{
}

std::optional<std::string> VersionRange::parse(VersionRange &vr, const std::string &s)
{
    if (s.size() > 32_KB)
        return { "Range string length must be less than 32K" };

    RangeParser d;
    auto r = d.parse(s);
    auto error = d.bb.error_msg;
    if (r == 0)
    {
        if (auto res = d.bb.getResult<VersionRange>(); res)
        {
            vr = res.value();
            return {};
        }
        else
            error = "parser error: empty result";
    }
    return error;
}

std::optional<VersionRange> VersionRange::parse(const std::string &s)
{
    VersionRange vr;
    auto in = preprocess_input(s);
    if (in.empty())
        vr.range.emplace_back(Version::min(), Version::max());
    else if (auto r = parse(vr, in); r)
        return {};
    return vr;
}

size_t VersionRange::size() const
{
    return range.size();
}

bool VersionRange::isEmpty() const
{
    return range.empty() && !isBranch();
}

bool VersionRange::isOutside(const Version &v) const
{
    return *this < v || *this > v;
}

VersionRange VersionRange::empty()
{
    VersionRange vr;
    vr.range.clear();
    return vr;
}

bool VersionRange::hasVersion(const Version &v) const
{
    if (isBranch() && v.isBranch())
        return branch == v.getBranch();
    if (isBranch() || v.isBranch())
        return false;
    return std::any_of(range.begin(), range.end(),
        [&v](const auto &r) { return r.hasVersion(v); });
}

bool VersionRange::RangePair::hasVersion(const Version &v) const
{
    //if (v.getLevel() > std::max(first.getLevel(), second.getLevel()))
        //return false;
    return first <= v && v <= second;
}

std::optional<VersionRange::RangePair> VersionRange::RangePair::operator&(const RangePair &rhs) const
{
    if (isBranch())
        return *this;
    if (rhs.isBranch())
        return rhs;
    VersionRange::RangePair rp;
    rp.first = std::max(first, rhs.first);
    rp.second = std::min(second, rhs.second);
    if (rp.first > rp.second)
        return {};
    return rp;
}

std::string VersionRange::RangePair::toString() const
{
    if (first.isBranch())
        return first.toString();
    if (second.isBranch())
        return second.toString();

    auto level = std::max(first.getLevel(), second.getLevel());
    if (first == second)
        return first.toString(level);
    std::string s;
    if (first != Version::min(first))
    {
        s += ">=";
        s += first.toString(level);
    }
    if (std::any_of(second.numbers.begin(), second.numbers.end(),
        [](const auto &n) {return n == Version::maxNumber(); }))
    {
        if (second == Version::max(second))
        {
            if (s.empty())
            {
                /*if (second.getLevel() > Version::minimum_level)
                {
                    String s;
                    for (int i = 0; i < second.getLevel(); i++)
                        s += "*.";
                    if (!s.empty())
                        s.resize(s.size() - 1);
                    return s;
                }*/
                return "*";
            }
            return s;
        }
        if (second.getPatch() == Version::maxNumber() ||
            second.getTweak() == Version::maxNumber())
        {
            if (!s.empty())
                s += " ";
            return s + "<" + second.getNextVersion(level).toString(level);
        }
    }
    if (!s.empty())
        s += " ";
    return s + "<=" + second.toString(level);
}

size_t VersionRange::RangePair::getStdHash() const
{
    size_t h = 0;
    hash_combine(h, first.getStdHash());
    hash_combine(h, second.getStdHash());
    return h;
}

bool VersionRange::RangePair::isBranch() const
{
    return first.isBranch() || second.isBranch();
}

bool VersionRange::isBranch() const
{
    return !branch.empty();
}

std::string VersionRange::toString() const
{
    if (isBranch())
        return branch;

    std::string s;
    for (auto &p : range)
        s += p.toString() + " || ";
    if (!s.empty())
        s.resize(s.size() - 4);
    return s;
}

std::string VersionRange::toStringV1() const
{
    if (isBranch())
        return branch;

    if (range[0].first == Version::min() && range[0].second == Version::max())
        return "*"; // one star in v1!

    GenericNumericVersion::Numbers n;
    if (!(
        ((range[0].first == Version::min() && range[0].first.numbers[2] == 1) ||
        (range[0].first != Version::min() && range[0].first.numbers[2] == 0))
        && range[0].second.numbers[2] == Version::maxNumber()))
    {
        n.push_back(range[0].first.numbers[2]);
    }
    for (int i = 1; i >= 0; i--)
    {
        if (!(range[0].first.numbers[i] == 0 && range[0].second.numbers[i] == Version::maxNumber()))
            n.push_back(range[0].first.numbers[i]);
    }
    std::reverse(n.begin(), n.end());
    Version v;
    v.numbers = n;
    return v.toString();
}

size_t VersionRange::getStdHash() const
{
    if (isBranch())
        return std::hash<std::string>()(branch);
    size_t h = 0;
    for (auto &n : range)
        hash_combine(h, n.getStdHash());
    return h;
}

bool VersionRange::operator<(const VersionRange &rhs) const
{
    if (isBranch() && rhs.isBranch())
        return branch < rhs.branch;
    if (isBranch())
        return false;
    if (rhs.isBranch())
        return true;

    if (rhs.range.empty())
        return false;
    if (range.empty())
        return true;
    return range < rhs.range;
}

bool VersionRange::operator==(const VersionRange &rhs) const
{
    return range == rhs.range && branch == rhs.branch;
}

bool VersionRange::operator!=(const VersionRange &rhs) const
{
    return !operator==(rhs);
}

VersionRange &VersionRange::operator|=(const RangePair &rhs)
{
    if (isBranch())
        throw SW_RUNTIME_ERROR("Cannot |= with branch on LHS");
    if (rhs.isBranch())
    {
        if (!isEmpty())
            throw SW_RUNTIME_ERROR("Cannot |= with branch on RHS");

        range.clear();
        branch = rhs.toString();
        return *this;
    }

    // we can do pairs |= p
    // but we still need to merge overlapped after
    // so we choose simple iterative approach instead

    bool added = false;
    for (auto i = range.begin(); i < range.end(); i++)
    {
        if (!added)
        {
            // skip add, p is greater
            if (i->second < rhs.first)
            {
                // but if it is greater only on 1, we merge intervals
                auto v = i->second;
                if (++v == rhs.first)
                    goto merge;
            }
            // insert as is
            else if (i->first > rhs.second)
            {
                i = range.insert(i, rhs);
                break; // no further merges requires
            }
            // merge overlapped
            else
            {
            merge:
                i->first = std::min(i->first, rhs.first);
                i->second = std::max(i->second, rhs.second);
                added = true;
            }
        }
        else
        {
            // after merging with existing entry we must ensure that
            // following intervals do not require merges too
            if ((i - 1)->second < i->first)
                break;
            else
            {
                (i - 1)->second = std::max((i - 1)->second, i->second);
                i = range.erase(i) - 1;
            }
        }
    }
    if (!added)
        range.push_back(rhs);
    return *this;
}

VersionRange &VersionRange::operator&=(const RangePair &rhs)
{
    if (isBranch())
        throw SW_RUNTIME_ERROR("Cannot &= with branch on LHS");
    if (rhs.isBranch())
    {
        if (!isEmpty())
            throw SW_RUNTIME_ERROR("Cannot &= with branch on RHS");

        range.clear();
        branch = rhs.toString();
        return *this;
    }

    if (range.empty())
        return *this;

    auto rp = rhs;
    for (auto i = range.begin(); i < range.end(); i++)
    {
        rp.first = std::max(i->first, rp.first);
        rp.second = std::min(i->second, rp.second);

        if (rp.first > rp.second)
        {
            range.clear();
            return *this;
        }
    }
    range.clear();
    range.push_back(rp);
    return *this;
}

VersionRange &VersionRange::operator|=(const VersionRange &rhs)
{
    if (isBranch())
        throw SW_RUNTIME_ERROR("Cannot |= with branch on LHS");
    if (rhs.isBranch())
    {
        if (!isEmpty())
            throw SW_RUNTIME_ERROR("Cannot |= with branch on RHS");

        range.clear();
        branch = rhs.toString();
        return *this;
    }
    for (auto &rp : rhs.range)
        operator|=(rp);
    return *this;
}

VersionRange &VersionRange::operator&=(const VersionRange &rhs)
{
    if (isBranch())
        throw SW_RUNTIME_ERROR("Cannot &= with branch on LHS");
    if (rhs.isBranch())
    {
        if (!isEmpty())
            throw SW_RUNTIME_ERROR("Cannot &= with branch on RHS");

        range.clear();
        branch = rhs.toString();
        return *this;
    }
    VersionRange vr;
    vr.range.clear();
    for (auto &rp1 : range)
    {
        for (auto &rp2 : rhs.range)
        {
            if (auto o = rp1 & rp2; o)
                vr |= o.value();
        }
    }
    return *this = vr;
}

VersionRange VersionRange::operator|(const VersionRange &rhs) const
{
    auto tmp = *this;
    tmp |= rhs;
    return tmp;
}

VersionRange VersionRange::operator&(const VersionRange &rhs) const
{
    auto tmp = *this;
    tmp &= rhs;
    return tmp;
}

/*VersionRange VersionRange::operator~() const
{
    auto tmp = *this;
    throw SW_RUNTIME_ERROR("not implemented");
    return tmp;
}*/

bool operator<(const VersionRange &r, const Version &v)
{
    if (r.isEmpty())
        return false;
    if (r.isBranch())
        return Version(r.branch) < v;
    return r.range.back().second < v;
}

bool operator<(const Version &v, const VersionRange &r)
{
    if (r.isEmpty())
        return false;
    if (r.isBranch())
        return v < Version(r.branch);
    return v < r.range.front().first;
}

bool operator>(const VersionRange &r, const Version &v)
{
    if (r.isEmpty())
        return false;
    if (r.isBranch())
        return Version(r.branch) > v;
    return r.range.front().first > v;
}

bool operator>(const Version &v, const VersionRange &r)
{
    if (r.isEmpty())
        return false;
    if (r.isBranch())
        return v > Version(r.branch);
    return v > r.range.back().second;
}

std::optional<Version> VersionRange::getMinSatisfyingVersion(const std::set<Version> &s) const
{
    for (auto &v : s)
    {
        if (hasVersion(v))
            return v;
    }
    return {};
}

std::optional<Version> VersionRange::getMaxSatisfyingVersion(const std::set<Version> &s) const
{
    for (auto i = s.rbegin(); i != s.rend(); i++)
    {
        if (hasVersion(*i))
            return *i;
    }
    return {};
}

}
