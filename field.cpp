#include "field.hpp"

using namespace std;

namespace silicontrip{

field::field() { ; }
field::field(point p1, point p2, point p3)
{
    if (s2pred::Sign(p1.s2latlng().ToPoint(),p2.s2latlng().ToPoint(),p3.s2latlng().ToPoint()) > 0) { 
        field_points[0] = p1;
        field_points[1] = p2;
        field_points[2] = p3;
    } else {
        field_points[0] = p1;
        field_points[1] = p3;
        field_points[2] = p2;
    }
    
    field_lines[0] = line(field_points[0],field_points[1]);
    field_lines[1] = line(field_points[1],field_points[2]);
    field_lines[2] = line(field_points[2],field_points[0]);

    // field_loop = new S2Loop(loop_points);
    // field_loop->Normalize();

}

field::field(const field& f)
{
    // going to assume these are already normalized if they come from another field
    field_points[0] = f.point_at(0);
    field_points[1] = f.point_at(1);
    field_points[2] = f.point_at(2);
    
    field_lines[0] = f.field_lines[0];
    field_lines[1] = f.field_lines[1];
    field_lines[2] = f.field_lines[2];
    
    //field_poly_ = f.field_poly_;
    //field_poly_ptr_ = &field_poly_;
}

// field::~field() { ; }
/*
double field::sign(point p1, point p2, point p3) const 
{ 
    return sign (
        p1.s2latlng().lat().degrees(), p1.s2latlng().lng().degrees(), 
        p2.s2latlng().lat().degrees(), p2.s2latlng().lng().degrees(), 
        p3.s2latlng().lat().degrees(), p3.s2latlng().lng().degrees()
    );
}

double field::sign (double p1a, double p1o, double p2a, double p2o, double p3a, double p3o) const
{
    return (p1o - p3o) * (p2a - p3a) - (p2o - p3o) * (p1a - p3a);
}
*/
//S2Loop* field::s2loop() const { return field_loop; }
/*
S2Polygon* field::get_field_poly() 
{ 
    if (!field_poly_ptr_)
    {
        vector<S2Point> loop_points;
        loop_points.push_back(point_at(0).s2latlng().ToPoint());
        loop_points.push_back(point_at(1).s2latlng().ToPoint());
        loop_points.push_back(point_at(2).s2latlng().ToPoint());

        S2Loop* field_loop = new S2Loop(loop_points);
        field_loop->Normalize();

        field_poly_ = S2Polygon(unique_ptr<S2Loop>(field_loop));
        field_poly_ptr_ = & field_poly_;
    }
    return field_poly_ptr_;
}
*/

point field::point_at(int i) const { return field_points[i]; }
bool field::has_point(point p) const { return p == field_points[0] || p == field_points[1] || p == field_points[2]; }
int field::point_index(point p) const
{
    if (p == field_points[0])
        return 0;
    if (p == field_points[1])
        return 1;
    if (p == field_points[2])
        return 2;
    return -1;
}
std::vector<point> field::get_points() const
{
    std::vector<point> ret;
    ret.push_back(field_points[0]);
    ret.push_back(field_points[1]);
    ret.push_back(field_points[2]);

    return ret;
}
long field::lat_at(int i) const { return field_points[i].s2latlng().lat().e6(); }
long field::lng_at(int i) const { return field_points[i].s2latlng().lng().e6(); }
double field::geo_area() const { 
    //S2Polygon* s2p = s2polygon();
    //return S2Earth::SteradiansToSquareKm(field_poly.GetArea()); 

    double a = field_lines[0].ang_distance();
    double b = field_lines[1].ang_distance();
    double c = field_lines[2].ang_distance();
    
    double s = (a + b + c) / 2;
    //L'Huilier's Formula
    double t = 4 * atan(sqrt(tan(s/2) * tan((s-a)/2) * tan ((s-b)/2) * tan((s-c)/2)));

    // S2 Library changed its definition of Earth radius
    return t * point::earth_radius_2;

}
double field::geo_perimeter() const { 

    // S2Loops and S2Poly's are expensive want to avoid using them
    //return S2Earth::ToKm(S2::GetPerimeter(field_loop->vertices_span())); 

    return field_lines[0].geo_distance() + field_lines[1].geo_distance() + field_lines[2].geo_distance();

}
std::vector<line> field::get_lines() const 
{
    std::vector<line> res = std::vector<line>(field_lines,field_lines+3);
    return res;
}

line field::line_at(int i) const
{
    return field_lines[i];
    /*
    if (i == 0)
        return line(field_points[0],field_points[1]);
    if (i == 1)
        return line(field_points[1],field_points[2]);
    if (i == 2)
        return line(field_points[2],field_points[0]);

    return line(0,0,0,0);
    */
}

bool field::touches(const field& f) const
{
    return (f.point_at(0) == point_at(0)  ||
        f.point_at(0) == point_at(1)  ||
        f.point_at(0) == point_at(2)  ||
        f.point_at(1) == point_at(1)  ||
        f.point_at(1) == point_at(2)  ||
        f.point_at(2) == point_at(2)) ;
}

bool field::intersects(const field& f) const
{
    // this returns true if the areas intersect we only want to know if the edges intersect
    // return field_poly.Intersects(*f.s2polygon());
    // S2PointLoopSpan thisSpan = field_loop->vertices_span();

    for (line l1: field_lines) {
        for (line l2: f.field_lines)
        {
            if (l1.intersects(l2))
                return true;
        }
    }
    return false;
}

bool field::intersects(const std::vector<field>& f) const
{
    for (field fi: f)
        if (intersects(fi))
            return true;
    return false;
}

bool field::shares_line(const field& f) const
{
    return 
        line_at(0) == f.line_at(0) || 
        line_at(0) == f.line_at(1) ||
        line_at(0) == f.line_at(2) ||
        line_at(1) == f.line_at(0) ||
        line_at(1) == f.line_at(1) ||
        line_at(1) == f.line_at(2) ||
        line_at(2) == f.line_at(0) ||
        line_at(2) == f.line_at(1) ||
        line_at(2) == f.line_at(2) ;
}

line field::shared_line(const field& f) const
{
    if ( line_at(0) == f.line_at(0) || line_at(0) == f.line_at(1) || line_at(0) == f.line_at(2)) return line_at(0);
    if ( line_at(1) == f.line_at(0) || line_at(1) == f.line_at(1) || line_at(1) == f.line_at(2)) return line_at(1);
    if ( line_at(2) == f.line_at(0) || line_at(2) == f.line_at(1) || line_at(2) == f.line_at(2)) return line_at(2);

    return line(0,0,0,0);

}

bool field::has_line(line l) const { return l == line_at(0) || l == line_at(1) || l == line_at(2); }

bool field::intersects(line l) const
{
// wonder if there is an efficient S2 method for this
    return l.intersects(line_at(0)) || l.intersects(line_at(1)) || l.intersects(line_at(2));
}

bool field::intersects(const std::vector<line>& l) const
{
    for (line li: l)
        if (intersects(l))
            return true;
    return false;
}

std::vector<link> field::get_intersections(const std::vector<link>& l) const
{
    vector<link> all;

    line l1 = line_at(0);
    line l2 = line_at(1);
    line l3 = line_at(2);

    for (link li: l)
    {
        // Line ll = li.getLine();

        if (l1.intersects(li)) {
            all.push_back(li); 
        } else if (l2.intersects(li)) {
            all.push_back(li); 
        } else if (l3.intersects(li)) {
            all.push_back(li); 
        }
    }
    return all;        
}


team_count field::count_intersections(const std::vector<link>& l) const
{
    vector<link> blocks;
    team_count block;

    for (link li: blocks)
        block.inc_team_enum(li.get_team_enum());

    return block;
}

bool field::inside(point p) const { 
    //return get_field_poly()->GetDistance(p.s2latlng().ToPoint()).radians() == 0; 
    int a = s2pred::SignDotProd(field_lines[0].normal(), p.s2latlng().ToPoint());
    int b = s2pred::SignDotProd(field_lines[1].normal(), p.s2latlng().ToPoint());
    int c = s2pred::SignDotProd(field_lines[2].normal(), p.s2latlng().ToPoint());

    return a < 0 && b < 0 && c < 0;
}

bool field::inside(const std::vector<point>& p) const
{
    for (point po: p)
        if (!inside(po))
            return false;
    return true;
}

bool field::inside(const field& f) const
{
    if (inside(f.point_at(0)) && inside(f.point_at(1)) && inside(f.point_at(2))) 
        return true;
    return false;
}

bool field::layers(const field& f) const { return inside(f) || f.inside(*this); }

bool field::layers(const std::vector<field>& f) const
{
    if (f.size() == 0 )
        return true;
    // this logic seems incorrect.
    // this is saying that layers is true if any one of the fields layer this field
    // rather than all fields must layer this field
    for (field fi: f)
        if (layers(fi))
            return true;
    
    return false;
}

point field::other_point(line l) const
{
    if (!l.has_point(point_at(0))) return point_at(0);
    if (!l.has_point(point_at(1))) return point_at(1);
    if (!l.has_point(point_at(2))) return point_at(2);

    return point(0L,0L);

}

field field::inverse_corner_field(int corner) const
{
    int alt1=1, alt2=2;

    if (corner == 0) { alt1=1; alt2=2; }
    if (corner == 1) { alt1=0; alt2=2; }
    if (corner == 2) { alt1=0; alt2=1; }

    return field(point_at(corner), point_at(alt1).inverse(), point_at(alt2).inverse());

}

double field::difference(const field& fi) const
{
    double total = 0.0;
    double least;

    for (int f1 =0; f1 < 3; f1++)
    {
        least = 9999.9; // max link is 6881km
        for (int f2=0; f2<3; f2++)
        {
            double ff = point_at(f1).geo_distance_to(fi.point_at(f2));
            if (ff < least)
                least = ff;
        }
        if (least > total)
            //total += least;
            total = least;
    }       

    return total;

}

bool field::found_in(const std::vector<field>& f) const
{
    for (field fi: f)
        if ( *this == fi)
            return true;
    return false;

}

bool field::operator==(const field& f) const
{
    // assumes same rotation (which is handled by constructor)
    return
    (point_at(0) == f.point_at(0) && point_at(1) == f.point_at(1) && point_at(2) == f.point_at(2)) ||
    (point_at(0) == f.point_at(1) && point_at(1) == f.point_at(2) && point_at(2) == f.point_at(0)) ||
    (point_at(0) == f.point_at(2) && point_at(1) == f.point_at(0) && point_at(2) == f.point_at(1)) ;

}

std::string field::drawtool() const
{
    return "{\"type\":\"polygon\",\"latLngs\":[{\"lat\":" + std::to_string(point_at(0).s2latlng().lat().degrees()) + 
                ",\"lng\":" + std::to_string(point_at(0).s2latlng().lng().degrees()) +
                "},{\"lat\":" + std::to_string(point_at(1).s2latlng().lat().degrees()) +
                ",\"lng\":" + std::to_string(point_at(1).s2latlng().lng().degrees()) +
                "},{\"lat\":" + std::to_string(point_at(2).s2latlng().lat().degrees()) +
                ",\"lng\":" + std::to_string(point_at(2).s2latlng().lng().degrees()) +
                "}],\"color\":\"#a05000\"}";
}

string field::to_string() const
{
    return "" + field_points[0].to_string() + "-" + field_points[1].to_string() + "-" + field_points[2].to_string() + " " + std::to_string(geo_area()) +"km^2.";
}

field& field::operator= (const field& f)
{

    this->field_points[0] = f.field_points[0];
    this->field_points[1] = f.field_points[1];
    this->field_points[2] = f.field_points[2];

    field_lines[0] = f.field_lines[0];
    field_lines[1] = f.field_lines[1];
    field_lines[2] = f.field_lines[2];

    return *this;
}


}

std::ostream& operator<<(std::ostream& os, const silicontrip::field& l)
{
    os << l.to_string();
    return os;
}
