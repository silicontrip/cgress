#include "portal.hpp"

using std::string;
using std::vector;
using std::unordered_map;
using std::any;
using std::any_cast;

namespace silicontrip {

		string portal::get_guid() const { return guid; }
		string portal::get_title() const { return title; }
		int portal::get_health() const { return health; }
		int portal::get_res_count() const { return res_count; }
		string portal::get_team() const { return team; }
		int portal::get_level() const { return level; }

		string portal::get_short_team() const
		{
			if (team[0] == 'E' )
                return "ENL";
        	if (team[0] == 'R' )
				return "RES";
        	if (team[0] == 'N' )
				return "NEU";
                
			return "";
		}
	
		void portal::set_guid(std::string s) { guid = s; }
		void portal::set_title(std::string s) { title = s; }
		void portal::set_team(std::string s) { team = s; }
		void portal::set_health(int i) { health = i; }
		void portal::set_res_count(int i) { res_count = i; }
		void portal::set_level(int i) { level = i; }

		bool portal::is_enlightened() { return team[0] == 'E'; }
		bool portal::is_resistance() { return team[0] == 'R'; }
		bool portal::is_machina() { return team[0] == 'N'; };

		vector<portal>* portal::connected_portals(const vector<link>& l, unordered_map<string, portal>& pm)
		{
			vector<portal>* connected = new vector<portal>();
			for (link li: l)
            {
				string lguid;
				if (li.d_s2latlng() == latlng )
				{
					try
					{
						portal po = pm[li.get_dguid()];
						connected->push_back(po);
					} catch(int &e) { ; }
				}
			}
			return connected;
		}

		portal::portal(const portal& po)
		{
			guid = po.guid;
			title = po.title;
			health = po.health;
			res_count = po.res_count;
			team = po.team;
			level = po.level;

			latlng = S2LatLng::FromE6(po.latlng.lat().e6(),po.latlng.lng().e6());
		}
		/*
		portal::portal(unordered_map<string,any>& pt) 
		{
			guid = any_cast<string>(pt["guid"]);
			title = any_cast<string>(pt["title"]);
			health = any_cast<int>(pt["health"]);
			res_count = any_cast<int>(pt["rescount"]);
			team = any_cast<string>(pt["team"]);
			level = any_cast<int>(pt["level"]);

			long la = any_cast<long>(pt["lat"]);
			long lo = any_cast<long>(pt["lng"]);
			latlng = S2LatLng::FromE6(la,lo);
		}
		*/
		portal::portal(string g, string ti, int h, int r, string te, int le, long la, long lo)
		{
			guid = g;
			title = ti;
			health = h;
			res_count = r;
			team = te;
			level = le;
			latlng = S2LatLng::FromE6(la,lo);
		}

		portal::portal() { ; }

		string portal::to_string() const
		{
			return "" + title + " (" + get_short_team() + ") " + latlng.ToStringInDegrees();
		}
}

std::ostream& operator<<(std::ostream& os, const silicontrip::portal& p)
{
    os << p.to_string();
    return os;
}