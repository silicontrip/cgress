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
        std::unordered_map<std::string, uniform_distribution> mu_cache; // cell mu cache
        std::unordered_map<field,uniform_distribution> field_mu_cache;

        field_factory();  // no one else can create one
  		~field_factory(); // prevent accidental deletion

  		static field_factory* ptr;

        Json::Value json_from_array(const std::vector<std::string>& desc) const;
        std::unordered_map<std::string, uniform_distribution>query_mu_from_servlet(const std::vector<std::string>& cell_tokens) const;
        bool link_exists(const std::vector<line>&l, int j, point p1, point p2) const;
        bool share_line_index(const std::unordered_map<point, std::unordered_set<size_t>>& point_exists, const point& p1, const point& p2) const;
        uniform_distribution calculate_mu(const S2Polygon& p);
        uniform_distribution get_ud_mu(const field& f);
        int get_est_mu(const field& f);  // use get_cache_mu instead
        S2Polygon s2polygon(const field& f) const;
        S2CellUnion cells(const S2Polygon& p) const;
        std::unordered_map<S2CellId,double> cell_intersection(const S2Polygon& p) const;
        std::vector<std::string> union_to_tokens(const S2CellUnion& s2u) const;


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
        std::vector<field> filter_fields_with_cell(const std::vector<field>&f,std::string s2cellid_token) const;

        std::unordered_map<std::string,double> cell_intersection(const field& f) const; // prefered for cellfields
        std::vector<std::string> celltokens(const field& f) const; // prefered for cellfields
        std::unordered_map<std::string, uniform_distribution>query_mu(const std::vector<std::string>& cells); // used by cellfields
        int get_cache_mu(const field& f);
        uniform_distribution get_cache_ud_mu(const field& f);


        std::vector<field> make_fields_from_single_links(const std::vector<line>&l) const;
        std::vector<field> make_fields_from_single_links_v2(const std::vector<line>&l) const;

        // the argument order is important.
        // two from lines1 and 1 from lines2
        std::vector<field> make_fields_from_double_links(const std::vector<line>&lk1, const std::vector<line>&lk2) const;
        std::vector<field> make_fields_from_triple_links(const std::vector<line>&lk1, const std::vector<line>&lk2, const std::vector<line>&lk3) const;
        std::vector<field> get_splits(const field& f1, const field& f2) const;
        std::vector<field> add_splits(const std::vector<field>& fields) const;

};

}

template<> struct std::hash<S2CellId> {
    std::size_t operator()(S2CellId const& s) const noexcept {
        std::size_t h1 = std::hash<uint64_t>{}(s.id());
        return h1; 
    }
};

#endif
