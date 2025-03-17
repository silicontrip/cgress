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

vector<link> link_factory::purge_links(const vector<portal>& portals, const unordered_map<string,link>& links) const
{

    S2LatLngRect bound;

    for(portal po: portals)
    {
        bound.AddPoint(po.s2latlng());
    }


    S2Point p1 = bound.GetVertex(0).ToPoint();
    S2Point p2 = bound.GetVertex(1).ToPoint();
    S2Point p3 = bound.GetVertex(2).ToPoint();
    S2Point p4 = bound.GetVertex(3).ToPoint();

    vector<link> new_links;

    for ( pair<string,link> li: links) {

        link link = li.second;

        if (bound.GetDistance(link.d_s2latlng()).radians()==0 || bound.GetDistance(link.o_s2latlng()).radians()==0)
        {

            new_links.push_back(link);

// this might be expensive, but hopefully the above case catches most
        } else if (S2::CrossingSign(link.o_s2point(),link.d_s2point(),p1,p2)==1 ||
                   S2::CrossingSign(link.o_s2point(),link.d_s2point(),p2,p3)==1 ||
                   S2::CrossingSign(link.o_s2point(),link.d_s2point(),p3,p4)==1 ||
                   S2::CrossingSign(link.o_s2point(),link.d_s2point(),p4,p1)==1) {

            new_links.push_back(link);
        }
    }

    return new_links;
}

unordered_map<string,link> link_factory::get_all_links() const
{

    Json::Value res;
    if (link_api.substr(0,4) == "file")
    {
        res = json_reader::read_json_from_file(link_api);
    } else {
        res = json_reader::read_json_from_http(link_api);
    }

    unordered_map<string,link> result;

    for (Json::Value jv: res)
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

	result[li.get_guid()] = li;
        //pair<string,link> glink (li.get_guid(),li);
        //result->insert(glink);

    }

    return result;
}

vector<link> link_factory::get_purged_links (const vector<portal>& portals) const
{
    unordered_map<string,link> links = get_all_links();
    return purge_links(portals,links);
}

vector<line> link_factory::make_lines_from_single_cluster(const vector<portal>& portals) const
{
    vector<line> la;
                
    for (int i =0; i<portals.size(); i++)
    {
        portal pki = portals.at(i);
                        
        for (int j=i+1; j < portals.size(); j++)
        {
            portal pkj = portals.at(j);
            line li = line (pki,pkj);
            la.push_back(li);
        }
    }
    return la;
}

vector<line> link_factory::make_lines_from_double_cluster(const vector<portal>& portals1, const vector<portal>& portals2) const
{
    vector<line> la;
    for (portal pki: portals1)
    {          
        for (portal pkj: portals2)
        {
            line li = line(pki,pkj);
            la.push_back(li);
        }
    }
    return la;
}

bool geo_comparison(const pair<double,line>& a, const pair<double,line>& b)
{
    return a.first > b.first;
}

vector<line> link_factory::percentile_lines(const std::vector<line>& lines, double percentile) const
{
    vector< pair<double,line> > v;

    for (line li: lines)
    {
        pair<double,line> pa (li.geo_distance(),li);
        v.push_back(pa);

    }
    sort(v.begin(),v.end(),geo_comparison);

    int end = round((v.size() * percentile / 100.0));

    v.resize(end);

    vector<line> result;

    for (pair<double,line> pali: v)
    {
        result.push_back(pali.second);
    }

    return result;
}

vector<line> link_factory::filter_links(const vector<line>& lines, const vector<link>& links, team_count max) const
{
    vector<line> la;
    for (line l: lines) 
    {                            
        team_count bb;
        for (link link: links) 
        {
            if (l.intersects(link)) {
                bb.inc_team_enum(link.get_team_enum());
            }
            if (bb > max)
                break;
        }
        if (!(bb > max))
            la.push_back(l);
    }
        
    return la;

}

vector<line> link_factory::filter_link_by_blocker (const vector<line>& lines, const vector<link>& links, const vector<portal>& portals) const
{
    vector<line> la;
    for (line l: lines)
    {
        bool unblocked =true;
        for (link link: links)
        {
            if (l.intersects(link)) {
                point op = link.get_o_point();
                point dp = link.get_d_point();
                int count = 0;
                for (portal pp: portals)
                {
                    if (dp == pp || op == pp)
                        count++;
                    if (count==2)
                    {
                        unblocked = false;
                        break;
                    }
                }
            }
            if (!unblocked)
                break;
        }
        if (unblocked)
            la.push_back(l);
    }
    return la;
}



}
