
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
			// The older S2 library used this definition
			static constexpr double earth_radius = 6367.0;
			static constexpr double earth_radius_2 = 40538689.0;
			point(S2Point p);
			point(long la, long ln);
			point(std::string ld);
			point(std::string la, std::string ln);
			point(double la, double ln);
			point(const point& p);
			point(S2LatLng ll);
			point();
			//~point();
			
			std::string to_string() const;
			S2LatLng s2latlng() const;
			void set_s2latlng(S2LatLng ll);

			point inverse() const;
			double geo_distance_to(const point& p) const;
			//S1Angle getAngle(point p1, point p2);
			//S1Angle getBearing(point p);

			int count_links(std::vector<line> l) const;
			int count_dlinks(std::vector<line> l) const;
			int count_olinks(std::vector<line> l) const;

			static point Invalid();
			bool is_valid() const;
			bool operator==(const point& p) const;
	};

}

std::ostream& operator<<(std::ostream& os, const silicontrip::point& p);

template<> struct std::hash<silicontrip::point> {
    std::size_t operator()(silicontrip::point const& s) const noexcept {
        std::size_t h1 = std::hash<double>{}(s.s2latlng().lat().radians());
        std::size_t h2 = std::hash<double>{}(s.s2latlng().lng().radians());
        return h1 ^ (h2 << 1); 
    }
};

#endif
