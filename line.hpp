#ifndef SILICONTRIP_LINE_HPP
#define SILICONTRIP_LINE_HPP

#include <string>

#include "point.hpp"

#include <vector>
#include <s2/s2latlng.h>
#include <s2/s2edge_crosser.h>
#include <s2/s2edge_crossings.h>
#include <s2/s2edge_distances.h>
#include <math.h>

namespace silicontrip {

	class line {
		protected:
			point o_point;
			point d_point;

			S2Point o_s2Point;
			S2Point d_s2Point;
			//S2EdgeCrosser crosser;
			double eps = 1e-8;

			int point_on(S2Point D, line l) const;

		public:
			S2Point normal() const;
			long d_lat_e6() const;
			long d_lng_e6() const;
			long o_lat_e6() const;
			long o_lng_e6() const;

			double d_lat_double() const;
			double d_lng_double() const;
			double o_lat_double() const;
			double o_lng_double() const;

			S2Point d_s2point() const;
			S2Point o_s2point() const;

			point get_o_point() const;
			point get_d_point() const;

			S2LatLng d_s2latlng() const;
			S2LatLng o_s2latlng() const;

			line(const line& l);
			line(point d, point o);
			line(long dla, long dlo, long ola, long olo);
			line();

			bool operator==(const line& l) const;
			bool found(std::vector<line> l) const;
			bool intersects(const line& l) const;	
			bool intersects(const std::vector<line>& l) const;	
			bool intersect_or_equal(line l) const;
			bool intersect_or_equal(std::vector<line> l) const;

			double ang_distance() const;
			double geo_distance() const;
			double geo_distance(const point& p) const;

			std::string to_string() const;

			bool has_point(const point& p) const;
			int great_circle_intersection_type(line l) const;


	};

}

std::ostream& operator<<(std::ostream& os, const silicontrip::line& l);

template<> struct std::hash<silicontrip::line> {
    std::size_t operator()(silicontrip::line const& s) const noexcept {
        std::size_t h1 = std::hash<silicontrip::point>{}(s.get_o_point());
        std::size_t h2 = std::hash<silicontrip::point>{}(s.get_d_point());
        return h1 ^ h2;  // the equals operator isn't concerned with direction, neither should we.
    }
};


#endif

/*
    1 // A common hash combine function (often found in boost::hash_combine)
    2 inline void hash_combine(std::size_t& seed, std::size_t hash_value) {
    3     seed ^= hash_value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    4 }
    5 
    6 template<> struct std::hash<silicontrip::line> {
    7     std::size_t operator()(silicontrip::line const& s) const noexcept {
    8         std::size_t h1 = std::hash<silicontrip::point>{}(s.get_o_point());
    9         std::size_t h2 = std::hash<silicontrip::point>{}(s.get_d_point());
   10 
   11         // Ensure order-independent hashing for lines (A,B) and (B,A)
   12         // by sorting the hashes before combining them.
   13         std::array<std::size_t, 2> hashes = {h1, h2};
   14         std::sort(hashes.begin(), hashes.end()); // Sort to make (A,B) and (B,A) hash the same
   15 
   16         std::size_t seed = 0;
   17         hash_combine(seed, hashes[0]);
   18         hash_combine(seed, hashes[1]);
   19         return seed;
   20     }
   21 };

*/
