#ifndef SILICONTRIP_FIELD_FACTORY_HPP
#define SILICONTRIP_FIELD_FACTORY_HPP

#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <vector>

#include <json/json.h>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Options.hpp>

#include <s2/s2latlng.h>

#include "field.hpp"
#include "team_count.hpp"
#include "link.hpp"
#include "uniform_distribution.hpp"
#include "json_reader.hpp"

namespace silicontrip {

class field_factory {
    private:
        std::string cell_api;
        std::unordered_map<std::string, uniform_distribution> mu_cache;
        field_factory();  // no one else can create one
  		~field_factory(); // prevent accidental deletion

  		static field_factory* ptr;

        Json::Value json_from_array(const std::vector<std::string>& desc) const;
        std::unordered_map<std::string, uniform_distribution>query_mu_from_servlet(const std::vector<std::string>& cell_tokens) const;
        std::unordered_map<std::string, uniform_distribution>query_mu(const std::vector<std::string>& cells);
        bool link_exists(const std::vector<line>&l, int j, point p1, point p2) const;

    public:
		  static field_factory* get_instance();

  		field_factory(const field_factory&) = delete;
  		field_factory(field_factory&&) = delete;
  		field_factory& operator=(const field_factory&) = delete;
  		field_factory& operator=(field_factory&&) = delete;

      std::vector<field> over_target(const std::vector<field>&f, const std::vector<point>& t) const;
      std::vector<field> percentile(const std::vector<field>&f, double percent) const;
      std::vector<field> filter_fields(const std::vector<field>&f, const std::vector<link>&l, team_count tc) const;
      std::vector<field> filter_existing_fields(const std::vector<field>&f, const std::vector<link>&l) const;

      S2Polygon s2polygon(const field& f) const;
      S2CellUnion cells(const S2Polygon& p) const;
      std::unordered_map<S2CellId,double> cell_intersection(const S2Polygon& p) const;
      int calculate_mu(const S2Polygon& p);
      int get_est_mu(const field& f);

      std::vector<field> make_fields_from_single_links(const std::vector<line>&l) const;
      // the argument order is important.
      // two from lines1 and 1 from lines2
      std::vector<field> make_fields_from_double_links(const std::vector<line>&lk1, const std::vector<line>&lk2) const;
      std::vector<field> make_fields_from_triple_links(const std::vector<line>&lk1, const std::vector<line>&lk2, const std::vector<line>&lk3) const;

};

}

template<> struct std::hash<S2CellId> {
    std::size_t operator()(S2CellId const& s) const noexcept {
        std::size_t h1 = std::hash<uint64>{}(s.id());
        return h1; 
    }
};

#endif
