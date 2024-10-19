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

			S2Point normal() const;
			int point_on(S2Point D, line l) const;

		public:
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

			S2LatLng d_s2latlng() const;
			S2LatLng o_s2latlng() const;

			line(point d, point o);
			line(long dla, long dlo, long ola, long olo);
			line();

			bool operator==(const line& l) const;
			bool found(std::vector<line> l) const;
			bool intersects(const line& l) const;	
			bool intersects(std::vector<line> l) const;	
			bool intersect_or_equal(line l) const;
			bool intersect_or_equal(std::vector<line> l) const;

			double geo_distance() const;
			double geo_distance(const point& p) const;

			std::string to_string() const;

			bool has_point(const point& p) const;
			int great_circle_intersection_type(line l) const;


	};

}

std::ostream& operator<<(std::ostream& os, const silicontrip::line& l);


#endif
