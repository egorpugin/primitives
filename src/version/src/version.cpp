#include <primitives/version.h>

#include <pystring.h>

#include <algorithm>

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

Version::Version()
{
    patch = 1;
}

Version::Version(const Extra &e)
    : Version()
{
    extra = e;
    checkExtra();
}

Version::Version(const std::string &s)
{
    if (!parse(*this, preprocess_input(s)))
        throw_bad_version(s);
}

Version::Version(Number ma)
    : major(ma)
{
    if (major < 0)
        throw_bad_version(toString());
    else if (major == 0)
        patch = 1;
}

Version::Version(Number ma, Number mi)
    : major(ma), minor(mi)
{
    if (major < 0 || minor < 0)
        throw_bad_version(toString());
    else if (major == 0 && minor == 0)
        patch = 1;
}

Version::Version(Number ma, Number mi, Number pa)
    : major(ma), minor(mi), patch(pa)
{
    if (major < 0 || minor < 0 || patch < 0)
        throw_bad_version(toString());
    else if (major == 0 && minor == 0 && patch == 0)
    {
        tweak = 1;
        level = 4;
    }
}

Version::Version(Number ma, Number mi, Number pa, Number tw)
    : major(ma), minor(mi), patch(pa), tweak(tw)
{
    level = 4;
    if (major < 0 || minor < 0 || patch < 0 || tweak < 0)
        throw_bad_version(toString());
    else if (major == 0 && minor == 0 && patch == 0 && tweak == 0)
        throw_bad_version(toString());
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
    return major;
}

Version::Number Version::getMinor() const
{
    return minor;
}

Version::Number Version::getPatch() const
{
    return patch;
}

Version::Number Version::getTweak() const
{
    return tweak;
}

Version::Extra Version::getExtra() const
{
    return extra;
}

std::string Version::getBranch() const
{
    return branch;
}

std::string Version::toString(int level) const
{
    if (!branch.empty())
        return branch;

    auto s = printVersion(level);
    if (!extra.empty())
        s += "-" + printExtra();
    return s;
}

std::string Version::toString(const Version &v) const
{
    return toString(v.level);
}

std::string Version::printVersion(int level) const
{
    std::string s;
    s += std::to_string(major);
    s += "." + std::to_string(minor);
    s += "." + std::to_string(patch);
    if (tweak > 0 || level > 3)
        s += "." + std::to_string(tweak);
    return s;
}

std::string Version::printExtra() const
{
    std::string s;
    for (auto &e : extra)
        s += e + ".";
    s.resize(s.size() - 1);
    return s;
}

void Version::checkExtra() const
{
    Version v;
    auto c = std::any_of(extra.begin(), extra.end(),
        [&v](const auto &e) { return !parseExtra(v, e); });
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
    return std::tie(major, minor, patch, tweak, extra) < std::tie(rhs.major, rhs.minor, rhs.patch, rhs.tweak, rhs.extra);
}

bool Version::operator>(const Version &rhs) const
{
    if (isBranch() && rhs.isBranch())
        return branch > rhs.branch;
    if (isBranch())
        return true;
    if (rhs.isBranch())
        return false;
    return std::tie(major, minor, patch, tweak, extra) > std::tie(rhs.major, rhs.minor, rhs.patch, rhs.tweak, rhs.extra);
}

bool Version::operator<=(const Version &rhs) const
{
    if (isBranch() && rhs.isBranch())
        return branch <= rhs.branch;
    if (isBranch())
        return false;
    if (rhs.isBranch())
        return true;
    return std::tie(major, minor, patch, tweak, extra) <= std::tie(rhs.major, rhs.minor, rhs.patch, rhs.tweak, rhs.extra);
}

bool Version::operator>=(const Version &rhs) const
{
    if (isBranch() && rhs.isBranch())
        return branch >= rhs.branch;
    if (isBranch())
        return true;
    if (rhs.isBranch())
        return false;
    return std::tie(major, minor, patch, tweak, extra) >= std::tie(rhs.major, rhs.minor, rhs.patch, rhs.tweak, rhs.extra);
}

bool Version::operator==(const Version &rhs) const
{
    if (isBranch() && rhs.isBranch())
        return branch == rhs.branch;
    if (isBranch() || rhs.isBranch())
        return false;
    return std::tie(major, minor, patch, tweak, extra) == std::tie(rhs.major, rhs.minor, rhs.patch, rhs.tweak, rhs.extra);
}

bool Version::operator!=(const Version &rhs) const
{
    return !operator==(rhs);
}

Version Version::min(int level)
{
    if (level > 3)
        return Version(0, 0, 0);
    return Version();
}

Version Version::min(const Version &v)
{
    return min(v.level);
}

Version Version::max(int level)
{
    if (level > 3)
        return Version(maxNumber(), maxNumber(), maxNumber(), maxNumber());
    return Version(maxNumber(), maxNumber(), maxNumber());
}

Version Version::max(const Version &v)
{
    return max(v.level);
}

Version::Number Version::maxNumber()
{
    // currently set a limit up to 1 billion excluding
    return 999'999'999;
}

void Version::decrementVersion()
{
    decrementVersion(level);
}

void Version::incrementVersion()
{
    incrementVersion(level);
}

Version Version::getNextVersion() const
{
    return getNextVersion(level);
}

Version Version::getPreviousVersion() const
{
    return getPreviousVersion(level);
}

void Version::decrementVersion(int l)
{
    if (*this == min(*this))
        return;

    auto current = l > 3 ? &tweak : &(&major)[l - 1];
    while (*current == 0)
        *current-- = maxNumber();
    (*current)--;
}

void Version::incrementVersion(int l)
{
    if (*this == max(*this))
        return;

    auto current = l > 3 ? &tweak : &(&major)[l - 1];
    while (*current == maxNumber())
        *current-- = 0;
    (*current)++;
}

Version Version::getNextVersion(int l) const
{
    auto v = *this;
    v.incrementVersion(l);
    return v;
}

Version Version::getPreviousVersion(int l) const
{
    auto v = *this;
    v.decrementVersion(l);
    return v;
}

bool VersionRange::RangePair::hasVersion(const Version &v) const
{
    if (v.level > std::max(first.level, second.level))
        return false;
    return first <= v && v <= second;
}

VersionRange::VersionRange()
{
    range.emplace_back(Version::min(), Version::max());
}

VersionRange::VersionRange(const std::string &s)
{
    auto in = preprocess_input(s);
    if (in.empty())
        range.emplace_back(Version::min(), Version::max());
    else if (!parse(*this, in))
        throw_bad_version(in);
}

optional<VersionRange> VersionRange::parse(const std::string &s)
{
    VersionRange vr;
    auto in = preprocess_input(s);
    if (in.empty())
        vr.range.emplace_back(Version::min(), Version::max());
    else if (!parse(vr, in))
        return {};
    return vr;
}

bool VersionRange::empty() const
{
    return range.empty();
}

bool VersionRange::hasVersion(const Version &v) const
{
    return std::any_of(range.begin(), range.end(),
            [&v](const auto &r) { return r.hasVersion(v); });
}

std::string VersionRange::RangePair::toString() const
{
    auto level = std::max(first.level, second.level);
    if (first == second)
        return first.toString(level);
    std::string s;
    if (first != Version::min(first))
    {
        s += ">=";
        s += first.toString(level);
    }
    if (second.major == Version::maxNumber() ||
        second.minor == Version::maxNumber() ||
        second.patch == Version::maxNumber() ||
        second.tweak == Version::maxNumber()
        )
    {
        if (second == Version::max(second))
        {
            if (s.empty())
                return "*";
            return s;
        }
        if (second.patch == Version::maxNumber() ||
            second.tweak == Version::maxNumber())
        {
            if (!s.empty())
                s += " ";
            return s + "<" + second.getNextVersion().toString(level);
        }
    }
    if (!s.empty())
        s += " ";
    return s + "<=" + second.toString(level);
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

    auto p = rhs;
    for (auto i = range.begin(); i < range.end(); i++)
    {
        p.first = std::max(i->first, p.first);
        p.second = std::min(i->second, p.second);

        if (p.first > p.second)
        {
            range.clear();
            return *this;
        }
    }
    range.clear();
    range.push_back(p);
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
    for (auto &rp : rhs.range)
        operator&=(rp);
    return *this;
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

}
