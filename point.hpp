
#ifndef SILICONTRIP_POINT_HPP
#define SILICONTRIP_POINT_HPP

#include <string>

#include <s2/s2latlng.h>
#include <s2/s2point.h>
#include <s2/s1angle.h>

namespace silicontrip {
class point {
	private:
		S2LatLng latlng;
	
	public:

		const double earth_radius = 6367.0;

		long lat_e6() const;
		long lng_e6() const;
		double lat_double() const;
		double lng_double() const;
		double lat() const;
		double lng() const;
		S2LatLng s2latlng() const;
		double lat_radians() const;
		double lng_radians() const;
		S2Point s2point() const;


		point(S2Point p);
		point(long la, long ln);
		point(std::string ld);
		point(std::string la, std::string ln);
		point(double la, double ln);
		point(point& p);
		~point();
		
		std::string toString() const;
		
		point inverse() const;
		S1Angle distance_to(point& p) const;
		double geo_distance_to(point& p) const;
		//S1Angle getAngle(point p1, point p2);
		//S1Angle getBearing(point p);
		
		bool operator==(const silicontrip::point& o) const;
		//std::ostream& operator<<(std::ostream& os, const point& p);
};

}

std::ostream& operator<<(std::ostream& os, const silicontrip::point& p);

#endif
