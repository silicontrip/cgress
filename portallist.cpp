#include <iostream>
#include <chrono>
#include <thread>
#include <unordered_map>
#include "run_timer.hpp"
#include "portal_factory.hpp"
#include "arguments.hpp"
#include "draw_tools.hpp"


using namespace std;

void print_usage()
{
		cerr << "Usage:" << endl;
		cerr << "portallist [options] <portal cluster> " << endl;
		cerr << "Options:" << endl;
		cerr << " -C <#colour>      Set Drawtools output colour" << endl;
}

int main (int argc, char* argv[])
{
	using namespace silicontrip;
	run_timer rt;

	int output_type = 0;

	//srand(time(NULL));
	rt.start();

	arguments ag(argc,argv);
	ag.add_req("C","colour",true); // drawtools colour
	ag.add_req("h","help",false);
	ag.add_req("p","portallist",false); 

	if (!ag.parse())
	{
		print_usage();
		exit(1);
	}

	if (ag.has_option("h"))
	{
		print_usage();
		exit(1);
	}

	if (ag.has_option("p"))
		output_type = 1;

	draw_tools dt;

	if (ag.has_option("C")) {
		cerr << "set colour: " << ag.get_option_for_key("C") << endl;
		dt.set_colour(ag.get_option_for_key("C"));
	}

	cerr << "start." << endl << endl;	

	try {
	portal_factory* pf = portal_factory::get_instance();


	vector<portal> all_portals;

	if (ag.argument_size() == 1)
		all_portals = pf->cluster_from_description(ag.get_argument_at(0));

	if (output_type == 0)
	{
		for (portal p : all_portals)
			dt.add(p);

		cout << dt.to_string() << endl;
	}

	if (output_type == 1)
	{
		// check for duplicates
		unordered_set<string> portal_names;
		unordered_set<string> duplicates;
		for (portal p : all_portals)
		{			
			if (portal_names.find(p.get_title()) == portal_names.end())
				portal_names.insert(p.get_title());
			else
				duplicates.insert(p.get_title());
		}
		for (portal p : all_portals)
		{
			if (duplicates.find(p.get_title()) == portal_names.end())
				cout << p.get_title() << endl;
			else
				cout << p.point::to_string() << " (" << p.get_title() << ")" << endl;
		}
	}
	} catch (exception &e) {
		cerr << "An Error occured: " << e.what() << endl;
	}
	cerr << endl << "stop: " << rt.stop() << endl;

	return 0;
}
