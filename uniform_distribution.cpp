#include "uniform_distribution.hpp"

using namespace std;

namespace silicontrip {
uniform_distribution::uniform_distribution() { lower = 0; upper =FLT_MAX; }
uniform_distribution::uniform_distribution(double a, double b) { lower = a; upper = b; }
uniform_distribution::uniform_distribution(const uniform_distribution& u) { lower = u.lower; upper = u.upper; }

double uniform_distribution::mean() const { return ( lower + upper ) / 2.0; }
double uniform_distribution::rounded_mean() const { return round(( lower + upper ) / 2.0); }

double uniform_distribution::range() const { return upper - lower; }
double uniform_distribution::rounded_range() const { return round(upper+0.000000001) - round(lower+0.000000001); }

// some say it should be (upper - lower)/2  / ((lower + upper) / 2)
double uniform_distribution::perror() const { return 100 * (upper - lower) / ((lower + upper) / 2.0 ); }

double uniform_distribution::get_lower() const { return lower; }
double uniform_distribution::get_upper() const { return upper; }


uniform_distribution uniform_distribution::inverse() const 
{ 
    double nlower = DBL_MAX;
    if (lower !=0)
        nlower = 1 / lower;
    double nupper = DBL_MAX;
    if (upper != 0)
        nupper = 1 / upper;

    if (nlower < nupper)
        return uniform_distribution(nlower,nupper);

    return uniform_distribution(nupper,nlower);
}

bool uniform_distribution::contains(double d) const { return (lower <= d && d <= upper); }

uniform_distribution uniform_distribution::intersection (const uniform_distribution& o) const
{
    double nlower=0.0;
    double nupper=0.0;
    if (o.contains(lower))
        nlower = lower;
    if (o.contains(upper))
        nupper=upper;

    if (contains(o.lower))
        nlower=o.lower;
    if (contains(o.upper))
        nupper=o.upper;

    if (nlower < nupper)
        return uniform_distribution(nlower,nupper);

    return uniform_distribution(nupper,nlower);
}


uniform_distribution uniform_distribution::operator+(const uniform_distribution& o) const
{
    double nlower = lower + o.lower;
    double nupper = upper + o.upper;
    if (nlower < nupper)
        return uniform_distribution(nlower,nupper);

    return uniform_distribution(nupper,nlower);
}

uniform_distribution& uniform_distribution::operator+=(const uniform_distribution& o)
{
    double nlower = lower + o.lower;
    double nupper = upper + o.upper;
    if (nlower < nupper)
    {
        lower = nlower;
        upper = nupper;
    } else {
        lower = nupper;
        upper = nlower;
    }

    return *this;
}

uniform_distribution uniform_distribution::operator-=(const uniform_distribution& o)
{
    double nlower = lower - o.upper;
    double nupper = upper - o.lower;
    if (nlower < nupper)
    {
        lower = nlower;
        upper = nupper;
    } else {
        lower = nupper;
        upper = nlower;
    }

    return *this;
}

uniform_distribution uniform_distribution::operator-(const uniform_distribution& o) const
{
    double nlower = lower - o.upper;
    double nupper = upper - o.lower;
    if (nlower < nupper)
        return uniform_distribution(nlower,nupper);

    return uniform_distribution(nupper,nlower);
}

uniform_distribution uniform_distribution::operator*(const double d) const
{
    double nlower = lower * d;
    double nupper = upper * d;
    if (nlower < nupper)
        return uniform_distribution(nlower,nupper);

    return uniform_distribution(nupper,nlower);
}

uniform_distribution uniform_distribution::operator/(const double d) const
{
    double nlower = lower / d;
    double nupper = upper / d;
    if (nlower < nupper)
        return uniform_distribution(nlower,nupper);

    return uniform_distribution(nupper,nlower);
}

string uniform_distribution::to_string() const
{
    return "[" + std::to_string(lower) + "," + std::to_string(upper) +"]";
}

}

std::ostream& operator<<(std::ostream& os, const silicontrip::uniform_distribution& l)
{
    os << l.to_string();
    return os;
}