#include <primitives/version.h>

#include "range_parser.h"

#include <fmt/format.h>
#include <primitives/constants.h>
#include <primitives/hash.h>

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
    throw std::runtime_error("Invalid version: " + s);
}

static void throw_bad_version_extra(const std::string &s)
{
    throw std::runtime_error("Invalid version extra: " + s);
}

static void throw_bad_version_range(const std::string &s)
{
    throw std::runtime_error("Invalid version range: " + s);
}

namespace primitives::version
{

GenericNumericVersion::GenericNumericVersion()
    : GenericNumericVersion(minimum_level)
{
}

GenericNumericVersion::GenericNumericVersion(empty_tag, Level level)
{
    numbers.resize(level);
}

GenericNumericVersion::GenericNumericVersion(Level level)
{
    numbers.resize(level - 1);
    numbers.push_back(1);
}

GenericNumericVersion::GenericNumericVersion(const std::initializer_list<Number> &list, Level level)
    : numbers(list)
{
    if (std::any_of(numbers.begin(), numbers.end(), [](const auto &n) {return n < 0; }))
        throw std::runtime_error("Version number cannot be less than 0");
    auto l = std::max(minimum_level, level);
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
        //throw std::runtime_error("There is no that much numbers");
    return numbers[level];
}

std::string GenericNumericVersion::printVersion() const
{
    std::string s;
    for (auto &n : numbers)
        s += "." + std::to_string(n);
    s = s.substr(1);
    return s;
}

GenericNumericVersion::Level GenericNumericVersion::getLevel() const
{
    return numbers.size();
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

Version::Version()
{
}

Version::Version(const Extra &e)
    : Version()
{
    extra = e;
    checkExtra();
}

Version::Version(const std::string &s)
    : GenericNumericVersion(empty_tag{})
{
    if (!parse(*this, preprocess_input(s)))
        throw_bad_version(s);
    if (isBranch() && branch.size() > 200)
        throw_bad_version(s + ", branch must have size <= 200");
}

Version::Version(const char *s)
    : Version(std::string(s))
{
}

Version::Version(Level level)
    : GenericNumericVersion(level)
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

const Version::Extra &Version::getExtra() const
{
    return extra;
}

std::string Version::getBranch() const
{
    return branch;
}

std::string Version::toString(Level level) const
{
    if (!branch.empty())
        return branch;

    auto s = printVersion(/*level*/);
    if (!extra.empty())
        s += "-" + printExtra();
    return s;
}

std::string Version::toString(const Version &v) const
{
    return toString(v.numbers.size());
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

bool Version::operator<(const Version &rhs) const
{
    if (isBranch() && rhs.isBranch())
        return branch < rhs.branch;
    if (isBranch())
        return false;
    if (rhs.isBranch())
        return true;
    if (numbers < rhs.numbers)
        return true;
    if (numbers == rhs.numbers)
    {
        if (extra.empty())
            return false; // >=
        if (rhs.extra.empty())
            return true;
        return extra < rhs.extra;
    }
    return false;
}

bool Version::operator>(const Version &rhs) const
{
    if (isBranch() && rhs.isBranch())
        return branch > rhs.branch;
    if (isBranch())
        return true;
    if (rhs.isBranch())
        return false;
    return std::tie(numbers, extra) > std::tie(rhs.numbers, rhs.extra);
}

bool Version::operator<=(const Version &rhs) const
{
    if (isBranch() && rhs.isBranch())
        return branch <= rhs.branch;
    if (isBranch())
        return false;
    if (rhs.isBranch())
        return true;
    return std::tie(numbers, extra) <= std::tie(rhs.numbers, rhs.extra);
}

bool Version::operator>=(const Version &rhs) const
{
    if (isBranch() && rhs.isBranch())
        return branch >= rhs.branch;
    if (isBranch())
        return true;
    if (rhs.isBranch())
        return false;
    return std::tie(numbers, extra) >= std::tie(rhs.numbers, rhs.extra);
}

bool Version::operator==(const Version &rhs) const
{
    if (isBranch() && rhs.isBranch())
        return branch == rhs.branch;
    if (isBranch() || rhs.isBranch())
        return false;
    return std::tie(numbers, extra) == std::tie(rhs.numbers, rhs.extra);
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
    return Version(level);
}

Version Version::min(const Version &v)
{
    return min(v.getLevel());
}

Version Version::max(Level level)
{
    Version v(level);
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

    s = fmt::format(s,
        fmt::arg("M", getMajor()),
        fmt::arg("m", getMinor()),
        fmt::arg("p", getPatch()),
        fmt::arg("t", getTweak()),
        fmt::arg("5", get(4)),
        // letter variants
        // captical
        fmt::arg("ML", create_latin_replacements_capital(getMajor())),
        fmt::arg("mL", create_latin_replacements_capital(getMinor())),
        fmt::arg("pL", create_latin_replacements_capital(getPatch())),
        fmt::arg("tL", create_latin_replacements_capital(getTweak())),
        fmt::arg("5L", create_latin_replacements_capital(get(4))),
        // small
        fmt::arg("Ml", create_latin_replacements(getMajor())),
        fmt::arg("ml", create_latin_replacements(getMinor())),
        fmt::arg("pl", create_latin_replacements(getPatch())),
        fmt::arg("tl", create_latin_replacements(getTweak())),
        fmt::arg("5l", create_latin_replacements(get(4))),
        //
        fmt::arg("e", printExtra()),
        fmt::arg("b", branch),
        fmt::arg("v", toString())
    );
}

bool VersionRange::RangePair::hasVersion(const Version &v) const
{
    if (v.getLevel() > std::max(first.getLevel(), second.getLevel()))
        return false;
    return first <= v && v <= second;
}

VersionRange::VersionRange(GenericNumericVersion::Level level)
    : VersionRange(Version::min(level), Version::max(level))
{
}

VersionRange::VersionRange(const Version &v1, const Version &v2)
{
    if (v1 > v2)
        range.emplace_back(v2, v1);
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

optional<std::string> VersionRange::parse(VersionRange &vr, const std::string &s)
{
    if (s.size() > 32_KB)
        return { "Range string length must be less than 32K" };

    RangeParser d;
    //d.set_debug_level(d.bb.debug);
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

optional<VersionRange> VersionRange::parse(const std::string &s)
{
    VersionRange vr;
    auto in = preprocess_input(s);
    if (in.empty())
        vr.range.emplace_back(Version::min(), Version::max());
    else if (auto r = parse(vr, in); r)
        return {};
    return vr;
}

bool VersionRange::isEmpty() const
{
    return range.empty();
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
    return std::any_of(range.begin(), range.end(),
        [&v](const auto &r) { return r.hasVersion(v); });
}

optional<VersionRange::RangePair> VersionRange::RangePair::operator&(const RangePair &rhs) const
{
    VersionRange::RangePair rp;
    rp.first = std::max(first, rhs.first);
    rp.second = std::min(second, rhs.second);
    if (rp.first > rp.second)
        return {};
    return rp;
}

std::string VersionRange::RangePair::toString() const
{
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
                return "*";
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

std::string VersionRange::toString() const
{
    std::string s;
    for (auto &p : range)
        s += p.toString() + " || ";
    if (!s.empty())
        s.resize(s.size() - 4);
    return s;
}

size_t VersionRange::getStdHash() const
{
    size_t h = 0;
    for (auto &n : range)
        hash_combine(h, n.getStdHash());
    return h;
}

bool VersionRange::operator<(const VersionRange &rhs) const
{
    if (rhs.range.empty())
        return false;
    if (range.empty())
        return true;
    return range < rhs.range;
}

bool VersionRange::operator==(const VersionRange &rhs) const
{
    return range == rhs.range;
}

bool VersionRange::operator!=(const VersionRange &rhs) const
{
    return !operator==(rhs);
}

VersionRange &VersionRange::operator|=(const RangePair &p)
{
    // we can do pairs |= p
    // but we still need to merge overlapped after
    // so we choose simple iterative approach instead

    bool added = false;
    for (auto i = range.begin(); i < range.end(); i++)
    {
        if (!added)
        {
            // skip add, p is greater
            if (i->second < p.first)
                ;
            // insert as is
            else if (i->first > p.second)
            {
                i = range.insert(i, p);
                break; // no further merges requires
            }
            // merge overlapped
            else
            {
                i->first = std::min(i->first, p.first);
                i->second = std::max(i->second, p.second);
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
        range.push_back(p);
    return *this;
}

VersionRange &VersionRange::operator&=(const RangePair &rhs)
{
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
    for (auto &rp : rhs.range)
        operator|=(rp);
    return *this;
}

VersionRange &VersionRange::operator&=(const VersionRange &rhs)
{
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

bool operator<(const VersionRange &r, const Version &v)
{
    if (r.isEmpty())
        return false;
    return r.range.back().second < v;
}

bool operator<(const Version &v, const VersionRange &r)
{
    if (r.isEmpty())
        return false;
    return v < r.range.front().first;
}

bool operator>(const VersionRange &r, const Version &v)
{
    if (r.isEmpty())
        return false;
    return r.range.front().first > v;
}

bool operator>(const Version &v, const VersionRange &r)
{
    if (r.isEmpty())
        return false;
    return v > r.range.back().second;
}

optional<Version> VersionRange::getMinSatisfyingVersion(const std::set<Version> &s) const
{
    for (auto &v : s)
    {
        if (hasVersion(v))
            return v;
    }
    return {};
}

optional<Version> VersionRange::getMaxSatisfyingVersion(const std::set<Version> &s) const
{
    for (auto i = s.rbegin(); i != s.rend(); i++)
    {
        if (hasVersion(*i))
            return *i;
    }
    return {};
}

}
