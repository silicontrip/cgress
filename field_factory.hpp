#ifndef SILICONTRIP_FIELD_FACTORY_HPP
#define SILICONTRIP_FIELD_FACTORY_HPP

#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <vector>

#include <json/json.h>
#include <curlpp/Options.hpp>
#include <s2/s2latlng.h>

#include "field.hpp"
#include "team_count.hpp"
#include "link.hpp"

namespace silicontrip {

class field_factory {
    private:
		std::string cell_api;
		field_factory();  // no one else can create one
  		~field_factory(); // prevent accidental deletion

  		static field_factory* ptr;

		Json::Value* read_json_from_file(const std::string url) const;
		Json::Value* read_json_from_http(const std::string url) const;

        bool link_exists(std::vector<line>*l, int j, point p1, point p2) const;

    public:
		static field_factory* get_instance();

  		field_factory(const field_factory&) = delete;
  		field_factory(field_factory&&) = delete;
  		field_factory& operator=(const field_factory&) = delete;
  		field_factory& operator=(field_factory&&) = delete;

        std::vector<field>* over_target(std::vector<field>*f, std::vector<point>* t) const;
        std::vector<field>* percentile(std::vector<field>*f, double percent) const;
        std::vector<field>* filter_fields(std::vector<field>*f, std::vector<link>*l, team_count tc) const;
        std::vector<field>* filter_existing_fields(std::vector<field>*f, std::vector<link>*l) const;

        std::vector<field>* make_fields_from_single_links(std::vector<line>*l) const;
        // the argument order is important.
        // two from lines1 and 1 from lines2
        std::vector<field>* make_fields_from_double_links(std::vector<line>*lk1,std::vector<line>*lk2) const;
        std::vector<field>* make_fields_from_triple_links(std::vector<line>*lk1,std::vector<line>*lk2,std::vector<line>*lk3) const;

};

}

#endif
