#include "arguments.hpp"

using namespace std;

namespace silicontrip {

arguments::arguments(int argc, char* argv[])
{
	for (int i =0; i < argc; i++)
		args.push_back(argv[i]);
}

void arguments::add_req(std::string sa,std::string la,bool v)
{
	struct argsreq t;

	t.short_arg = sa;
	t.long_arg = la;
	t.val_req = v;

	required_args.push_back(t);
}

bool arguments::parse()
{
	for (int i=0; i<args.size(); i++)
	{
		string thisArg = args.at(i);
		if (thisArg.substr(0,2) == "--")
		{
			bool found = false;
			for (int j=0; j<required_args.size(); j++)
			{
				struct argsreq sReq = required_args.at(j);
				if (thisArg.substr(2) == sReq.long_arg)
				{
					i++;
					string t = args.at(i);
					parsed_args[sReq.long_arg]=t;
					parsed_args[sReq.short_arg]=t;
					found = true;
					break;
				}
			}
			if (!found)
				return false;
		} 
		else if (thisArg[0] == '-')
		{
			bool found = false;
			for (int j=0; j<required_args.size(); j++)
			{
				struct argsreq sReq = required_args.at(j);
				if (thisArg.substr(1) == sReq.short_arg)
				{
					i++;
					string t = args.at(i);
					parsed_args[sReq.long_arg]=t;
					parsed_args[sReq.short_arg]=t;
					found = true;
					break;
				}
			}
			if (!found)
				return false;
		} else {
			remain_args.push_back(thisArg);
		}

	}
	return true;
}

int arguments::argument_size() const { return remain_args.size(); }
vector<string> arguments::get_arguments() const { return remain_args; }
string arguments::get_argument_at(int i) const { return remain_args.at(i); }
double arguments::get_argument_at_as_double(int i) const {
	double r;
	istringstream(remain_args.at(i)) >> r;
	return r;
}
int arguments::get_argument_at_as_int(int i) const {
	int r;
	istringstream(remain_args.at(i)) >> r;
	return r;
}
unordered_map<string,string> arguments::get_options() const { return parsed_args; }
bool arguments::has_option(string s) const { return parsed_args.count(s)>0; }
string arguments::get_option_for_key(string s) const { 
	if (has_option(s))
		return parsed_args.at(s); 
	return "";
}
double arguments::get_option_for_key_as_double(string s) const 
{
	double r;
	istringstream(parsed_args.at(s)) >> r;
	return r; 
}
int arguments::get_option_for_key_as_int(string s) const 
{
	int r;
	istringstream(parsed_args.at(s)) >> r;
	return r; 
}




}
