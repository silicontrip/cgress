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

	srand(time(NULL));
	rt.start();

	arguments ag(argc,argv);
	ag.add_req("C","colour",true); // drawtools colour
	ag.add_req("h","help",false);

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

	draw_tools dt;

	if (ag.has_option("C")) {
		cerr << "set colour: " << ag.get_option_for_key("C") << endl;
		dt.set_colour(ag.get_option_for_key("C"));
	}

	cout << "start." << endl;	

	portal_factory* pf = portal_factory::get_instance();


	vector<portal> all_portals;

	if (ag.argument_size() == 1)
		all_portals = pf->vector_from_map(pf->cluster_from_description(ag.get_argument_at(0)));

	for (portal p : all_portals)
		dt.add(p);

	cout << dt.to_string() << endl;

	cout << "stop: " << rt.stop() << endl;

	return 0;
}
