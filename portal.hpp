#ifndef SILICONTRIP_PORTAL_HPP
#define SILICONTRIP_PORTAL_HPP

#include <string>

#include "link.hpp"
#include "point.hpp"

#include <vector>
#include <unordered_map>
#include <any>

namespace silicontrip {

class portal: public point {
	private:
		std::string guid;
		std::string title;	
		int health;
		int res_count;
		std::string team;
		int level;

	public:

		std::string get_guid() const;
		std::string get_title() const;
		int get_health() const;
		int get_res_count() const;
		std::string get_team() const;
		int get_level() const;

		std::string get_short_team() const;
		
		void set_guid(std::string s);
		void set_title(std::string s);
		void set_team(std::string s);
		void set_health(int i);
		void set_res_count(int i);
		void set_level(int i);

		bool is_enlightened();
		bool is_resistance();
		bool is_machina();

		std::vector<portal>* connected_portals(const std::vector<link>& l,std::unordered_map<std::string, portal>& p);

		portal(const portal& po);
		//portal(std::unordered_map<std::string,std::any>& pt);
		portal(std::string g, std::string ti, int h, int r, std::string te, int le, long la, long lo);
		portal();
		
		std::string to_string() const;	
};

}
std::ostream& operator<<(std::ostream& os, const silicontrip::portal& p);

#endif
