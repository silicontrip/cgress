#include "portal_factory.hpp"

using namespace std;

namespace silicontrip {

portal_factory* portal_factory::ptr = 0;

portal_factory::portal_factory() 
{
    ifstream file_properties("portal_factory_properties.json");
    Json::Value properties;

    file_properties >> properties;

    portal_api = properties["portalurl"].asString();

    //cout << portal_api << endl;
}
portal_factory* portal_factory::get_instance()
{
    if (!ptr)
    ptr = new portal_factory();
    return ptr;
}

vector<string> portal_factory::split_str(const string str, char del) const
{
    // https://www.geeksforgeeks.org/how-to-split-string-by-delimiter-in-cpp/
    // Create a stringstream object to str
    stringstream ss(str);
	
  	// Temporary object to store the splitted
  	// string
    string t;

    vector<string> v;
   	// Splitting the str string by delimiter
    while (getline(ss, t, del))
        v.push_back(t);

    return v;
}

S2Loop* portal_factory::s2loop_from_json(const string desc) const
{
    Json::Value dt;

    vector<S2Point>* loop_points = new vector<S2Point>();
    istringstream(desc) >> dt;
    for (Json::Value jv: dt)
    {
        for (Json::Value pp: jv["latLngs"])
        {
            double lat = pp["lat"].asDouble();
            double lng = pp["lng"].asDouble();

            S2LatLng ll = S2LatLng::FromDegrees(lat,lng);
            loop_points->push_back(ll.ToPoint());
        }
    }
    S2Loop* result = new S2Loop(*loop_points);

    delete loop_points;
    result->Normalize();  // in case the loop is CW (it's meant to be CCW)
    return result;
}

// get single returns a real portal.
// if desc is a lat/lng it must match the portal location exactly
// this may return multiples if title is not unique
vector<portal> portal_factory::get_single(const string desc) const
{
    Json::Value res;
    if (portal_api.substr(0,4) == "file")
    {
        res = json_reader::read_json_from_file(portal_api);
    } else {
        string url = portal_api+"?ll="+curlpp::escape(desc);
        cerr << "query: " << url << endl;
        res = json_reader::read_json_from_http(url);
    }
    // filter
    vector<portal>result;
    for (Json::Value jv: res)
    {
        // why is this here?
        // if this is needed it has to handle title, guid and location
        // we need to handle file reading which returns the entire json entries.
        //if (jv["title"] == desc)
        if (json_matches(jv,desc))   // hopefully this function is the fix.
        {
            portal p;

            p.set_guid(jv["guid"].asString());
            p.set_health(jv["health"].asInt());
            p.set_level(jv["level"].asInt());
            p.set_res_count(jv["rescount"].asInt());
            p.set_team(jv["team"].asString());
            p.set_title(jv["title"].asString());

            long la = jv["lat"].asInt();
            long ln = jv["lng"].asInt();

            S2LatLng ll = S2LatLng::FromE6(la,ln);

            p.set_s2latlng(ll);
            result.push_back(p);
        }
    }

    return result;
}

// it is legacy that this returns a map.  
// TODO: return vector<portal>
vector<portal> portal_factory::cluster_from_description(const string desc) const
{
    S2Region* search_region = nullptr;
    if (desc[0]=='.' && desc[1]=='/') { return cluster_from_file(desc); }
    if (desc[0]=='.' && desc[1]=='\\') { return cluster_from_file(desc); }  // if someone decides to port it.

    if (desc[0]=='[' && desc[1]=='{') 
    {
        search_region = s2loop_from_json(desc);
        return cluster_from_region(search_region);
    }
    if (desc[0]=='0' && desc[1]=='x') 
    {
        string token = desc.substr(2);
        S2CellId id = S2CellId::FromToken(token);
        S2Cell s2c = S2Cell(id);
        search_region = &s2c;
        return cluster_from_region(search_region);
    }
    
    // =  seems to be one of the only characters not used in portal titles
    vector<string> pt = split_str(desc,'=');

    int sz = pt.size();
    if (sz==1) {
        // single or radius
        vector<string> pd = split_str(desc,':');
        if (pd.size() == 1)
        {
            return get_single(desc); // this must be a portal

            //vector<portal> result;
		    //result.push_back(ploc);
            //pair<string,portal> gloc (ploc.get_guid(),ploc);
            //result.insert(gloc);

            //return result;

        } else if (pd.size() == 2) {
            // skip if we've been given a lat/lng string
            S2Point loc; // this is what we want
            if (is_point(pd.at(0)))
            {
                point ploc = point_from(pd.at(0));
                loc = ploc.s2latlng().ToPoint();
            } else {
                vector<portal> ploc = get_single(pd.at(0)); // this can be an arbitrary point
                if (ploc.size() > 1)
                {
                    stringstream ss;
                    ss << "Non unique portal title: " << pd.at(0);
                    for (portal po: ploc)
                        ss << " " << po.s2latlng();

                    throw domain_error(ss.str());
                }
                //cerr << "portal location: " << ploc[0] << endl;
                loc = ploc[0].s2latlng().ToPoint();
            }

            size_t end;
            double rangek = stod(pd.at(1),&end);

            //S1Angle range = S2Earth::KmToAngle(rangek);
            // The S2 Library changed it's value of earth radius
            S1Angle range = S1Angle::Radians(rangek / point::earth_radius);

            S2Cap s2c = S2Cap(loc,range);
            search_region = &s2c;

            return cluster_from_region(search_region);
        }
         
    } else if (sz==2) {
        // FIX: it's valid for these to be arbitrary points
        vector<point> ploc = get_array(pt);
        S2LatLngRect s2ll;

        s2ll = S2LatLngRect::FromPointPair(ploc.at(0).s2latlng(),ploc.at(1).s2latlng());
        search_region = &s2ll;

        return cluster_from_region(search_region);
    } else if (sz==3) {
        // FIX: it's valid for these to be arbitrary points
        vector<point> ploc = get_array(pt);

        //S2Builder::Options* opt = new S2Builder::Options();
        //S2Builder* builder = new S2Builder(opt);

        vector<S2Point> ppo = { ploc.at(0).s2latlng().ToPoint(), ploc.at(1).s2latlng().ToPoint(), ploc.at(2).s2latlng().ToPoint()};
        S2Loop* search_loop = new S2Loop(ppo);
        search_loop->Normalize();
        search_region = search_loop;

        return cluster_from_region(search_region);
        delete(search_loop);

    }
	vector<portal> map;
    return map;
}

vector<portal> portal_factory::cluster_from_file(const string desc) const
{
    vector<portal> res;
    vector<string> portal_list;
    ifstream portal_list_ss(desc);

    string t;
    while(getline(portal_list_ss, t))
    {
        portal_list.push_back(t);
    }

    res = cluster_from_array(portal_list);

    return res;

}

portal portal_factory::portal_from_json(Json::Value jv) const
{
    portal p;
    p.set_guid(jv["guid"].asString());
    p.set_health(jv["health"].asInt());
    p.set_level(jv["level"].asInt());
    p.set_res_count(jv["rescount"].asInt());
    p.set_team(jv["team"].asString());
    p.set_title(jv["title"].asString());

    long la = jv["lat"].asInt();
    long ln = jv["lng"].asInt();

    S2LatLng ll = S2LatLng::FromE6(la,ln);

    p.set_s2latlng(ll);

    return p;
}

/*
vector<portal> portal_factory::vector_from_map(const unordered_map<string,portal>& portals) const
{
    vector<portal> poli;
	for (pair<string,portal> it: portals) 
		poli.push_back(it.second);
	return poli;
}
*/

vector<portal> portal_factory::cluster_from_region(S2Region* reg) const
{
    Json::Value res;
    if (portal_api.substr(0,4) == "file")
    {
        res = json_reader::read_json_from_file(portal_api);
    } else {
       
        S2LatLngRect bound = reg->GetRectBound();

        string ll = bound.lo().ToStringInDegrees();
        string l2 = bound.hi().ToStringInDegrees();

        string url = portal_api+"?ll="+curlpp::escape(ll) + "&l2=" + l2;
        res = json_reader::read_json_from_http(url);
    }

    // filter
    vector<portal> results;

    for (Json::Value jv: res)
    {
        long la = jv["lat"].asInt();
        long ln = jv["lng"].asInt();

        S2LatLng ll = S2LatLng::FromE6(la,ln);
        S2Cell cell = S2Cell(ll);
        if (reg->Contains(cell))
        {
            portal p = portal_from_json(jv);
		    results.push_back(p);
            //pair<string,portal> gloc (p.get_guid(),p);
            //results->insert(gloc);
        } else if (dynamic_cast<S2Polygon*>(reg)) {
            S2Polygon* preg = dynamic_cast<S2Polygon*>(reg);
            S1Angle ang = preg->GetDistance(ll.ToPoint());
            if (ang.e6() == 0 )
            {
                portal p = portal_from_json(jv);
                results.push_back(p);
            }
        } else if (dynamic_cast<S2Loop*>(reg)) {
            S2Loop* preg = dynamic_cast<S2Loop*>(reg);
            S1Angle ang = preg->GetDistance(ll.ToPoint());
            if (ang.e6() == 0 )
            {
                portal p = portal_from_json(jv);
                results.push_back(p);
            }
        }

    }

    return results;
}


// these will all be single portals.
vector<portal> portal_factory::cluster_from_array(const vector<string>& desc) const
{

    Json::Value res = read_json_from_array(desc);
    
    // filter
    vector<portal>results;

    for (Json::Value jv: res)
    {
        for (string tt: desc) {
            //if (jv["title"] == tt)
            if (json_matches(jv,tt))
            {
                portal p = portal_from_json(jv);
                results.push_back(p);
                //pair<string,portal> gloc (p.get_guid(),p);
                //results->insert(gloc);
            }
        }
    }

    return results;
}

// this is used for arbitrary regions
// any lat/lngs do not need to be exact portals.
vector<point> portal_factory::get_array(const vector<string>& desc) const
{

    // check desc for points.
    vector<string> unknown;
    vector<point> results;
    for (string po : desc)
        if (is_point(po))
            results.push_back(point_from(po));
        else
            unknown.push_back(po);

    Json::Value res = read_json_from_array(unknown);
    
    // filter
    //vector<point> results;

    for (Json::Value jv: res)
    {
        for (string tt: desc) {
            // if (jv["title"] == tt)
            if (json_matches(jv,tt))
            {
                portal p =portal_from_json(jv);
                results.push_back(p);
            }
        }
    }

    return results;
}

Json::Value portal_factory::read_json_from_array(const vector<string>& desc) const
{
    Json::Value res;
    if (portal_api.substr(0,4) == "file")
    {
        res = json_reader::read_json_from_file(portal_api);
    } else {
        Json::Value query;

        int count=0;
        for (string title: desc)
        {
            query[count++] = title;
        }
        ostringstream jplist;
        jplist << query;
        string url = portal_api+"?portals="+curlpp::escape(jplist.str());
        res = json_reader::read_json_from_http(url);
    }
    return res;
}

bool portal_factory::json_matches(Json::Value jv, std::string desc) const
{
    regex rguid("^[0-9a-fA-F]{32}\\.1[16]$");
    regex rlatlng("(\\+|-)?([0-9]+(\\.[0-9]+)),(\\+|-)?([0-9]+(\\.[0-9]+))");

    if (regex_match(desc, rguid))
    {
        if (jv["guid"] == desc)
            return true;
        return false;
    }
    if (regex_match(desc,rlatlng))
    {
        point p = point_from(desc);
        double lat = jv["lat"].asDouble() / 1000000;
        double lng = jv["lng"].asDouble() / 1000000;

        point jvp = point (lat,lng);
        if (p == jvp)
            return true;
        return false;
    }

    if (jv["title"].asString() == desc)
        return true;

    return false;

}


bool portal_factory::is_point(string p) const
{
    vector<string> comment_desc = split_str(p,'#'); // optional comment
    p = comment_desc[0];
    vector<string> point_desc = split_str(p,',');
    if (point_desc.size() != 2)
        return false;

    size_t pos;
    string str;
    str = point_desc.at(0);
    stod(str, &pos);
    if (pos != str.size())
        return false;

    str = point_desc.at(1);
    stod(str, &pos);
    if (pos != str.size())
        return false;

    return true;
}

point portal_factory::point_from(string p) const
{
    point po;
    if (is_point(p))
    {
        vector<string> comment_desc = split_str(p,'#'); // optional comment
        p = comment_desc[0];
        vector<string> point_desc = split_str(p,',');

        double lat = stod(point_desc.at(0));
        double lng = stod(point_desc.at(1));
        po = point(lat,lng);
    }
    return po;
}

vector<point> portal_factory::points_from_string(string p) const
{
    vector<string> point_desc = split_str(p,',');
    vector<point> pa;

    if (point_desc.size() % 2 == 1)
        return pa; // probably should throw an exception
                                
    for (int i =0; i < point_desc.size(); i += 2)
        pa.push_back(point(point_desc.at(i),point_desc.at(i+1)));


    return pa;

}

vector<portal> portal_factory::remove_portals(const vector<portal>& portals, const vector<portal>& remove) const
{
    vector<portal> pa;
    for (portal p1 : portals)
    {
        bool found = false;
        for (portal p2 : remove)
        {
            if (p1 == p2)
            {
                found = true;
                break;
            }
        }
        if(!found)
            pa.push_back(p1);
    }
    return pa;
}


}
