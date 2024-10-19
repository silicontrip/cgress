#include "link_factory.hpp"

using namespace std;

namespace silicontrip {

link_factory* link_factory::ptr = 0;

link_factory::link_factory() 
{
    ifstream file_properties("portal_factory_properties.json");
    Json::Value properties;

    file_properties >> properties;

    link_api = properties["linkurl"].asString();

}
link_factory* link_factory::get_instance()
{
    if (!ptr)
    ptr = new link_factory();
    return ptr;
}

// Hmm, identical to the portal_factory versions.
Json::Value* link_factory::read_json_from_file(const string url) const
{
    int pathsep = url.find_first_of('/');
    string path = url.substr(pathsep);

    Json::Value* result = new Json::Value;

    ifstream file(path);

    file >> *result;

    return result;
}

Json::Value* link_factory::read_json_from_http(const string url) const
{

    Json::Value* result = new Json::Value();

    stringstream str_res;
    str_res << curlpp::options::Url(url);
    str_res >> *result;

    return result;
}

vector<link>* link_factory::purge_links(vector<portal>* portals, unordered_map<string,link>* links) const
{

    S2LatLngRect bound;

    for(portal po: *portals)
    {
        bound.AddPoint(po.s2latlng());
    }

    S2Point p1 = bound.GetVertex(0).ToPoint();
    S2Point p2 = bound.GetVertex(1).ToPoint();
    S2Point p3 = bound.GetVertex(2).ToPoint();
    S2Point p4 = bound.GetVertex(3).ToPoint();

    vector<link>* new_links = new vector<link>();

    for ( auto& li: *links) {
        link link = li.second;
        if (bound.GetDistance(link.d_s2latlng()).radians()==0 || bound.GetDistance(link.o_s2latlng()).radians()==0)
        {
            new_links->push_back(link);
// this might be expensive, but hopefully the above case catches most
        } else if (S2::CrossingSign(link.o_s2point(),link.d_s2point(),p1,p2)==1 ||
                   S2::CrossingSign(link.o_s2point(),link.d_s2point(),p2,p3)==1 ||
                   S2::CrossingSign(link.o_s2point(),link.d_s2point(),p3,p4)==1 ||
                   S2::CrossingSign(link.o_s2point(),link.d_s2point(),p4,p1)==1) {

            new_links->push_back(link);
        }
    }
    return new_links;
}

unordered_map<string,link>* link_factory::get_all_links() const
{
    Json::Value* res;
    if (link_api.substr(0,4) == "file")
    {
        res = read_json_from_file(link_api);
    } else {
        res = read_json_from_http(link_api);
    }

    unordered_map<string,link>* result = new unordered_map<string,link>();

    for (Json::Value jv: *res)
    {
        link li = link(
            jv["guid"].asString(),
            jv["dguid"].asString(),
            jv["dlat"].asInt(),
            jv["dlng"].asInt(),
            jv["oguid"].asString(),
            jv["olat"].asInt(),
            jv["olng"].asInt(),
            jv["team"].asString()
        );

        pair<string,link> glink (li.get_guid(),li);
        result->insert(glink);

    }
    delete res;
    return result;
}

vector<link>* link_factory::get_purged_links (vector<portal>* portals) const
{
    unordered_map<string,link>* links = get_all_links();
    return purge_links(portals,links);
}

vector<line>* link_factory::make_lines_from_single_cluster(vector<portal>* portals) const
{
    vector<line>* la = new vector<line>();
                
    for (int i =0; i<portals->size(); i++)
    {
        portal pki = portals->at(i);
                        
        for (int j=i+1; j < portals->size(); j++)
        {
            portal pkj = portals->at(j);
            line li = line (pki,pkj);
            la->push_back(li);
        }
    }
    return la;
}

vector<line>* link_factory::make_lines_from_double_cluster(vector<portal>* portals1, vector<portal>* portals2) const
{
    vector<line>* la = new vector<line>();            
    for (portal pki: *portals1)
    {          
        for (portal pkj: *portals2)
        {
            line li = line(pki,pkj);
            la->push_back(li);
        }
    }
    return la;
}

bool geo_comparison(const pair<double,line>& a, const pair<double,line>& b)
{
    a.first < b.first;
}

vector<line>* link_factory::percentile_lines(std::vector<line>* lines, double percentile) const
{
    vector< pair<double,line> > v;

    for (line li: *lines)
    {
        pair<double,line> pa (li.geo_distance(),li);
        v.push_back(pa);
    }
    sort(v.begin(),v.end(),geo_comparison);

    int end = round((v.size() * percentile / 100.0));

    v.resize(end);

    vector<line>* result = new vector<line>();

    for (pair<double,line> pali: v)
    {
        result->push_back(pali.second);
    }

    return result;
}

vector<line>* link_factory::filter_links(vector<line>* lines, vector<link>* links, team_count max) const
{
    vector<line>* la = new vector<line>();
    for (line l: *lines) 
    {                            
        team_count bb;
        for (link link: *links) 
        {
            if (l.intersects(link)) {
                bb.inc_team_enum(link.get_team_enum());
            }
            if (bb > max)
                break;
        }
        if (!(bb > max))
            la->push_back(l);
    }
        
    return la;

}



}