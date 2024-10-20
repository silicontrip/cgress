#ifndef SILICONTRIP_FIELD_HPP
#define SILICONTRIP_FIELD_HPP

#include <vector>
#include <unordered_map>

#include <s2/s2loop.h>
#include <s2/s2cell_union.h>
#include <s2/s2region_coverer.h>
#include <s2/s2polygon.h>

#include "point.hpp"
#include "team_count.hpp"
#include "link.hpp"
#include "line.hpp"

namespace silicontrip {
    class field {
        private:
            S2Loop *field_loop;
            S2Polygon field_poly;
            S2Polygon *field_poly_ptr;
            point field_points[3];
            line field_lines[3];

            int calculate_mu();
            double sign(point p1, point p2, point p3) const;
            double sign (double p1a, double p1o, double p2a, double p2o, double p3a, double p3o) const;
            S2CellUnion cells() const;

        public:

            field& operator= (const field& f);

            field();
            field(point p1, point p2, point p3);
            field(const field& f);
            ~field();

            int get_est_mu();

            //S2Loop* s2loop() const;
            S2Polygon* s2polygon() const;
            //S2CellId doesn't have a working hash function
            std::unordered_map<uint64,double>* cell_intersection() const;
            point point_at(int i) const;
            bool has_point(point p) const;
            int point_index(point p) const;
            std::vector<point>* get_points() const;
            long lat_at(int i) const;
            long lng_at(int i) const;
            double geo_area() const;
            double geo_perimeter() const;
            std::vector<line>* get_lines() const;
            line line_at(int i) const;

            bool touches(field f) const;
            bool intersects(field f) const;
            bool intersects(std::vector<field>* f) const;
            bool shares_line(field f) const;
            line shared_line(field f) const;
            bool has_line(line l) const;
            bool intersects(line l) const;
            bool intersects(std::vector<line>* l) const;
            team_count count_intersections(std::vector<link>* l) const;
            std::vector<link>* get_intersections(std::vector<link>* l) const;
            bool inside(point p) const;
            bool inside(std::vector<point>* p) const;
            bool inside(field f) const;
            bool layers(field f) const;
            bool layers(std::vector<field>* f) const;
            point other_point(line l) const;
            field inverse_corner_field(int corner) const;
            double difference(const field f) const;
            bool operator==(field f) const;
            bool found_in(std::vector<field>* f) const;
            std::string drawtool() const;
            std::string to_string() const;

    };
}

std::ostream& operator<<(std::ostream& os, const silicontrip::field& l);


#endif

