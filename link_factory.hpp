#ifndef SILICONTRIP_LINK_FACTORY_HPP
#define SILICONTRIP_LINK_FACTORY_HPP

#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <vector>

#include <json/json.h>
#include <curlpp/Options.hpp>


#include "portal.hpp"
#include "link.hpp"
#include "team_count.hpp"
#include "json_reader.hpp"

namespace silicontrip {

class link_factory {
    private:
      std::string link_api;
      link_factory();  // no one else can create one
      ~link_factory(); // prevent accidental deletion

      static link_factory* ptr;

    public:
      static link_factory* get_instance();

      link_factory(const link_factory&) = delete;
      link_factory(link_factory&&) = delete;
      link_factory& operator=(const link_factory&) = delete;
      link_factory& operator=(link_factory&&) = delete;
          
      std::vector<link> purge_links(const std::vector<portal>& portals, const std::unordered_map<std::string,link>& links) const;
      std::vector<link> get_purged_links (const std::vector<portal>& portals) const;
      std::vector<link> links_in_rect(S2LatLngRect bound, const std::unordered_map<std::string,link>& links) const;
      std::unordered_map<std::string,link> get_all_links() const;

      std::vector<line> make_lines_from_single_cluster(const std::vector<portal>& portals) const;
      std::vector<line> make_lines_from_double_cluster(const std::vector<portal>& portals1, const std::vector<portal>& portals2) const;

      std::vector<line> percentile_lines(const std::vector<line>& lines, double percentile) const;
      std::vector<line> filter_links(const std::vector<line>& lines, const std::vector<link>& links, team_count max) const;
      std::vector<line> filter_link_by_blocker (const std::vector<line>& lines, const std::vector<link>& links, const std::vector<portal>& portals) const;
      std::vector<silicontrip::link> filter_link_by_portal (const std::vector<silicontrip::link>& lines, const std::vector<portal>& portals) const;
      std::vector<line> filter_link_by_length (const std::vector<line>& lines,double km) const;

};

}

#endif
