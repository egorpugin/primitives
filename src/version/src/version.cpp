#include <primitives/version.h>

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
    if (!parse(*this, s))
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
    if (!parseExtra(*this, e))
        throw_bad_version_extra(e);
}

Version::Version(Number ma, Number mi, const std::string &e)
    : Version(ma, mi)
{
    if (!parseExtra(*this, e))
        throw_bad_version_extra(e);
}

Version::Version(Number ma, Number mi, Number pa, const std::string &e)
    : Version(ma, mi, pa)
{
    if (!parseExtra(*this, e))
        throw_bad_version_extra(e);
}

Version::Version(Number ma, Number mi, Number pa, Number tw, const std::string &e)
    : Version(ma, mi, pa, tw)
{
    if (!parseExtra(*this, e))
        throw_bad_version_extra(e);
}

std::string Version::toString() const
{
    if (!branch.empty())
        return branch;

    auto s = printVersion();
    if (!extra.empty())
        s += "-" + printExtra();
    return s;
}

std::string Version::printVersion() const
{
    std::string s;
    s += std::to_string(major);
    s += "." + std::to_string(minor);
    s += "." + std::to_string(patch);
    if (tweak > 0)
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

bool Version::operator==(const Version &rhs) const
{
    if (isBranch() && rhs.isBranch())
        return branch == rhs.branch;
    if (isBranch() || rhs.isBranch())
        return false;
    return std::tie(major, minor, patch, tweak, extra) == std::tie(rhs.major, rhs.minor, rhs.patch, rhs.tweak, rhs.extra);
}

Version Version::min()
{
    return Version(0, 0, 0);
}

Version Version::max()
{
    return Version(
        maxNumber(),
        maxNumber(),
        maxNumber(),
        maxNumber());
}

Version::Number Version::maxNumber()
{
    // currently set a limit up to 1 bi excluding
    return 999'999'999;
}

void Version::decrementVersion()
{
    if (tweak > 0)
    {
        if (tweak != 1)
            tweak--;
        else if (major > 0 || minor > 0 || patch > 0)
            tweak--;
    }
    else if (patch > 0)
    {
        if (patch != 1)
            patch--;
        else if (major > 0 || minor > 0)
            patch--;
    }
    else if (minor > 0)
    {
        if (minor != 1)
        {
            minor--;
            patch = maxNumber();
        }
        else if (major > 0)
        {
            minor--;
            patch = maxNumber();
        }
    }
    else if (major > 0)
    {
        major--;
        minor = maxNumber();
        patch = maxNumber();
    }
}

void Version::incrementVersion()
{
    // carry overflows like zero underflows in decrement?
    if (tweak > 0)
        tweak++;
    else
        patch++;
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
    range.emplace(Version::min(), Version::max());
}

VersionRange::VersionRange(const std::string &s)
{
    if (!parse(*this, s))
        throw_bad_version(s);
    normalize();
}

void VersionRange::normalize()
{

}

std::string VersionRange::RangePair::toString() const
{
    if (first == second)
        return first.toString();
    return first.toString() + " - " + second.toString();
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

}
