#pragma once

constexpr auto operator ""_B(unsigned long long int i)
{
    return i;
}

constexpr auto operator ""_KB(unsigned long long int i)
{
    return i * 1024_B;
}

constexpr auto operator ""_MB(unsigned long long int i)
{
    return i * 1024_KB;
}

constexpr auto operator ""_GB(unsigned long long int i)
{
    return i * 1024_MB;
}

constexpr auto operator ""_TB(unsigned long long int i)
{
    return i * 1024_GB;
}

constexpr auto operator ""_PB(unsigned long long int i)
{
    return i * 1024_TB;
}
