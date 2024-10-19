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

namespace silicontrip {

class link_factory {
    private:
		std::string link_api;
		link_factory();  // no one else can create one
  		~link_factory(); // prevent accidental deletion

  		static link_factory* ptr;

		Json::Value* read_json_from_file(const std::string url) const;
		Json::Value* read_json_from_http(const std::string url) const;

    public:
		static link_factory* get_instance();

  		link_factory(const link_factory&) = delete;
  		link_factory(link_factory&&) = delete;
  		link_factory& operator=(const link_factory&) = delete;
  		link_factory& operator=(link_factory&&) = delete;

        
        std::vector<link>* purge_links(std::vector<portal>* portals, std::unordered_map<std::string,link>* links) const;
        std::vector<link>* get_purged_links (std::vector<portal>* portals) const;
        std::unordered_map<std::string,link>* get_all_links() const;

        std::vector<line>* make_lines_from_single_cluster(std::vector<portal>* portals) const;
        std::vector<line>* make_lines_from_double_cluster(std::vector<portal>* portals1, std::vector<portal>* portals2) const;

        std::vector<line>* percentile_lines(std::vector<line>* lines, double percentile) const;
        std::vector<line>* filter_links(std::vector<line>* lines, std::vector<link>* links, team_count max) const;

};

}

#endif