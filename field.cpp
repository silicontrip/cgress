#include "field.hpp"

using namespace std;

namespace silicontrip{

field::field() { ; }
field::field(point p1, point p2, point p3)
{
    if (sign(p1,p2,p3) > 0) { 
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


    vector<S2Point> loop_points;
    loop_points.push_back(p1.s2latlng().ToPoint());
    loop_points.push_back(p2.s2latlng().ToPoint());
    loop_points.push_back(p3.s2latlng().ToPoint());

    field_loop = new S2Loop(loop_points);
    field_loop->Normalize();

    field_poly = S2Polygon(unique_ptr<S2Loop>(field_loop));
    field_poly_ptr = &field_poly; 
    
    field_loop = new S2Loop(loop_points);
    field_loop->Normalize();

}
field::field(const field& f)
{
    point p1 = f.point_at(0);
    point p2 = f.point_at(1);
    point p3 = f.point_at(2);

    if (sign(p1,p2,p3) > 0) { 
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

    vector<S2Point> loop_points;
    loop_points.push_back(p1.s2latlng().ToPoint());
    loop_points.push_back(p2.s2latlng().ToPoint());
    loop_points.push_back(p3.s2latlng().ToPoint());

    field_loop = new S2Loop(loop_points);
    field_loop->Normalize();

    field_poly = S2Polygon(unique_ptr<S2Loop>(field_loop));
    field_poly_ptr = &field_poly; 
    
    field_loop = new S2Loop(loop_points);
    field_loop->Normalize();

}
field::~field() { ; }

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


//S2Loop* field::s2loop() const { return field_loop; }
S2Polygon* field::s2polygon() const { return field_poly_ptr; }

S2CellUnion field::cells() const
{
    S2RegionCoverer::Options opt;
    opt.set_max_level(13);
    opt.set_min_level(0);
    opt.set_max_cells(20);
    S2RegionCoverer rc(opt);

    return rc.GetCovering(field_poly);
}

std::unordered_map<uint64,double>* field::cell_intersection() const
{
    unordered_map<uint64,double>* area = new unordered_map<uint64,double>();
    S2CellUnion cell_union = cells();

    for (S2CellId cellid: cell_union)
    {
        S2Cell cell = S2Cell(cellid);
        vector<S2Point> cell_points;
        cell_points.push_back(cell.GetVertex(0));
        cell_points.push_back(cell.GetVertex(1));
        cell_points.push_back(cell.GetVertex(2));
        cell_points.push_back(cell.GetVertex(3));
        S2Loop cell_loop = S2Loop(cell_points);
        cell_loop.Normalize();
        S2Polygon cell_poly = S2Polygon(unique_ptr<S2Loop>(&cell_loop));


        S2Polygon int_poly;
        int_poly.InitToIntersection(field_poly, cell_poly);

        //polyArea.put(cellid,new Double(intPoly.getArea() *  earthRadius * earthRadius));
        double sqkm = S2Earth::SteradiansToSquareKm(int_poly.GetArea());
        uint64 id = cellid.id();
        pair<uint64,double> area_pair (id,sqkm);
        area->insert(area_pair);

    }
    return area;
}

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
std::vector<point>* field::get_points() const
{
    std::vector<point>* ret = new std::vector<point>();
    ret->push_back(field_points[0]);
    ret->push_back(field_points[1]);
    ret->push_back(field_points[2]);

    return ret;
}
long field::lat_at(int i) const { return field_points[i].s2latlng().lat().e6(); }
long field::lng_at(int i) const { return field_points[i].s2latlng().lng().e6(); }
double field::geo_area() const { return S2Earth::SteradiansToSquareKm(field_poly.GetArea()); }
double field::geo_perimeter() const { return S2Earth::ToKm(S2::GetPerimeter(field_loop->vertices_span())); }
std::vector<line>* field::get_lines() const 
{
    std::vector<line>* res = new std::vector<line>(field_lines,field_lines+3);
/*
    res->push_back(line(field_points[0],field_points[1]));
    res->push_back(line(field_points[1],field_points[2]));
    res->push_back(line(field_points[2],field_points[0]));
*/
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

bool field::touches(field f) const
{
    return (f.point_at(0) == point_at(0)  ||
        f.point_at(0) == point_at(1)  ||
        f.point_at(0) == point_at(2)  ||
        f.point_at(1) == point_at(1)  ||
        f.point_at(1) == point_at(2)  ||
        f.point_at(2) == point_at(2)) ;
}

bool field::intersects(field f) const
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

bool field::intersects(std::vector<field>* f) const
{
    for (field fi: *f)
        if (intersects(fi))
            return true;
    return false;
}

bool field::shares_line(field f) const
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

line field::shared_line(field f) const
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

bool field::intersects(std::vector<line>* l) const
{
    for (line li: *l)
        if (intersects(l))
            return true;
    return false;
}

std::vector<link>* field::get_intersections(std::vector<link>* l) const
{
    vector<link>* all = new vector<link>();

    line l1 = line_at(0);
    line l2 = line_at(1);
    line l3 = line_at(2);

    for (link li: *l)
    {
        // Line ll = li.getLine();

        if (l1.intersects(li)) {
            all->push_back(li); 
        } else if (l2.intersects(li)) {
            all->push_back(li); 
        } else if (l3.intersects(li)) {
            all->push_back(li); 
        }
    }
    return all;        
}


team_count field::count_intersections(std::vector<link>* l) const
{
    vector<link>* blocks = get_intersections(l);
    team_count block;

    for (link li: *blocks)
        block.inc_team_enum(li.get_team_enum());

    return block;

}

bool field::inside(point p) const { 
    return field_poly.GetDistance(p.s2latlng().ToPoint()).radians() == 0; }

bool field::inside(std::vector<point> p) const
{
    for (point po: p)
        if (!inside(po))
            return false;
    return true;
}

bool field::inside(field f) const
{
    if (inside(f.point_at(0)) && inside(f.point_at(1)) && inside(f.point_at(2))) 
        return true;
    return false;
}

bool field::layers(field f) const { return inside(f) || f.inside(*this); }

bool field::layers(std::vector<field>* f) const
{
    if (f->size() == 0 )
        return true;
    // this logic seems incorrect.
    // this is saying that layers is true if any one of the fields layer this field
    // rather than all fields must layer this field
    for (field fi: *f)
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

double field::difference(const field fi) const
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

bool field::found_in(std::vector<field>* f) const
{
    for (field fi: *f)
        if ( *this == fi)
            return true;
    return false;

}


bool field::operator==(field f) const
{
    // assumes same rotation (which is handled by constructor)
    return
    (point_at(0) == f.point_at(0) && point_at(1) == f.point_at(1) && point_at(2) == f.point_at(2)) ||
    (point_at(0) == f.point_at(1) && point_at(1) == f.point_at(2) && point_at(2) == f.point_at(0)) ||
    (point_at(0) == f.point_at(2) && point_at(1) == f.point_at(0) && point_at(2) == f.point_at(1)) ;

}

std::string field::drawtool()
{
    return "{\"type\":\"polygon\",\"latLngs\":[{\"lat\":" + std::to_string(point_at(0).s2latlng().lat().degrees()) + 
                ",\"lng\":" + std::to_string(point_at(0).s2latlng().lng().degrees()) +
                "},{\"lat\":" + std::to_string(point_at(1).s2latlng().lat().degrees()) +
                ",\"lng\":" + std::to_string(point_at(1).s2latlng().lng().degrees()) +
                "},{\"lat\":" + std::to_string(point_at(2).s2latlng().lat().degrees()) +
                ",\"lng\":" + std::to_string(point_at(2).s2latlng().lng().degrees()) +
                "}],\"color\":\"#a05000\"}";
}



}
