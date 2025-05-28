#ifndef SILICONTRIP_UNIFORM_DISTRIBUTION_HPP
#define SILICONTRIP_UNIFORM_DISTRIBUTION_HPP

#include <cfloat>
#include <cmath>
#include <iostream>
#include <string>

namespace silicontrip
{
class uniform_distribution {
	private:
		double upper;
		double lower;

	public:
		uniform_distribution();
		uniform_distribution(double a, double b);
		uniform_distribution(const uniform_distribution& u);

		double mean() const;
		double range() const;
		double rounded_range() const;
		double perror() const;
		uniform_distribution inverse() const; 

		uniform_distribution operator+(const uniform_distribution& o) const;
		uniform_distribution& operator+=(const uniform_distribution& o);
		uniform_distribution operator-(const uniform_distribution& o) const;

		uniform_distribution operator*(const double d) const;

		std::string to_string() const;

};

}

std::ostream& operator<<(std::ostream& os, const silicontrip::uniform_distribution& l);

#endif
