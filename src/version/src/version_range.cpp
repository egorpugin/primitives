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

static void throw_bad_version_range(const std::string &s)
{
    throw SW_RUNTIME_ERROR("Invalid version range: " + s);
}

namespace primitives::version
{

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
    auto in = detail::preprocess_input(s);
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
    auto in = detail::preprocess_input(s);
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

bool VersionRange::contains(const Version &v) const
{
    if (isBranch() && v.isBranch())
        return branch == v.getBranch();
    if (isBranch() || v.isBranch())
        return false;
    return std::any_of(range.begin(), range.end(),
        [&v](const auto &r) { return r.contains(v); });
}

bool VersionRange::RangePair::contains(const Version &v) const
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

std::string VersionRange::RangePair::toString(VersionRangePairStringRepresentationType t) const
{
    if (first.isBranch())
        return first.toString();
    if (second.isBranch())
        return second.toString();

    auto level = std::max(first.getLevel(), second.getLevel());
    if (t == VersionRangePairStringRepresentationType::SameRealLevel)
        level = std::max(first.getRealLevel(), second.getRealLevel());
    if (first == second)
    {
        if (t == VersionRangePairStringRepresentationType::IndividualRealLevel)
            return first.toString(first.getRealLevel());
        else
            return first.toString(level);
    }
    std::string s;
    if (first != Version::min(first))
    {
        s += ">=";
        if (t == VersionRangePairStringRepresentationType::IndividualRealLevel)
            s += first.toString(first.getRealLevel());
        else
            s += first.toString(level);
    }
    // reconsider?
    // probably wrong now: 1.MAX.3
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
            if (t == VersionRangePairStringRepresentationType::IndividualRealLevel)
            {
                auto nv = second.getNextVersion(second.getRealLevel());
                return s + "<" + nv.toString(nv.getRealLevel());
            }
            auto nv = second.getNextVersion(level);
            if (t == VersionRangePairStringRepresentationType::SameRealLevel)
            {
                level = std::max(first.getRealLevel(), nv.getRealLevel());
                s.clear();
                if (first != Version::min(first))
                {
                    s += ">=";
                    s += first.toString(level);
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
        return s + "<=" + second.toString(second.getRealLevel());
    else
        return s + "<=" + second.toString(level);
}

std::optional<Version> VersionRange::toVersion() const
{
    if (isBranch())
        return branch;
    if (size() != 1)
        return {};
    return range.begin()->toVersion();
}

std::optional<Version> VersionRange::RangePair::toVersion() const
{
    if (first != second)
        return {};
    return first;
}

size_t VersionRange::RangePair::getHash() const
{
    size_t h = 0;
    hash_combine(h, first.getHash());
    hash_combine(h, second.getHash());
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
    return toString(VersionRangePairStringRepresentationType::SameDefaultLevel);
}

std::string VersionRange::toString(VersionRangePairStringRepresentationType t) const
{
    if (isBranch())
        return branch;

    std::string s;
    for (auto &p : range)
        s += p.toString(t) + " || ";
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

size_t VersionRange::getHash() const
{
    if (isBranch())
        return std::hash<std::string>()(branch);
    size_t h = 0;
    for (auto &n : range)
        hash_combine(h, n.getHash());
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
        branch = rhs.toString(VersionRangePairStringRepresentationType::IndividualRealLevel);
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
        branch = rhs.toString(VersionRangePairStringRepresentationType::IndividualRealLevel);
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

}
