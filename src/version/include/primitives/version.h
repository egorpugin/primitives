#pragma once

#include <string>
#include <set>
#include <vector>
#include <regex>

namespace primitives::version
{

struct PRIMITIVES_VERSION_API Version
{
    using Number = int64_t;
    using Extra = std::vector<std::string>;

    /// default is 0.0.1
    Version();

    /// default is 0.0.1
    Version(const Extra &e);

    /// parse from string
    Version(const std::string &v);

    Version(Number ma);
    Version(Number ma, Number mi);
    Version(Number ma, Number mi, Number pa);
    Version(Number ma, Number mi, Number pa, Number tw);

    Version(Number ma, const Extra &e);
    Version(Number ma, Number mi, const Extra &e);
    Version(Number ma, Number mi, Number pa, const Extra &e);
    Version(Number ma, Number mi, Number pa, Number tw, const Extra &e);

    Version(Number ma, const std::string &e);
    Version(Number ma, Number mi, const std::string &e);
    Version(Number ma, Number mi, Number pa, const std::string &e);
    Version(Number ma, Number mi, Number pa, Number tw, const std::string &e);

    // basic access
    Number getMajor() const { return major; }
    Number getMinor() const { return minor; }
    Number getPatch() const { return patch; }
    Number getTweak() const { return tweak; }
    Extra getExtra() const { return extra; }
    std::string getBranch() const { return branch; }

    std::string toString() const;

    // modificators and getters?
    void decrementVersion(/* int level? */);
    void incrementVersion();
    Version getNextVersion() const;
    Version getPreviousVersion() const;

    bool isBranch() const;
    bool isVersion() const;

    bool operator<(const Version &) const;
    bool operator==(const Version &) const;

public:
    /// minimal version is 0.0.0.1 (or 0.0.1?)
    static Version min();

    /// maximal version is NumberMax.NumberMax.NumberMax.NumberMax (or NumberMax.NumberMax.NumberMax?)
    static Version max();

    /// maximal version is NumberMax.NumberMax.NumberMax.NumberMax (or NumberMax.NumberMax.NumberMax?)
    static Number maxNumber();

private:
    Number major = 0;
    Number minor = 0;
    Number patch = 0;
    Number tweak = 0;
    Extra extra;
    std::string branch;

    /// version will be in an invalid state in case of errors
    static bool parse(Version &v, const std::string &s);
    static bool parseExtra(Version &v, const std::string &s);

    std::string printVersion() const;
    std::string printExtra() const;

    void checkExtra() const;

    friend struct VersionRange;
};

struct PRIMITIVES_VERSION_API VersionRange
{
    struct RangePair : std::pair<Version, Version>
    {
        using base = std::pair<Version, Version>;
        using base::base;

        std::string toString() const;
    };
    using Range = std::set<RangePair>;

    /// default is any version or * or Version::min() - Version::max()
    VersionRange();

    /// parse from string
    VersionRange(const std::string &v);

    /// prints in normalized form
    std::string toString() const;

private:
    Range range;

    // make unions of intersected ranges
    void normalize();

    /// range will be in an invalid state in case of errors
    bool parse(VersionRange &v, const std::string &s);
};

}
