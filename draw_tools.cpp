#include "draw_tools.hpp"

using namespace std;

namespace silicontrip {

draw_tools::draw_tools()
{
    colour="#a24ac3";
    istringstream init;
    init.str("[]");
    output_type = 0;
    init >> entities;
}

draw_tools::draw_tools(string description)
{
    colour="#a24ac3";
    istringstream init;
    init.str(description);
    output_type = 0;
    init >> entities;
}

/*
[{"type":"polygon","latLngs":[{"lat":-37.813154,"lng":145.348738},{"lat":-37.80919,"lng":145.343338},{"lat":-37.813431,"lng":145.341783}],"color":"#a05000"},
{"type":"polyline","latLngs":[{"lat":-37.809694,"lng":145.342332},{"lat":-37.813431,"lng":145.341783}],"color":"#a24ac3"},
{"type":"circle","latLng":{"lat":-37.809267,"lng":145.346449},"radius":169.36755847372498,"color":"#a24ac3"},
{"type":"marker","latLng":{"lat":-37.811968,"lng":145.343227},"color":"#a24ac3"}]
*/
vector<point> draw_tools::get_points(Json::Value poly) const
{
    string type = poly["type"].asString();
    vector<point> v;

    if (type == "circle" || type == "marker")
    {
        Json::Value latlng = poly["latLng"];
        double lat = latlng["lat"].asDouble();
        double lng = latlng["lng"].asDouble();

        point p =point(lat,lng);
        v.push_back(p);
    }
    if (type == "polygon" || type == "polyline")
    {
        Json::Value latlngs = poly["latLngs"];
        for (Json::Value latlng: latlngs)
        {
            double lat = latlng["lat"].asDouble();
            double lng = latlng["lng"].asDouble();

            point p =point(lat,lng);
            v.push_back(p);
        }
    }

    return v;
}


vector<point> draw_tools::get_unique_points() const
{
    unordered_set<point> us;
    for (Json::Value jv: entities)
    {
        vector<point> pts = get_points(jv);
        for (point p: pts)
        {
            us.insert(p);
        }
    }
    vector<point> res;
    res.assign(us.begin(),us.end());
    
    return res;
}

void draw_tools::erase()
{
    istringstream init;
    init.str("[]");
    init >> entities;
}

void draw_tools::set_output_as_polyline() { output_type = 1; }
void draw_tools::set_output_as_polygon() { output_type = 2; }
void draw_tools::set_output_as_intel() { output_type = 3; }
void draw_tools::set_output_as_is() { output_type =0; }

void draw_tools::set_colour(string s) 
{
    // should validate this. 
    if (regex_match(s,regex("^#[0-9a-fA-F]{6}$")))
        colour = s; 
}
string draw_tools::get_colour() const { return colour; }

void draw_tools::add(line l)
{
    Json::Value latLngs;
    Json::Value ll;

    ll["lat"] = l.o_s2latlng().lat().e6() / 1000000.0;
    ll["lng"] = l.o_s2latlng().lng().e6() / 1000000.0;

    latLngs.append(ll);

    ll["lat"] = l.d_s2latlng().lat().e6() / 1000000.0;
    ll["lng"] = l.d_s2latlng().lng().e6() / 1000000.0;

    latLngs.append(ll);

    Json::Value obj;
    obj["type"] = "polyline";
    obj["color"] = colour;
    obj["latLngs"] = latLngs;

    entities.append(obj);

}

void draw_tools::add(field f)
{
    Json::Value latLngs;
    Json::Value ll;

    for (int i=0; i<3; i++)
    {
        ll["lat"] = f.point_at(i).s2latlng().lat().e6() / 1000000.0;
        ll["lng"] = f.point_at(i).s2latlng().lng().e6() / 1000000.0;

        latLngs.append(ll);
    }
    Json::Value obj;
    obj["type"] = "polygon";
    obj["color"] = colour;
    obj["latLngs"] = latLngs;

    entities.append(obj);
}

void draw_tools::add(point p)
{
    Json::Value ll;
    ll["lat"] = p.s2latlng().lat().e6() / 1000000.0;
    ll["lng"] = p.s2latlng().lng().e6() / 1000000.0;
        
    Json::Value obj;

    obj["type"] = "marker";
    obj["color"] = colour;
    obj["latLng"] = ll;

    entities.append(obj);
}

void draw_tools::add(point p,double r)
{
    Json::Value ll;
    ll["lat"] = p.s2latlng().lat().e6() / 1000000.0;
    ll["lng"] = p.s2latlng().lng().e6() / 1000000.0;
        
    Json::Value obj;

    obj["type"] = "circle";
    obj["color"] = colour;
    obj["latLng"] = ll;
    obj["radius"] = r;

    entities.append(obj);
}

int draw_tools::size() const { return entities.size(); }

Json::Value draw_tools::make_polygon (Json::Value ll1, Json::Value ll2, Json::Value ll3) const
{
    Json::Value latLngs;
    latLngs.append(ll1);
    latLngs.append(ll2);
    latLngs.append(ll3);
    
    Json::Value obj;
    obj["type"] = "polygon";
    obj["color"] = colour;
    obj["latLngs"] = latLngs;

    return obj;
}

Json::Value draw_tools::make_polyline (Json::Value ll1, Json::Value ll2) const
{
    Json::Value latLngs;
    // order so that lines are unique
    if (ll1["lat"] < ll2["lat"])
    {
        latLngs.append(ll1);
        latLngs.append(ll2);
    } else if ( ll2["lat"] < ll1["lat"]) {
        latLngs.append(ll2);
        latLngs.append(ll1);
    } else if ( ll1["lng"] < ll2["lng"]) {
        latLngs.append(ll1);
        latLngs.append(ll2);
    } else {
        latLngs.append(ll2);
        latLngs.append(ll1);
    }

    Json::Value obj;
    obj["type"] = "polyline";
    obj["color"] = colour;
    obj["latLngs"] = latLngs;

    return obj;
}

string draw_tools::to_string() const
{

    if (output_type == 3)
        return as_intel();

    Json::Value out = entities;
    if (output_type == 1 )
        out = as_polyline();
    if (output_type == 2)
        out = as_polygon();
        
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    builder.settings_["precision"] = 9;
    return Json::writeString(builder, out);
}


Json::Value draw_tools::as_polygon() const
{
    Json::Value out;
    unordered_set<Json::Value> temp;

    for (Json::Value dto1: entities) 
    {
        if (dto1["type"] == "polyline")
        {
            bool insert = false;
            for (Json::Value dto2: entities)
            {
                if (dto2["type"] == "polyline")
                {
                    if (dto1["latLngs"][0] == dto2["latLngs"][0])
                    {
                        for (Json::Value dto3: entities)
                        {
                            if (dto3["type"] == "polyline")
                            {
                                if (((dto1["latLngs"][1] == dto3["latLngs"][0]) && 
                                (dto2["latLngs"][1] == dto3["latLngs"][1])) || 
                                ((dto1["latLngs"][1] == dto3["latLngs"][1]) && 
                                (dto2["latLngs"][1] == dto3["latLngs"][0]) )) 
                                {
                                    insert = true;
                                    temp.insert(make_polygon(dto1["latLngs"][0],dto1["latLngs"][1],dto2["latLngs"][0]));
                                }
                            }
                        }
                    } else if (dto1["latLngs"][0] == dto2["latLngs"][1]) {
                        for (Json::Value dto3: entities)
                        {
                            if (dto3["type"] == "polyline")
                            {
                                if (((dto1["latLngs"][1] == dto3["latLngs"][0]) && 
                                (dto2["latLngs"][0] == dto3["latLngs"][1])) || 
                                ((dto1["latLngs"][1] == dto3["latLngs"][1]) && 
                                (dto2["latLngs"][0] == dto3["latLngs"][0]) )) 
                                {
                                    insert = true;
                                    temp.insert(make_polygon(dto1["latLngs"][0],dto1["latLngs"][1],dto2["latLngs"][0]));
                                }
                            }
                        }   
                    }
                }
            }
            if (!insert)
                out.append(dto1);
        } else {
            out.append(dto1);
        }
    }
    for (Json::Value jv: temp)
        out.append(jv);

    return out;
}

Json::Value draw_tools::as_polyline() const
{
    Json::Value out;
    unordered_set<Json::Value> lines;

// this is not a true unique, as lines are undirected.
// where as this treats opposite direction lines as different
// hopefully changes to make_polyline should fix reversed lines
    for (Json::Value po: entities)
    {
        if (po["type"] == "polygon")
        {
            lines.insert(make_polyline(po["latLngs"][0],po["latLngs"][1]));
            lines.insert(make_polyline(po["latLngs"][1],po["latLngs"][2]));
            lines.insert(make_polyline(po["latLngs"][2],po["latLngs"][0]));
        } else if (po["type"] == "polyline") {
            lines.insert(make_polyline(po["latLngs"][0],po["latLngs"][1]));
        } else {
            out.append(po);
        }
    }
    for (Json::Value jv: lines)
        out.append(jv);
    return out;
}

std::string draw_tools::as_intel() const
{
    Json::Value pos = as_polyline();

    stringstream ss;
    bool exceeds_maxlinks = false;
    int link_count = 0;
    bool first = true;
    double maxLength = 0;
    double centreLat =0;
    double centreLng =0;
    double pointCount = 0;

    ss.precision(9);
    ss << "https://www.ingress.com/intel?pls="; 

    for (Json::Value po: pos)
    {
        if (po["type"] == "polyline")
        {
            link_count++;
            // Intel claims 40 links but I think it is less than this.
            // my tests seem to indicate the limit is 25.
            if (link_count <= 25) {
                centreLat += po["latLngs"][0]["lat"].asDouble();
                centreLng += po["latLngs"][0]["lng"].asDouble();
                centreLat += po["latLngs"][1]["lat"].asDouble();
                centreLng += po["latLngs"][1]["lng"].asDouble();
                pointCount += 2;

                if (!first) {
                    ss << "_";
                }

                ss << po["latLngs"][0]["lat"].asDouble(); 
                ss << ","; 
                ss << po["latLngs"][0]["lng"].asDouble();
                ss << ",";
                ss << po["latLngs"][1]["lat"].asDouble(); 
                ss << ","; 
                ss << po["latLngs"][1]["lng"].asDouble();

                first = false;
            } else 
                exceeds_maxlinks = true;
        }
    }
    //check for maximum links.
    centreLat /= pointCount;
    centreLng /= pointCount;

    centreLat = round(centreLat*1000000)/1000000.0;
    centreLng = round(centreLng*1000000)/1000000.0;

    ss << "&ll=";
    ss << centreLat;
    ss << ",";
    ss << centreLng;
    // should be a define somewhere.
    if (exceeds_maxlinks)
        ss << endl << "Warning: " << link_count <<" links exceeded maximum. Only showing 25.";

    return ss.str();

}

void draw_tools::convert_to_polyline()
{
    entities = as_polyline();
}

void draw_tools::convert_to_polygon()
{
    entities = as_polygon();
}

void draw_tools::convert()
{
    if (output_type == 3 || output_type == 1)
        convert_to_polyline();

    if (output_type == 2)
        convert_to_polygon();

    // do nothing otherwise
}

vector<line> draw_tools::get_lines() const
{
    unordered_set<line> line_set;
    Json::Value pos = as_polyline();
    for (Json::Value dto1: pos) 
    {
        if (dto1["type"] == "polyline")
        {
            Json::Value ll = dto1["latLngs"];
            double lato = ll[0]["lat"].asDouble();
            double lngo = ll[0]["lng"].asDouble();
            double latd = ll[1]["lat"].asDouble();
            double lngd = ll[1]["lng"].asDouble();
            
            point po = point(lato,lngo);
            point pd = point(latd,lngd);
            line l = line (po,pd);
            line_set.insert(l);
        }
    }
    vector<line> result;
    result.assign(line_set.begin(),line_set.end());
    return result;
}

vector<point> draw_tools::get_points() const
{
    unordered_set<point> point_set;

    for (Json::Value dto1: entities)
    {
        if (dto1["type"] == "polyline" || dto1["type"] == "polygon")
        {
            Json::Value lls = dto1["latLngs"];
            for (Json::Value ll : lls)
            {
                double lat = ll["lat"].asDouble();
                double lng = ll["lng"].asDouble();
                point p = point(lat,lng);

                point_set.insert(p);
            }
        }
    }
    vector<point> result;
    result.assign(point_set.begin(),point_set.end());
    return result;
}

}

std::ostream& operator<<(std::ostream& os, const silicontrip::draw_tools& l)
{
    os << l.to_string();
    return os;
}