#include "link.hpp"

using std::string;
using std::vector;
using std::unordered_map;
using std::any;
using std::any_cast;

namespace silicontrip {

    string link::get_guid() const { return guid; }
    string link::get_dguid() const { return d_guid; }
    string link::get_oguid() const { return o_guid; }

    vector<link>* link::get_intersects (const vector<link>& l) const
    {
        vector<link>* all = new vector<link>();
        for (link li : l)
        {
            if (intersects(li)) { all->push_back(li); }
        }
        return all;
    }

    void link::set_team(string s) { 
        team = s; 
        if (s[0] =='R')
            team_enum = RESISTANCE;
        if (s[0] == 'E')
            team_enum = ENLIGHTENED;
        if (s[0 == 'N'])
            team_enum = NEUTRAL;
    }

    ingressteam link::get_team_enum() const { return team_enum; }

    link::link() { ; }
    link::link(const link& li)
    {
        guid = li.guid;
        d_guid = li.d_guid;
        o_guid = li.o_guid;
        team = li.team;

        d_point = point(li.d_point);
        o_point = point(li.o_point);

        o_s2Point = o_point.s2latlng().ToPoint();
	    d_s2Point = d_point.s2latlng().ToPoint();
    }

    link::link(unordered_map<string,any>& pt)
    {
        guid = any_cast<string>(pt["guid"]);
        d_guid = any_cast<string>(pt["dguid"]);
        o_guid = any_cast<string>(pt["oguid"]);
        team = any_cast<string>(pt["team"]);

        long dlat = any_cast<long>(pt["dlat"]);
        long dlng = any_cast<long>(pt["dlng"]);
        long olat = any_cast<long>(pt["olat"]);
        long olng = any_cast<long>(pt["olng"]);

        d_point=point(dlat,dlng);
	    o_point=point(olat,olng);

	    o_s2Point = o_point.s2latlng().ToPoint();
	    d_s2Point = d_point.s2latlng().ToPoint();
    }

    link::link(string g, string dg, long dla, long dlo, string og, long ola, long olo, string tt)
    {
        guid = g;
        d_guid = dg;
        o_guid = og;
        team = tt;

        d_point = point(dla,dlo);
        o_point = point(ola,olo);

        o_s2Point = o_point.s2latlng().ToPoint();
	    d_s2Point = d_point.s2latlng().ToPoint();
    }

    string link::to_string() const
    {
        return team + " " + line::to_string() ;
    }


}

std::ostream& operator<<(std::ostream& os, const silicontrip::link& l)
{
    os << l.to_string();
    return os;
}