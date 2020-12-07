#include <primitives/version_range.h>

#include <range_parser.h>

#include <fmt/format.h>
#include <primitives/constants.h>
#include <primitives/exceptions.h>
#include <primitives/hash_combine.h>
#include <primitives/string.h>

#include <pystring.h>

#include <algorithm>
#include <map>

const primitives::version::Version::Level minimum_level = 3;

primitives::version::Version prepare_version(
    primitives::version::Version &ver, primitives::version::Version::Number fill_value = 0,
    primitives::version::Version::Level level = minimum_level);

namespace primitives::version
{

detail::RangePair::RangePair(const Version &v1, bool strong_relation1, const Version &v2, bool strong_relation2)
    : first{ v1, strong_relation1 }, second{ v2, strong_relation2 }
{
    if (getFirst() > getSecond())
        throw SW_RUNTIME_ERROR("Left version must be <= than right: " + getFirst().toString() + " > " + getSecond().toString());

    // prepare pair
    /*auto level = std::max(getFirst().getLevel(), getSecond().getLevel());

    //
    prepare_version(first.v, 0, level);
    if (getFirst() < Version::min(level))
    {
        // keep extra for first
        auto e = getFirst().extra;
        first.v = Version::min(level);
        first.v.extra = e;
    }

    //
    prepare_version(second.v, Version::maxNumber(), level);
    if (getSecond() > Version::max(level))
    {
        // but not second?
        second.v = Version::max(level);
    }*/
}

detail::RangePair::RangePair(const Side &l, const Side &r)
    : first(l), second(r)
{
}

bool detail::RangePair::Side::operator<(const Side &rhs) const
{
    if (v != rhs.v)
        return v < rhs.v;
    if (strong_relation || rhs.strong_relation)
        return true;
    return strong_relation < rhs.strong_relation;
}

bool detail::RangePair::Side::operator<(const Version &rhs) const
{
    if (strong_relation)
        return v < rhs;
    else
        return v <= rhs;
}

bool detail::RangePair::Side::operator>(const Version &rhs) const
{
    if (strong_relation)
        return v > rhs;
    else
        return v >= rhs;
}

bool detail::RangePair::contains(const Version &v) const
{
    return first < v && second > v;
}

bool detail::RangePair::contains(const RangePair &v) const
{
    // unused for now
    SW_UNIMPLEMENTED;
    //return first <= v.first && v.second <= second;
}

std::optional<detail::RangePair> detail::RangePair::operator&(const detail::RangePair &rhs) const
{
    auto &f = std::max(first, rhs.first);
    auto &s = std::min(second, rhs.second);
    if (s < f)
        return {};
    return detail::RangePair(f, s);
}

std::string detail::RangePair::toString(VersionRangePairStringRepresentationType t) const
{
    SW_UNIMPLEMENTED;
    /*auto level = std::max(getFirst().getLevel(), getSecond().getLevel());
    if (t == VersionRangePairStringRepresentationType::SameRealLevel)
        level = std::max(getFirst().getRealLevel(), getSecond().getRealLevel());
    if (getFirst() == getSecond())
    {
        if (t == VersionRangePairStringRepresentationType::IndividualRealLevel)
            return getFirst().toString(getFirst().getRealLevel());
        else
            return getFirst().toString(level);
    }
    std::string s;
    if (getFirst() != Version::min(getFirst()))
    {
        s += ">=";
        if (t == VersionRangePairStringRepresentationType::IndividualRealLevel)
            s += getFirst().toString(getFirst().getRealLevel());
        else
            s += getFirst().toString(level);
    }
    // reconsider?
    // probably wrong now: 1.MAX.3
    if (std::any_of(getSecond().numbers.begin(), getSecond().numbers.end(),
        [](const auto &n) {return n == Version::maxNumber(); }))
    {
        if (getSecond() == Version::max(getSecond()))
        {
            if (s.empty())
                return "*";
            return s;
        }
        if (getSecond().getPatch() == Version::maxNumber() ||
            getSecond().getTweak() == Version::maxNumber())
        {
            if (!s.empty())
                s += " ";
            if (t == VersionRangePairStringRepresentationType::IndividualRealLevel)
            {
                auto nv = getSecond().getNextVersion(getSecond().getRealLevel());
                return s + "<" + nv.toString(nv.getRealLevel());
            }
            auto nv = getSecond().getNextVersion(level);
            if (t == VersionRangePairStringRepresentationType::SameRealLevel)
            {
                level = std::max(getFirst().getRealLevel(), nv.getRealLevel());
                s.clear();
                if (getFirst() != Version::min(getFirst()))
                {
                    s += ">=";
                    s += getFirst().toString(level);
                    if (!s.empty())
                        s += " ";
                }
            }
            return s + "<" + nv.toString(level);
        }
    }
    if (!s.empty())
        s += " ";
    if (t == VersionRangePairStringRepresentationType::IndividualRealLevel)
        return s + "<=" + getSecond().toString(getSecond().getRealLevel());
    else
        return s + "<=" + getSecond().toString(level);*/
}

std::optional<Version> detail::RangePair::toVersion() const
{
    if (getFirst() != getSecond())
        return {};
    return getFirst();
}

size_t detail::RangePair::getHash() const
{
    size_t h = 0;
    hash_combine(h, getFirst().getHash());
    hash_combine(h, getSecond().getHash());
    return h;
}

VersionRange::VersionRange()
    //: VersionRange(minimum_level)
{
}

/*VersionRange::VersionRange(Version::Level level)
    : VersionRange(Version::min(level), Version::max(level))
{
}*/

VersionRange::VersionRange(const Version &v)
    : VersionRange(v, v)
{
}

VersionRange::VersionRange(const Version &v1, const Version &v2)
{
    range.emplace_back(v1, false, v2, false);
}

VersionRange::VersionRange(const std::string &s)
{
    auto in = detail::preprocess_input(s);
    if (in.empty())
        SW_UNIMPLEMENTED;
        //range.emplace_back(Version::min(), Version::max());
    else if (auto r = parse(*this, in); r)
        throw SW_RUNTIME_ERROR("Invalid version range: " + in + ", error: " + r.value());
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
    auto in = detail::preprocess_input(s);
    if (in.empty())
        SW_UNIMPLEMENTED;
        //vr.range.emplace_back(Version::min(), Version::max());
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
    return range.empty();
}

bool VersionRange::contains(const char *s) const
{
    return contains(Version(s));
}

bool VersionRange::contains(const Version &v) const
{
    return std::any_of(range.begin(), range.end(),
        [&v](const auto &r) { return r.contains(v); });
}

bool VersionRange::contains(const VersionRange &r) const
{
    return (*this & r) == r;
}

std::optional<Version> VersionRange::toVersion() const
{
    if (size() != 1)
        return {};
    return range.begin()->toVersion();
}

std::string VersionRange::toString() const
{
    return toString(VersionRangePairStringRepresentationType::SameDefaultLevel);
}

std::string VersionRange::toString(VersionRangePairStringRepresentationType t) const
{
    std::string s;
    for (auto &p : range)
        s += p.toString(t) + " || ";
    if (!s.empty())
        s.resize(s.size() - 4);
    return s;
}

size_t VersionRange::getHash() const
{
    size_t h = 0;
    for (auto &n : range)
        hash_combine(h, n.getHash());
    return h;
}

bool VersionRange::operator==(const VersionRange &rhs) const
{
    return range == rhs.range;
}

VersionRange &VersionRange::operator|=(const detail::RangePair &rhs)
{
    // TODO: we need to merge overlapped
    range.push_back(rhs);
    return *this;
}

VersionRange &VersionRange::operator&=(const detail::RangePair &rhs)
{
    if (range.empty())
        return *this;

    auto rp = rhs;
    for (auto i = range.begin(); i < range.end(); i++)
    {
        SW_UNIMPLEMENTED;
        //rp.getFirst() = std::max(i->getFirst(), rp.getFirst());
        //rp.getSecond() = std::min(i->getSecond(), rp.getSecond());

        if (rp.getFirst() > rp.getSecond())
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

/*bool operator<(const VersionRange &r, const Version &v)
{
    if (r.isEmpty())
        return false;
    if (r.isBranch())
        return Version(r.branch) < v;
    return r.range.back().getSecond() < v;
}

bool operator<(const Version &v, const VersionRange &r)
{
    if (r.isEmpty())
        return false;
    if (r.isBranch())
        return v < Version(r.branch);
    return v < r.range.front().getFirst();
}

bool operator>(const VersionRange &r, const Version &v)
{
    if (r.isEmpty())
        return false;
    if (r.isBranch())
        return Version(r.branch) > v;
    return r.range.front().getFirst() > v;
}

bool operator>(const Version &v, const VersionRange &r)
{
    if (r.isEmpty())
        return false;
    if (r.isBranch())
        return v > Version(r.branch);
    return v > r.range.back().getSecond();
}*/

std::optional<Version> VersionRange::getMinSatisfyingVersion(const std::set<Version> &s) const
{
    for (auto &v : s)
    {
        if (contains(v))
            return v;
    }
    return {};
}

std::optional<Version> VersionRange::getMaxSatisfyingVersion(const std::set<Version> &s) const
{
    for (auto i = s.rbegin(); i != s.rend(); ++i)
    {
        if (contains(*i))
            return *i;
    }
    return {};
}

bool PackageVersionRange::isBranch() const
{
    return std::holds_alternative<Branch>(value);
}

bool PackageVersionRange::isRange() const
{
    return std::holds_alternative<VersionRange>(value);
}

const VersionRange &PackageVersionRange::getRange() const
{
    return std::get<VersionRange>(value);
}

const PackageVersion::Branch &PackageVersionRange::getBranch() const
{
    return std::get<Branch>(value);
}

}
