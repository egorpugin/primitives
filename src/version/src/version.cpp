#include <primitives/version.h>

#include <boost/algorithm/string.hpp>

static std::string preprocess_input(const std::string &in)
{
    return boost::trim_copy(in);
}

static std::string preprocess_input_extra(const std::string &in)
{
    return boost::to_lower_copy(preprocess_input(in));
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
        tweak = 1;
}

Version::Version(Number ma, Number mi, Number pa, Number tw)
    : major(ma), minor(mi), patch(pa), tweak(tw)
{
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

std::string Version::toString(Number tw) const
{
    if (!branch.empty())
        return branch;

    auto s = printVersion(tw);
    if (!extra.empty())
        s += "-" + printExtra();
    return s;
}

std::string Version::toString(const Version &v) const
{
    return toString(v.tweak);
}

std::string Version::printVersion(Number tw) const
{
    std::string s;
    s += std::to_string(major);
    s += "." + std::to_string(minor);
    s += "." + std::to_string(patch);
    if (tweak > 0 || tw > 0)
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

Version Version::min(Number tweak)
{
    if (tweak > 0)
        return Version(0, 0, 0);
    return Version();
}

Version Version::min(const Version &v)
{
    return min(v.tweak);
}

Version Version::max(Number tweak)
{
    if (tweak > 0)
        return Version(maxNumber(), maxNumber(), maxNumber(), maxNumber());
    return Version(maxNumber(), maxNumber(), maxNumber());
}

Version Version::max(const Version &v)
{
    return max(v.tweak);
}

Version::Number Version::maxNumber()
{
    // currently set a limit up to 1 bi excluding
    return 999'999'999;
}

void Version::decrementVersion()
{
    if (*this == min(*this))
        return;

    auto current = tweak != 0 ? &tweak : &patch;
    while (*current == 0)
        *current-- = maxNumber();
    (*current)--;
}

void Version::incrementVersion()
{
    if (*this == max(*this))
        return;

    auto current = tweak != 0 ? &tweak : &patch;
    while (*current == maxNumber())
        *current-- = 0;
    (*current)++;
}

Version Version::getNextVersion() const
{
    auto v = *this;
    v.incrementVersion();
    return v;
}

Version Version::getPreviousVersion() const
{
    auto v = *this;
    v.decrementVersion();
    return v;
}

VersionRange::VersionRange()
{
    range.emplace_back(Version::min(), Version::max());
}

VersionRange::VersionRange(const std::string &s)
{
    if (!parse(*this, preprocess_input(s)))
        throw_bad_version(s);
    normalize();
}

void VersionRange::normalize(bool and_)
{
    if (range.size() < 2)
        return;

    std::sort(range.begin(), range.end());

    Range range2;
    if (and_)
    {
    }
    else
    {
        for (auto r1 = range.begin(); r1 < range.end(); r1++)
        {
            bool processed = false;
            for (auto r2 = r1 + 1; r2 < range.end(); r2++)
            {
                if (r1->second >= r2->second)
                    continue;
                if (r1->second >= r2->first)
                {
                    range2.emplace_back(r1->first, r2->second);
                    r1 = r2;
                }
                else
                {
                    range2.emplace_back(*r1);
                    r1 = r2 - 1;
                }
                processed = true;
                break;
            }
            if (!processed)
            {
                range2.emplace_back(*r1);
                break;
            }
        }
    }
    range = range2;
}

std::string VersionRange::RangePair::toString() const
{
    auto tw = std::max(first.tweak, second.tweak);
    if (first == second)
        return first.toString(tw);
    std::string s;
    if (first != Version::min(first))
    {
        s += ">=";
        s += first.toString(tw);
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
            return s + "<" + second.getNextVersion().toString(tw);
        }
    }
    if (!s.empty())
        s += " ";
    return s + "<=" + second.toString(tw);
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

VersionRange::RangePair VersionRange::RangePair::operator|(const RangePair &rhs) const
{

}

VersionRange::RangePair VersionRange::RangePair::operator&(const RangePair &) const
{

}

VersionRange::RangePair &VersionRange::RangePair::operator|=(const RangePair &) const
{

}

VersionRange::RangePair &VersionRange::RangePair::operator&=(const RangePair &) const
{

}


}
