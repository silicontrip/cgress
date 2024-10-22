#include "uniform_distribution.hpp"

namespace silicontrip {
uniform_distribution::uniform_distribution() { lower = 0; upper =0; }
uniform_distribution::uniform_distribution(double a, double b) { lower = a; upper = b; }
uniform_distribution::uniform_distribution(const uniform_distribution& u) { lower = u.lower; upper = u.upper; }

double uniform_distribution::mean() const { return ( lower + upper ) / 2.0; }
}
