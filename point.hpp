
#ifndef SILICONTRIP_POINT_HPP
#define SILICONTRIP_POINT_HPP

#include <string>
#include <vector>

#include <s2/s2latlng.h>
#include <s2/s2point.h>
#include <s2/s1angle.h>
#include <s2/s2earth.h>

namespace silicontrip {
	class line;
	class point {
		protected:
			S2LatLng latlng;
		public:

			point(S2Point p);
			point(long la, long ln);
			point(std::string ld);
			point(std::string la, std::string ln);
			point(double la, double ln);
			point(point& p);
			//~point();
			
			std::string to_string() const;
			S2LatLng s2latlng() const;
			point inverse() const;
			double geo_distance_to(point& p) const;
			//S1Angle getAngle(point p1, point p2);
			//S1Angle getBearing(point p);

			int count_links(std::vector<line> l);
			int count_dlinks(std::vector<line> l);
			int count_olinks(std::vector<line> l);

			bool operator==(const point& p) const;
	};

}

std::ostream& operator<<(std::ostream& os, const silicontrip::point& p);

#endif
