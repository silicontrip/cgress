#ifndef SILICONTRIP_UNIFORM_DISTRIBUTION_HPP
#define SILICONTRIP_UNIFORM_DISTRIBUTION_HPP

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

};

}

#endif
