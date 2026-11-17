#ifndef SILICONTRIP_CELL_FACTORY_HPP
#define SILICONTRIP_CELL_FACTORY_HPP

#include <s2/s2latlng.h>
#include <s2/s2cell_id.h>
#include <s2/s2cell.h>

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "field.hpp"
#include "link.hpp"
#include "uniform_distribution.hpp"
#include "s2cellid_hash.hpp"

namespace silicontrip {

class cell_factory {
	private:

		std::vector<float> buffer;
                static cell_factory* ptr;



		cell_factory();  // no one else can create one
		~cell_factory(); // prevent accidental deletion

		static std::pair<int,int> toXY(S2LatLng ll);
		static S2Polygon s2polygon(const field& f);
		static std::vector<std::string> union_to_tokens(const S2CellUnion& s2u);
		static std::unordered_map<S2CellId,double> cell_intersection(const S2Polygon& p);
		static S2CellUnion cells(const S2Polygon& p);


	public:
		static cell_factory* get_instance();

		cell_factory(const cell_factory&) = delete;
		cell_factory(cell_factory&&) = delete;
		cell_factory& operator=(const cell_factory&) = delete;
		cell_factory& operator=(cell_factory&&) = delete;

		double query_mu(std::string cellid) const;
		std::unordered_map<std::string, uniform_distribution> query_mu(const std::vector<std::string>& cells) const;

		uniform_distribution calculate_mu(const S2Polygon& p) const;
		uniform_distribution get_ud_mu(const field& f);
		int get_mu(const field& f);

};

}

#endif

