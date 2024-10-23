#ifndef SILICONTRIP_JSON_READER_HPP
#define SILICONTRIP_JSON_READER_HPP

#include <string>
#include <iostream>
#include <fstream>

#include <json/json.h>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Infos.hpp>

namespace silicontrip {

class json_reader {
	public:
		static Json::Value read_json_from_file(const std::string url);
		static Json::Value read_json_from_http(const std::string url);
};

}

#endif
