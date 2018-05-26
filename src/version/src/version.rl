#include <primitives/version.h>

namespace primitives::version
{

bool Version::parse(Version &v, const std::string &s)
{
    bool ok = false;
	int cs = 0;
	auto p = s.data();
    auto pe = p + s.size();
    auto eof = pe;

    Version::Number n; // number
    auto sb = p; // string begin

    auto part = &v.major;

    %%{
        machine Version;
	    write data;

        action SB { sb = p; }
        action OK { ok = p == pe; }

        alpha_ = alpha | '_';
        alnum_ = alnum | '_';

        number = digit+ >SB %{ n = std::stoll(std::string{sb, p}); };
        number_version_part = number %{ *part++ = n; };
        #basic_version = [=]? [v]? number_version_part ('.' number_version_part){,3};
        basic_version = number_version_part ('.' number_version_part){,3};
        extra_part = alnum_+ >SB %{ v.extra.emplace_back(sb, p); };
        extra = extra_part ('.' extra_part)*;
        version = basic_version ('-' extra)?;

        branch = (alpha_ alnum_*) >SB %{ v.branch.assign(sb, p); };

        main := (version | branch) %OK;

		write init;
		write exec;
    }%%

    if (v.major == 0 && v.minor == 0 && v.patch == 0 && v.tweak == 0)
    {
        if (part != &v.tweak)
            v.patch = 1;
        else
            v.tweak = 1;
    }

    return ok;
}

bool Version::parseExtra(Version &v, const std::string &s)
{
    bool ok = false;
	int cs = 0;
	auto p = s.data();
    auto pe = p + s.size();
    auto eof = pe;

    auto sb = p; // string begin

    %%{
        machine VersionExtra;
	    write data;

        action SB { sb = p; }
        action OK { ok = p == pe; }

        alnum_ = alnum | '_';

        extra_part = alnum_+ >SB %{ v.extra.emplace_back(sb, p); };
        extra = extra_part ('.' extra_part)*;

        main := extra %OK;

		write init;
		write exec;
    }%%

    return ok;
}

bool VersionRange::parse(VersionRange &vr, const std::string &ts)
{
    bool ok = false;
	int cs = 0;
	auto p = ts.data();
    auto pe = p + ts.size();
    auto eof = pe;

    Version::Number n; // number
    auto sb = p; // string begin

    enum { ANY = -2, UNSET = -1, };

    // on stack variables do not follow each other
    struct
    {
        Version::Number major;
        Version::Number minor;
        Version::Number patch;
        Version::Number tweak;
    } vn;

    Version::Extra extra;

    RangePair rp;
    Version v;
    Version::Number *part;
    VersionRange vr_and;

    enum { LT, GT, LE, GE, EQ, NE, } sign;

    %%{
        machine VersionRange;
		write data;

        action SB { sb = p; }
        action OK { ok = p == pe; }

        action CMP
        {
            switch (sign)
            {
            case LT:
                rp.first = Version::min();
                rp.second = v.getPreviousVersion();
                break;
            case LE:
                rp.first = Version::min();
                rp.second = v;
                break;
            case GT:
                rp.first = v.getNextVersion();
                rp.second = Version::max();
                break;
            case GE:
                rp.first = v;
                rp.second = Version::max();
                break;
            case EQ:
                rp.first = v;
                rp.second = v;
                break;
            case NE:
                rp.first = Version::min();
                rp.second = v.getPreviousVersion();
                vr.range.insert(rp);
                rp.first = v.getNextVersion();
                rp.second = Version::max();
                break;
            }
            vr.range.insert(rp);
        }

        action RESET_VER
        {
            vn.major = UNSET;
            vn.minor = UNSET;
            vn.patch = UNSET;
            vn.tweak = UNSET;
            extra.clear();
            part = &vn.major;
        }

        action BUILD_VER
        {
            v.major = vn.major;
            v.minor = vn.minor == UNSET ? 0 : vn.minor;
            v.patch = vn.patch == UNSET ? 0 : vn.patch;
            v.tweak = vn.tweak == UNSET ? 0 : vn.tweak;
            v.extra = extra;
        }

        alpha_ = alpha | '_';
        alnum_ = alnum | '_';
        logical_or = space* '||' space*;
        logical_and = space+ | space* (',' | '&&') space*;

        # from version - find a way to merge
        #
        number = digit+ >SB %{ n = std::stoll(std::string{sb, p}); };
        number_version_part = number %{ *part++ = n; } | [Xx*] %{ *part++ = ANY; };
        basic_version = number_version_part ('.' number_version_part){,3};
        extra_part = alnum_+ >SB %{ v.extra.emplace_back(sb, p); };
        extra = extra_part ('.' extra_part)*;
        version = (basic_version ('-' extra)?) >RESET_VER %BUILD_VER;
        #

        hyphen = version %{ rp.first = v; } space+ '-' space+ version %{ rp.second = v; vr.range.insert(rp); };
        tilde = '~' space* version;
        caret = '^' space* version;
        cmp = (
            '<'  %{ sign = LT; } |
            '>'  %{ sign = GT; } |
            '<=' %{ sign = LE; } |
            '>=' %{ sign = GE; } |
            '='  %{ sign = EQ; } |
            '!=' %{ sign = NE; }
        ) space* version %CMP;

        range = version %{ sign = EQ; } %CMP | cmp | caret | tilde | hyphen;

        range_set_and = range (logical_and range)*;
        range_set_or = range_set_and (logical_or range_set_and)*;

        main := range_set_or %OK;

		write init;
		write exec;
    }%%

    return ok;
}

}
