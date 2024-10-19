#ifndef SILICONTRIP_LINK_HPP
#define SILICONTRIP_LINK_HPP

#include "line.hpp"
#include <string>
#include <vector>
#include <any>
#include <s2/s2latlng.h>

namespace silicontrip {

enum ingressteam {
  RESISTANCE,
  ENLIGHTENED,
  NEUTRAL
};

        class link: public line {
            private:
                std::string guid;
                std::string d_guid;
                std::string o_guid;
                std::string team;
                enum ingressteam team_enum;

            public:
                std::string get_guid() const;
                std::string get_dguid() const;
                std::string get_oguid() const;

               // S2LatLng get_os2latlng() const;
               // S2LatLng get_ds2latlng() const;
                
                std::vector<link>* get_intersects (const std::vector<link>& l) const;

                void set_team(std::string s);
                ingressteam get_team_enum() const;
                
                link(std::unordered_map<std::string,std::any>& pt);
                link(const link& li);
                link(std::string g, std::string dg, long dla, long dlo, std::string og, long ola, long olo, std::string tt);
                link();

                std::string to_string() const;
        };
}

std::ostream& operator<<(std::ostream& os, const silicontrip::link& l);

#endif