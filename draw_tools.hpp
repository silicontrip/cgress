#ifndef SILICONTRIP_DRAWTOOLS_HPP
#define SILICONTRIP_DRAWTOOLS_HPP

#include <string>
#include <vector>
#include <unordered_set>
#include <regex>

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

        std::vector<point> get_points(Json::Value poly) const; // fixed return type
        Json::Value make_polygon (Json::Value ll1, Json::Value ll2, Json::Value ll3) const;
        Json::Value make_polyline (Json::Value ll1, Json::Value ll2) const;
        void from_intel(std::string intel);


    public:

        draw_tools();
        draw_tools(std::string description);

        void erase();
        void set_output_as_polyline();
        void set_output_as_polygon();
        void set_output_as_intel();
        void set_output_as_is();

        void set_colour(std::string s);
        std::string get_colour() const;
        void add(line l);
        void add(field f);
        void add(point p);
        void add(point p, double r);

        Json::Value as_polygon() const;
        Json::Value as_polyline() const;
        std::string as_intel() const;

        void convert_to_polyline();
        void convert_to_polygon();
        void convert();

        std::string to_string() const;
        int size() const;

        std::vector<line> get_lines() const;
        std::vector<point> get_points() const;
        std::vector<point> get_unique_points() const; //fixed return type

};

}
std::ostream& operator<<(std::ostream& os, const silicontrip::draw_tools& l);

template<> struct std::hash<Json::Value> {
    std::size_t operator()(Json::Value const& s) const noexcept {
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        std::string vv = Json::writeString(builder, s);
        std::size_t h1 = std::hash<std::string>{}(vv);
        return h1; 
    }
};

#endif
