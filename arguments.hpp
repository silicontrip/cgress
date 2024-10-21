#ifndef SILICONTRIP_ARGUMENTS_HPP
#define SILICONTRIP_ARGUMENTS_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>

namespace silicontrip {

struct argsreq {
	std::string short_arg;
	std::string long_arg;
	bool val_req;
};

class arguments {
	private:
		std::vector<std::string> args;
		std::unordered_map<std::string,std::string> parsed_args;
		std::vector<std::string> remain_args;
		std::vector<struct argsreq> required_args;
	public:

		arguments(int argc, char* argv[]);
		void add_req(std::string sa,std::string la,bool v);
		bool parse();
		
		int argument_size() const;
		std::vector<std::string> get_arguments() const;
		std::string get_argument_at(int i) const;
		double get_argument_at_as_double(int i) const;
		int get_argument_at_as_int(int i) const;
		std::unordered_map<std::string,std::string> get_options() const;
		bool has_option(std::string s) const;
		std::string get_option_for_key(std::string s) const;
		double get_option_for_key_as_double(std::string s) const;
		int get_option_for_key_as_int(std::string s) const;

};

}

#endif
