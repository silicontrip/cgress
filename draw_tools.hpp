#ifndef SILICONTRIP_DRAWTOOLS_HPP
#define SILICONTRIP_DRAWTOOLS_HPP

#include <string>
#include <vector>
#include <unordered_set>

#include <json/json.h>

#include "point.hpp"
#include "line.hpp"
#include "field.hpp"

namespace silicontrip {

class draw_tools {
    private:
        Json::Value entities;
        std::string colour;
        int output_type;

        std::vector<point>* get_points(Json::Value poly) const;
        Json::Value make_polygon (Json::Value ll1, Json::Value ll2, Json::Value ll3) const;
        Json::Value make_polyline (Json::Value ll1, Json::Value ll2) const;

    public:

        draw_tools();
        draw_tools(std::string description);

        std::vector<point>* get_unique_points() const;
        void erase();
        void set_output_as_polyline();
        void set_output_as_polygon();
        void set_output_as_intel();
        void set_output_as_is();

        void set_colour(std::string s);
        void add(line l);
        void add(field f);
        void add(point p);
        void add(point p, double r);

        //Json::Value as_polygon() const;
        //Json::Value as_polyline() const;
        //std::string as_intel() const;

        std::string to_string() const;
        int size() const;

};

}
std::ostream& operator<<(std::ostream& os, const silicontrip::draw_tools& l);

#endif
