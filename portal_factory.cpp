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
        for (Json::Value pp: dt["latLngs"])
        {
            double lat = pp["lat"].asDouble();
            double lng = pp["lng"].asDouble();
            S2LatLng ll = S2LatLng::FromDegrees(lat,lng);
            loop_points->push_back(ll.ToPoint());
        }
    }
    S2Loop* result = new S2Loop(*loop_points);
    delete loop_points;
    return result;
}

Json::Value portal_factory::read_json_from_file(const string url) const
{
    int pathsep = url.find_first_of('/');
    string path = url.substr(pathsep);

    Json::Value result;

    ifstream file(path);

    file >> result;

    return result;
}

Json::Value portal_factory::read_json_from_http(const string url) const
{

    Json::Value result;

    stringstream str_res;
    str_res << curlpp::options::Url(url);
    str_res >> result;

    return result;
}

portal portal_factory::get_single(const string desc) const
{
    Json::Value res;
    if (portal_api.substr(0,4) == "file")
    {
        res = read_json_from_file(portal_api);
    } else {
        string url = portal_api+"?ll="+curlpp::escape(desc);
        res = read_json_from_http(url);
    }
    // filter
    portal p;

    for (Json::Value jv: res)
    {
        if (jv["title"] == desc)
        {
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
        }
    }

    return p;
}

unordered_map<string,portal> portal_factory::cluster_from_description(const string desc) const
{
    S2Region* search_region = nullptr;
    if (desc[0]=='.' && desc[1]=='/') { return cluster_from_file(desc); } 
    if (desc[0]=='[' && desc[1]=='{') 
    {
        search_region = s2loop_from_json(desc);
        return cluster_from_region(search_region);
    }
    if (desc[0]=='0' && desc[1]=='x') 
    {
        char* end;
        uint64 result = strtoul(desc.c_str(), &end, 16);
        S2CellId id = S2CellId(result << 32);
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
            portal ploc = get_single(desc);

            unordered_map<string,portal> result;
		result[ploc.get_guid()] = ploc;
            //pair<string,portal> gloc (ploc.get_guid(),ploc);
            //result.insert(gloc);

            return result;

        } else if (pd.size() == 2) {
            portal ploc = get_single(pd.at(0));
            S2Point loc = ploc.s2latlng().ToPoint();
            char* end;

            double rangek = strtod(pd.at(1).c_str(),&end);
            S1Angle range = S2Earth::KmToAngle(rangek);
            S2Cap s2c = S2Cap(loc,range);
            search_region = &s2c;

            return cluster_from_region(search_region);
        }
         
    } else if (sz==2) {
        vector<portal> ploc = get_array(pt);
        S2LatLngRect s2ll;

        s2ll = S2LatLngRect::FromPointPair(ploc.at(0).s2latlng(),ploc.at(1).s2latlng());
        search_region = &s2ll;

        return cluster_from_region(search_region);
    } else if (sz==3) {
        vector<portal> ploc = get_array(pt);

        //S2Builder::Options* opt = new S2Builder::Options();
        //S2Builder* builder = new S2Builder(opt);

        vector<S2Point> ppo = { ploc.at(0).s2latlng().ToPoint(), ploc.at(1).s2latlng().ToPoint(), ploc.at(2).s2latlng().ToPoint()};
        search_region = new S2Loop(ppo);

        return cluster_from_region(search_region);

    }
	unordered_map<string,portal> map;
    return map;
}

unordered_map<string,portal> portal_factory::cluster_from_file(const string desc) const
{
    unordered_map<string,portal> res;
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

vector<portal> portal_factory::vector_from_map(const unordered_map<string,portal>& portals) const
{
    vector<portal> poli;
	for (pair<string,portal> it: portals) 
		poli.push_back(it.second);
	return poli;
}

unordered_map<string,portal> portal_factory::cluster_from_region(S2Region* reg) const
{
    Json::Value res;
    if (portal_api.substr(0,4) == "file")
    {
        res = read_json_from_file(portal_api);
    } else {
       
        S2LatLngRect bound = reg->GetRectBound();

        string ll = bound.lo().ToStringInDegrees();
        string l2 = bound.hi().ToStringInDegrees();

        string url = portal_api+"?ll="+curlpp::escape(ll) + "&l2=" + l2;
        res = read_json_from_http(url);
    }

    // filter
    unordered_map<string,portal> results;

    for (Json::Value jv: res)
    {
        long la = jv["lat"].asInt();
        long ln = jv["lng"].asInt();

        S2LatLng ll = S2LatLng::FromE6(la,ln);
        S2Cell cell = S2Cell(ll);
        if (reg->Contains(cell))
        {
            portal p = portal_from_json(jv);
		results[p.get_guid()]=p;
            //pair<string,portal> gloc (p.get_guid(),p);
            //results->insert(gloc);
        } else if (dynamic_cast<S2Polygon*>(reg)) {
            S2Polygon* preg = dynamic_cast<S2Polygon*>(reg);
            S1Angle ang = preg->GetDistance(ll.ToPoint());
            if (ang.e6() == 0 )
            {
                portal p = portal_from_json(jv);
		results[p.get_guid()] = p;
                //pair<string,portal> gloc (p.get_guid(),p);
                //results->insert(gloc);
            }
        }

    }

    return results;
}


unordered_map<string,portal> portal_factory::cluster_from_array(const vector<string>& desc) const
{

    Json::Value res = read_json_from_array(desc);
    
    // filter
    unordered_map<string,portal>results;

    for (Json::Value jv: res)
    {
        for (string tt: desc) {
            if (jv["title"] == tt)
            {
                portal p = portal_from_json(jv);
		results[p.get_guid()] = p;
                //pair<string,portal> gloc (p.get_guid(),p);
                //results->insert(gloc);
            }
        }
    }

    return results;
}

vector<portal> portal_factory::get_array(const vector<string>& desc) const
{

    Json::Value res = read_json_from_array(desc);
    
    // filter
    vector<portal> results;

    for (Json::Value jv: res)
    {
        for (string tt: desc) {
            if (jv["title"] == tt)
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
        res = read_json_from_file(portal_api);
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
        res = read_json_from_http(url);
    }
    return res;
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

}
