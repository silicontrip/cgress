#ifndef SILICONTRIP_PORTAL_FACTORY_HPP
#define SILICONTRIP_PORTAL_FACTORY_HPP

#include "portal.hpp"

#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <vector>

#include <json/json.h>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <s2/s2region.h>
#include <s2/s2cell_id.h>
#include <s2/s2cell.h>
#include <s2/s2cap.h>
#include <s2/s2latlng_rect.h>
#include <s2/s2loop.h>
#include <s2/s2polygon.h>

namespace silicontrip {

class portal_factory {
	private:
		std::string portal_api;
		portal_factory();  // no one else can create one
  		~portal_factory(); // prevent accidental deletion

  		static portal_factory* ptr;

		std::vector<std::string>* split_str(const std::string str, char del) const;
		Json::Value* read_json_from_file(const std::string url) const;
		Json::Value* read_json_from_http(const std::string url) const;
		S2Loop* s2loop_from_json(std::string desc) const;
		std::vector<portal>* get_array(const std::vector<std::string>* desc) const;
		Json::Value* read_json_from_array(const std::vector<std::string>* desc) const;
		portal portal_from_json(Json::Value jp) const;

	public:
		static portal_factory* get_instance();

  		portal_factory(const portal_factory&) = delete;
  		portal_factory(portal_factory&&) = delete;
  		portal_factory& operator=(const portal_factory&) = delete;
  		portal_factory& operator=(portal_factory&&) = delete;

		std::unordered_map<std::string,portal>* cluster_from_description(const std::string desc) const;
		std::unordered_map<std::string,portal>* cluster_from_file(const std::string desc) const;
		std::unordered_map<std::string,portal>* cluster_from_region(S2Region* reg) const;
		std::unordered_map<std::string,portal>* cluster_from_array(const std::vector<std::string>* desc) const;
		portal get_single(std::string desc) const;

};

}

#endif 
