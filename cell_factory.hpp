#ifndef SILICONTRIP_CELL_FACTORY_HPP
#define SILICONTRIP_CELL_FACTORY_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <s2/s2latlng.h>
#include <s2/s2cell_id.h>
#include <s2/s2cell.h>

#include "uniform_distribution.hpp"

namespace silicontrip {

class cell_factory {
	private:

		std::vector<float> buffer;
                static cell_factory* ptr;



		cell_factory();  // no one else can create one
		~cell_factory(); // prevent accidental deletion

	public:
		static cell_factory* get_instance();

		cell_factory(const cell_factory&) = delete;
		cell_factory(cell_factory&&) = delete;
		cell_factory& operator=(const cell_factory&) = delete;
		cell_factory& operator=(cell_factory&&) = delete;

		static std::pair<int,int> toXY(S2LatLng ll);
		double query_mu(std::string cellid) const;
		std::unordered_map<std::string, uniform_distribution> query_mu(const std::vector<std::string>& cells);

};

}

#endif

